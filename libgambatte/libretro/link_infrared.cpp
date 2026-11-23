#include "link_infrared.h"

#include <string.h>

#include "gambatte_log.h"

/* How late a flash arrives, and how often two units in use rendezvous: 15
 * microseconds, which is 64 cycles of an undoubled CPU.
 *
 * Light takes no time and this is not light, so it is as small as it can be
 * afforded. Every edge is late by the same amount, which no receiver can see
 * in the length of a flash; what a game CAN see is its partner's answers
 * arriving twice this late, once for each direction. The protocols leave room
 * for that because the hardware needed it -- Pokemon waits two milliseconds
 * before it answers and eight before it gives up.
 *
 * What it must not be is as long as a flash, or as long as a receiver takes to
 * look again. Two machines run by one frontend are in step, frame for frame:
 * each takes its interrupts and turns its polling loop at the moment the other
 * does, so a delay moves every flash against that work by the same amount,
 * every time. Pokemon Pinball opens with a 112-microsecond flash, and a flash
 * 122 microseconds late falls inside the interrupt the receiver takes as the
 * sender's flash ends. Super Mario Bros. Deluxe, at double speed, sends
 * 56-microsecond flashes and looks for one every 57; in step it stops catching
 * the first once it is 26 microseconds late.
 *
 * Smaller is a rendezvous between two machines that much more often. */
#define IR_LATENCY ((uint64_t)128)

/* The horizon of a port nobody is using, about two milliseconds. A promise this
 * long is still standing when the port wakes, and the first burst after that
 * goes out as late as the promise reaches, up to this many ticks, every edge
 * alike. That is far more than two machines in step allow (above), so a
 * protocol that flashes first from a port that has not been listening can fail
 * here. Both ends of a symmetric protocol listen before either flashes, which
 * keeps their ports in use. */
#define IR_IDLE_HORIZON ((uint64_t)16384)

/* How long after its last flash a port still counts as in use: an eighth of a
 * second. A unit that only ever transmits never switches its receiver on, so
 * the LED itself has to keep the horizon short between one flash and the next.
 * Also the silence across which an edge may go back to the ordinary latency. */
#define IR_HOLD ((uint64_t)1048576)

/* Bus message, packed by hand: see the note on LS_MSG_SIZE in link_serial.cpp. */
enum
{
	IR_LEVEL = 1
};

#define IR_MSG_SIZE 8

LinkInfrared::LinkInfrared()
	: LinkPortBase(LINK_INFRARED_PROTOCOL, LINK_INFRARED_CLOCK_RATE,
	               "Infrared", "unit(s) in view")
	, horizon_(IR_IDLE_HORIZON)
	, rp_(0)
	, led_(false)
	, told_(false)
	, delay_(IR_LATENCY)
	, lastEdge_(0)
	, edgeSeen_(false)
	, lit_(0)
	, edgeHead_(0)
	, edgeCount_(0)
	, overflowed_(false)
{
	memset(edges_, 0, sizeof(edges_));
}

LinkInfrared::~LinkInfrared()
{
	stop();
}

void LinkInfrared::attached()
{
	/* The LED is taken as dark and the receiver as off until RP is next
	 * written or restored. */
	rp_ = 0;
	led_ = false;
	delay_ = IR_LATENCY;
	edgeSeen_ = false;
	overflowed_ = false;
}

void LinkInfrared::detached()
{
	forgetPeers();
}

void LinkInfrared::forgetPeers()
{
	lit_ = 0;
	edgeHead_ = 0;
	edgeCount_ = 0;
}

bool LinkInfrared::hot() const
{
	/* Either half of the port: the receiver switched on, or the LED lit. And
	 * for a while after a flash, because a unit that only transmits writes
	 * nothing but that one bit and is dark between flashes. */
	if (rp_ & 0xC1)
		return true;
	return edgeSeen_ && now() - lastEdge_ < IR_HOLD;
}

void LinkInfrared::begin(unsigned long cc, bool doubleSpeed)
{
	(void)doubleSpeed;

	anchor(cc);
}

void LinkInfrared::wire(bool on)
{
	unsigned char msg[IR_MSG_SIZE];

	/* The promise made to the bus has to be kept, and it may reach further
	 * than the latency if it was made while the port was idle. Then the delay
	 * grows to meet it and stays grown: this edge and the next must be moved
	 * by the same amount. */
	if (now() + delay_ < promise())
		delay_ = promise() - now();

	memset(msg, 0, sizeof(msg));
	msg[0] = IR_LEVEL;
	msg[1] = (unsigned char)busIndex();
	msg[2] = on ? 1 : 0;
	broadcast(now() + delay_, msg, sizeof(msg));
}

/* Sends the LED's state when it is not what the peers last heard. Only inside
 * a chunk: nothing may be sent outside retro_run, and an edge needs the
 * machine's clock. */
void LinkInfrared::tell()
{
	if (!inChunk() || peers() < 2 || told_ == led_)
		return;

	told_ = led_;
	wire(led_);
}

void LinkInfrared::write(unsigned long cc, bool doubleSpeed, unsigned char rp)
{
	bool on = (rp & 1) != 0;

	note(cc, doubleSpeed);
	rp_ = rp;

	if (on != led_)
	{
		led_ = on;

		/* A new burst, if the last edge was long enough ago that nothing can
		 * be timing the gap between the two. */
		if (!edgeSeen_ || now() - lastEdge_ >= IR_HOLD)
			delay_ = IR_LATENCY;
		edgeSeen_ = true;
		lastEdge_ = now();
	}

	tell();
}

void LinkInfrared::restored(unsigned char rp)
{
	/* No clock, and no chunk running: the cycle counter has just been
	 * replaced. The machine stands where the last chunk left it, and the
	 * peers hear of the LED as the next one starts. */
	unanchor();
	rp_ = rp;
	led_ = (rp & 1) != 0;
}

bool LinkInfrared::lit(unsigned long cc, bool doubleSpeed)
{
	if (peers() < 2)
		return false;

	note(cc, doubleSpeed);
	applyDue();
	return lit_ != 0;
}

void LinkInfrared::apply(const Edge &edge)
{
	unsigned const bit = 1u << (edge.from < 31 ? edge.from : 31);

	if (edge.on)
		lit_ |= bit;
	else
		lit_ &= ~bit;
}

void LinkInfrared::queue(const Edge &edge)
{
	if (edgeCount_ >= EDGE_MAX)
	{
		/* A peer cannot have flashed this often inside one horizon, so this is
		 * the reasoning being wrong rather than the buffer being small. The
		 * oldest edge is acted on early to make room, which keeps the LED's
		 * final state right at the cost of when it got there, and it is said
		 * once. How many messages have arrived by a rendezvous is fixed by the
		 * grants, so even this is the same on two peers replaying it. */
		if (!overflowed_)
		{
			overflowed_ = true;
			gambatte_log(RETRO_LOG_WARN,
				"Infrared: more edges waiting than the horizon should allow;"
				" acting on the oldest early\n");
		}
		apply(edges_[edgeHead_]);
		edgeHead_ = (edgeHead_ + 1) % EDGE_MAX;
		--edgeCount_;
	}
	edges_[(edgeHead_ + edgeCount_) % EDGE_MAX] = edge;
	++edgeCount_;
}

void LinkInfrared::applyDue()
{
	while (edgeCount_ > 0 && edges_[edgeHead_].tick <= now())
	{
		apply(edges_[edgeHead_]);
		edgeHead_ = (edgeHead_ + 1) % EDGE_MAX;
		--edgeCount_;
	}
}

void LinkInfrared::received(uint64_t tick, unsigned from, const unsigned char *data, size_t len)
{
	Edge edge;

	/* A message this driver does not speak is dropped. */
	if (len != IR_MSG_SIZE || data[0] != IR_LEVEL)
		return;

	edge.tick = tick;
	edge.from = (unsigned char)(from < 255 ? from : 255);
	edge.on = data[2] & 1;
	queue(edge);
}

void LinkInfrared::membership()
{
	/* Forget who was lit, and say so again if this machine is. What was in
	 * view a moment ago is not evidence about what is in view now, and a unit
	 * that came into view since has never been told anything. The count may
	 * not have changed -- one unit swapped for another leaves it at two -- so
	 * the count alone cannot be trusted to say so. */
	forgetPeers();
	told_ = false;
}

void LinkInfrared::ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request)
{
	horizon_ = hot() ? IR_LATENCY : IR_IDLE_HORIZON;

	safe = promiseTo(tick + horizon_);

	/* Never more than its own horizon, which is what keeps two units from
	 * each waiting for the other to go first: whichever has promised the
	 * least is asking for no more than everybody has already promised it. */
	request = tick + horizon_;
	if (request > frameEnd)
		request = frameEnd;
}

void LinkInfrared::due()
{
	applyDue();
}

void LinkInfrared::running(uint64_t tick)
{
	LinkPortBase::running(tick);
	tell();
}

/* The waiting edges are written oldest first and field by field, and the
 * rest of the ring as zeros: where the ring's head stands, what an Edge's
 * padding holds and what the free slots last held are not the port's
 * state, and two machines in one state save the same bytes. horizon_ is
 * not in the record: ask() sets it at every advance. */
void LinkInfrared::saveState(LinkStateOut &out) const
{
	unsigned i;

	LinkPortBase::saveState(out);
	out.u8(rp_);
	out.u8(led_);
	out.u8(told_);
	out.u64(delay_);
	out.u64(lastEdge_);
	out.u8(edgeSeen_);
	out.u32(lit_);
	out.u32(edgeCount_);
	for (i = 0; i < EDGE_MAX; i++)
	{
		if (i < edgeCount_)
		{
			const Edge &edge = edges_[(edgeHead_ + i) % EDGE_MAX];

			out.u64(edge.tick);
			out.u8(edge.from);
			out.u8(edge.on);
		}
		else
			out.zeros(8 + 2);
	}
}

void LinkInfrared::loadState(LinkStateIn &in)
{
	unsigned i;

	LinkPortBase::loadState(in);
	rp_ = in.u8();
	led_ = in.u8() != 0;
	told_ = in.u8() != 0;
	delay_ = in.u64();
	/* It grows only to meet a promise, at most the idle horizon ahead. */
	if (delay_ > IR_IDLE_HORIZON)
		delay_ = IR_IDLE_HORIZON;
	lastEdge_ = in.u64();
	edgeSeen_ = in.u8() != 0;
	lit_ = in.u32();
	edgeCount_ = in.u32();
	if (edgeCount_ > EDGE_MAX)
		edgeCount_ = EDGE_MAX;
	edgeHead_ = 0;
	for (i = 0; i < EDGE_MAX; i++)
	{
		edges_[i].tick = in.u64();
		edges_[i].from = in.u8();
		edges_[i].on = in.u8();
	}
}

void LinkInfrared::forget()
{
	/* restored() has taken the LED from the state's RP, and the peers
	 * hear of it as the next chunk starts. */
	forgetPeers();
}
