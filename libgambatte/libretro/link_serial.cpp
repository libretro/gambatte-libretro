#include "link_serial.h"

#include <string.h>

#include "gambatte_log.h"

/* Ticks in a serial transfer at normal speed, and at the Game Boy Color's
 * faster 262144 Hz rate. Both are what gambatte itself schedules -- 4096 and
 * 128 cycles -- doubled into link ticks. */
#define LINK_XFER_NORMAL ((uint64_t)8192)
#define LINK_XFER_FAST ((uint64_t)256)

/* Rendezvous roughly every quarter of a transfer.
 *
 * The horizon is how far two machines may drift apart, and it is also how stale
 * the peer's serial register can be when this one clocks a byte out of it. A
 * game arms its end -- writes SB, then SC -- and then waits, so the value is
 * settled well before it is read, and a quarter of a byte time (244us of a
 * millisecond) is comfortably inside the gap every link protocol leaves between
 * bytes. Real ones leave far more: a Pokemon trade moves a byte every few
 * milliseconds and Tetris one a frame.
 *
 * The floor is cost. Every grain is a rendezvous: the frontend stops one
 * machine there and runs the other up to it, so this cannot go much lower
 * without both cores spending their time changing places instead of
 * emulating. The fast figure is a quarter of a fast transfer by the same
 * reasoning and is thirty-two times as expensive; it is paid only while a Game
 * Boy Color is actually running its link at 256 kHz, which almost nothing
 * does. */
#define LINK_GRAIN_NORMAL ((uint64_t)2048)
#define LINK_GRAIN_FAST ((uint64_t)64)

/* Bus message. Packed by hand rather than shipped as a struct: both ends are
 * the same build today, but protocol_id exists so that another Game Boy core
 * could speak this later, and by then a shared struct layout would be an
 * assumption nobody remembers having made. */
enum
{
	LS_STATE = 1,
	LS_CLOCK
};

enum
{
	LS_FLAG_ARMED = 1,
	LS_FLAG_FAST = 2
};

#define LS_MSG_SIZE 8

LinkSerial::LinkSerial()
	: LinkPortBase(LINK_SERIAL_PROTOCOL, LINK_SERIAL_CLOCK_RATE,
	               "Link cable", "machine(s) on the wire")
	, doubleSpeed_(false)
	, atStart_(false)
	, checkable_(false)
	, horizon_(LINK_GRAIN_NORMAL)
	, grain_(LINK_GRAIN_NORMAL)
	, fastSeen_(false)
	, peerSb_(0xFF)
	, peerArmed_(false)
	, lastSb_(0)
	, lastFlags_(0)
	, published_(false)
	, busyUntil_(0)
	, busy_(false)
	, clockReady_(false)
	, clockAt_(0)
	, clockSb_(0xFF)
	, clockFast_(false)
	, pendingCount_(0)
{
	memset(pending_, 0, sizeof(pending_));
}

LinkSerial::~LinkSerial()
{
	stop();
}

void LinkSerial::attached()
{
	fastSeen_ = false;
	pendingCount_ = 0;
	published_ = false;
	clockReady_ = false;
	busy_ = false;
}

void LinkSerial::detached()
{
	pendingCount_ = 0;
	clockReady_ = false;
	busy_ = false;
}

void LinkSerial::reset()
{
	/* The cycle counter starts again from zero; the next chunk re-anchors on
	 * it. Whatever this machine had offered or was shifting is gone with its
	 * registers, so its state goes out again. */
	unanchor();
	published_ = false;
	busy_ = false;
}

uint64_t LinkSerial::transferTicks(bool fastCgb) const
{
	uint64_t ticks = fastCgb ? LINK_XFER_FAST : LINK_XFER_NORMAL;

	/* gambatte schedules a transfer as a fixed number of CPU cycles, and a CPU
	 * cycle is half as long in double speed, so the same transfer takes half the
	 * link ticks. That is the hardware's behaviour, not an approximation of it:
	 * a Game Boy Color in double speed really does clock its serial port twice
	 * as fast. */
	return doubleSpeed_ ? (ticks >> 1) : ticks;
}

/* A transfer this machine took part in has run its course. */
void LinkSerial::settle()
{
	if (busy_ && now() >= busyUntil_)
	{
		busy_ = false;
		published_ = false;
	}
}

void LinkSerial::wire(uint64_t tick, unsigned char type, unsigned char sb, unsigned char flags)
{
	unsigned char msg[LS_MSG_SIZE];

	memset(msg, 0, sizeof(msg));
	msg[0] = type;
	msg[1] = (unsigned char)busIndex();
	msg[2] = sb;
	msg[3] = flags;
	broadcast(tick, msg, sizeof(msg));
}

void LinkSerial::publish(unsigned char sb, unsigned char sc)
{
	unsigned char flags = 0;
	unsigned char out;

	/* Nobody on the wire to tell. A game polling for a partner flips SC
	 * thousands of times a second, and a message for each would slow a Game
	 * Boy playing alone, Super R.C. Pro-Am to half speed. Left unpublished,
	 * so the state goes out as soon as a peer joins (membership() clears
	 * published_ too). */
	if (peers() < 2)
	{
		published_ = false;
		return;
	}

	/* Armed means the guest has set the transfer bit and is waiting to be
	 * clocked. A machine in the middle of a transfer is NOT armed even though
	 * that bit is still set: the peer's byte is shifted into SB as the transfer
	 * runs, so what the register holds part way through is half of each byte and
	 * belongs to nobody. Saying so keeps a peer that clocks during it from
	 * reading the wreckage.
	 *
	 * The transfer SPEED is deliberately not published. SC bit 1 selects the Game
	 * Boy Color's faster clock, but an original Game Boy reads it back as 1
	 * whatever was written -- gambatte ORs 0x7E into every SC write, as the
	 * hardware's unused bits do -- so a state carrying it would call every DMG on
	 * the wire fast. Speed belongs to whichever machine clocks the transfer, and
	 * that is what LS_CLOCK carries. */
	if ((sc & 0x80) && !busy_)
		flags |= LS_FLAG_ARMED;

	out = (flags & LS_FLAG_ARMED) ? sb : 0xFF;

	if (published_ && out == lastSb_ && flags == lastFlags_)
		return;

	lastSb_ = out;
	lastFlags_ = flags;
	published_ = true;

	/* Stamped a horizon ahead, which is the promise this machine published to
	 * the bus and has to keep. A peer reading it at that tick is reading a
	 * register this one settled a horizon ago -- which is exactly the delay a
	 * cable's own length would not give you, and is the price of letting two
	 * machines run without asking each other's permission every cycle. */
	wire(now() + horizon_, LS_STATE, out, flags);
}

void LinkSerial::queue(const Pending &msg)
{
	if (pendingCount_ >= PENDING_MAX)
	{
		/* Nothing sane fills this: a peer publishes on change, and it cannot change
		 * more than a handful of times inside one horizon, because it can never be
		 * more than one ahead of this machine.
		 *
		 * So this drops the message and says so, rather than making room by acting
		 * on an older one early. Acting early would change what the guest reads at a
		 * moment decided by how many messages happened to have arrived, which is
		 * wall-clock luck, and that is the one thing that must not reach a guest two
		 * netplay peers are both replaying. */
		gambatte_log(RETRO_LOG_WARN,
			"Link cable: inbox full, dropping a message the horizon should have made"
			" impossible\n");
		return;
	}
	pending_[pendingCount_++] = msg;
}

void LinkSerial::apply(const Pending &msg)
{
	switch (msg.type)
	{
	case LS_STATE:
		peerSb_ = msg.sb;
		peerArmed_ = (msg.flags & LS_FLAG_ARMED) != 0;
		break;
	case LS_CLOCK:
		clockReady_ = true;
		clockAt_ = msg.tick;
		clockSb_ = msg.sb;
		clockFast_ = (msg.flags & LS_FLAG_FAST) != 0;
		break;
	default:
		break;
	}
}

void LinkSerial::applyDue()
{
	while (pendingCount_ > 0 && pending_[0].tick <= now())
	{
		apply(pending_[0]);
		memmove(&pending_[0], &pending_[1], sizeof(Pending) * (pendingCount_ - 1));
		--pendingCount_;
	}
}

void LinkSerial::received(uint64_t tick, unsigned from, const unsigned char *data, size_t len)
{
	Pending msg;

	(void)from;

	/* A size this driver does not speak is dropped. */
	if (len != LS_MSG_SIZE)
		return;

	msg.tick = tick;
	msg.type = data[0];
	msg.sb = data[2];
	msg.flags = data[3];
	queue(msg);
}

bool LinkSerial::carries(unsigned count)
{
	if (count <= 2)
		return true;

	/* The bus is protocol-agnostic and will happily join five machines; a
	 * Game Boy link cable joins two. The four-player adapter existed, but
	 * gambatte does not emulate it, so carrying a third machine's bytes
	 * would be inventing a transfer rather than reproducing one. */
	gambatte_log(RETRO_LOG_WARN,
	             "Link cable: %u machines on one wire; a Game Boy link cable carries 2\n", count);
	return false;
}

void LinkSerial::membership()
{
	/* Forget the other end, and say everything again.
	 *
	 * What was on the wire a moment ago is not evidence about what is on it
	 * now: a lead that has moved may be joining two different machines, and
	 * even the same one has been running unwatched in the meantime. Keeping
	 * the last state read would let a machine clock a byte out of a register
	 * it has not been told the contents of since before the cable moved.
	 *
	 * And a machine cabled in after the last change would otherwise never hear
	 * a state at all, leaving both ends waiting to be told the other was
	 * there. Anything already in flight belonged to the old cable and goes
	 * with it.
	 *
	 * The count need not have changed: a peer may leave and come back at one
	 * frame edge, and a state saved before this port heard of a change loads
	 * with the count the change left. */
	peerSb_ = 0xFF;
	peerArmed_ = false;
	published_ = false;
	clockReady_ = false;
	pendingCount_ = 0;
}

void LinkSerial::ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request)
{
	grain_ = fastSeen_ ? LINK_GRAIN_FAST : LINK_GRAIN_NORMAL;
	horizon_ = grain_;

	safe = promiseTo(tick + horizon_);

	request = tick + grain_;
	if (request > safe)
		request = safe;
	if (request > frameEnd)
		request = frameEnd;
}

void LinkSerial::due()
{
	applyDue();
	settle();
}

/* The machine stops at a waiting message's tick and at the end of its own
 * transfer, so that each is acted on as the chunk after it starts. */
uint64_t LinkSerial::dueAt() const
{
	uint64_t at = RETRO_LINK_UNBOUNDED;

	if (pendingCount_ > 0)
		at = pending_[0].tick;
	if (busy_ && busyUntil_ < at)
		at = busyUntil_;
	return at;
}

void LinkSerial::running(uint64_t tick)
{
	LinkPortBase::running(tick);
	atStart_ = true;
	checkable_ = false;
}

/* Not in the record: doubleSpeed_, which the machine's first report of a
 * chunk sets before anything reads it, and horizon_ and grain_, which ask()
 * sets from fastSeen_ at every advance. The slots of pending_ past
 * pendingCount_ hold messages already applied, so they are written as
 * zeros: two machines in one state save the same bytes however they came
 * to it. */
void LinkSerial::saveState(LinkStateOut &out) const
{
	unsigned i;

	LinkPortBase::saveState(out);
	out.u8(fastSeen_);
	out.u8(peerSb_);
	out.u8(peerArmed_);
	out.u8(lastSb_);
	out.u8(lastFlags_);
	out.u8(published_);
	out.u64(busyUntil_);
	out.u8(busy_);
	out.u8(clockReady_);
	out.u64(clockAt_);
	out.u8(clockSb_);
	out.u8(clockFast_);
	out.u8(pendingCount_);
	for (i = 0; i < PENDING_MAX; i++)
	{
		if (i < pendingCount_)
		{
			out.u64(pending_[i].tick);
			out.u8(pending_[i].type);
			out.u8(pending_[i].sb);
			out.u8(pending_[i].flags);
		}
		else
			out.zeros(8 + 3);
	}
}

void LinkSerial::loadState(LinkStateIn &in)
{
	unsigned i;

	LinkPortBase::loadState(in);
	fastSeen_ = in.u8() != 0;
	peerSb_ = in.u8();
	peerArmed_ = in.u8() != 0;
	lastSb_ = in.u8();
	lastFlags_ = in.u8();
	published_ = in.u8() != 0;
	busyUntil_ = in.u64();
	busy_ = in.u8() != 0;
	clockReady_ = in.u8() != 0;
	clockAt_ = in.u64();
	clockSb_ = in.u8();
	clockFast_ = in.u8() != 0;
	pendingCount_ = in.u8();
	if (pendingCount_ > PENDING_MAX)
		pendingCount_ = PENDING_MAX;
	for (i = 0; i < PENDING_MAX; i++)
	{
		pending_[i].tick = in.u64();
		pending_[i].type = in.u8();
		pending_[i].sb = in.u8();
		pending_[i].flags = in.u8();
	}
}

void LinkSerial::forget()
{
	/* The machine's registers are another state's: its own side of the
	 * wire starts again, as at a reset, and what the peer had said or
	 * clocked was said to the machine that is gone. */
	reset();
	peerSb_ = 0xFF;
	peerArmed_ = false;
	clockReady_ = false;
	pendingCount_ = 0;
}

void LinkSerial::tick(unsigned long cc, bool doubleSpeed, unsigned char sb, unsigned char sc)
{
	if (!inChunk())
		return;

	/* What time brings, a message coming due or a transfer ending, is
	 * acted on as a chunk starts (due()) and not here: only the registers
	 * are the machine's to report. */
	checkable_ = atStart_;
	atStart_ = false;
	doubleSpeed_ = doubleSpeed;
	note(cc, doubleSpeed);
	publish(sb, sc);
}

bool LinkSerial::check(unsigned char out, unsigned char &in, bool &fastCgb)
{
	/* `out` is this machine's own serial register, which the network version
	 * writes back down the socket here. Nothing does with it: the peer read this
	 * machine's register out of the state it published, a horizon ago, because
	 * on a real cable the byte is already sitting in the shift register when the
	 * clock arrives -- there is nothing to ask for and nobody to ask. */
	(void)out;

	if (!checkable_)
		return false;
	checkable_ = false;

	if (peers() < 2 || !clockReady_ || now() < clockAt_)
		return false;

	clockReady_ = false;

	/* A clock nobody answered. The guest was not armed when the peer drove the
	 * line, so this machine took no part in that transfer and must not take part
	 * in it later either: leaving it standing would hand the byte to whatever
	 * the game arms NEXT, one transfer out of step, and a link protocol that
	 * slips a byte never recovers. */
	if (now() > clockAt_ + transferTicks(clockFast_))
		return false;

	in = clockSb_;
	fastCgb = clockFast_;
	fastSeen_ = clockFast_;

	/* The register is being shifted through from here on: said at once,
	 * not when the machine next happens to report. */
	busy_ = true;
	busyUntil_ = now() + transferTicks(clockFast_);
	published_ = false;
	publish(0xFF, 0);
	return true;
}

unsigned char LinkSerial::send(unsigned char data, bool fastCgb)
{
	if (!inChunk() || peers() < 2)
		return 0xFF;

	applyDue();
	fastSeen_ = fastCgb;

	/* Announced a horizon out, not at this instant. The peer may have run as far
	 * as this machine's published horizon and no further, so a transfer stamped
	 * there is one it is guaranteed not to have passed -- which is the whole
	 * reason the promise exists. It also means the peer's half of the transfer
	 * starts a horizon after this machine's; the two interrupts land that far
	 * apart, and no link protocol notices, because on real hardware the gap
	 * between one byte and the next is measured in milliseconds. */
	wire(now() + horizon_, LS_CLOCK, data, fastCgb ? LS_FLAG_FAST : 0);

	busy_ = true;
	busyUntil_ = now() + transferTicks(fastCgb);
	published_ = false;
	publish(0xFF, 0);

	/* What the other end had in its register. An unarmed peer reads back as the
	 * 0xFF of an open line, which is what a Game Boy clocking a cable with
	 * nothing listening on it gets, and what the game is written to expect. */
	return peerArmed_ ? peerSb_ : 0xFF;
}
