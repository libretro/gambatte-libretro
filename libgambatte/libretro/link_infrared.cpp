#include "link_infrared.h"

#include <string.h>

#include "gambatte_log.h"

/* How late a flash arrives, and how often two units in use rendezvous: 122
 * microseconds, which is 512 cycles of an undoubled CPU.
 *
 * Light takes no time and this is not light, so it is as small as it can be
 * afforded. Every edge is late by the same amount, which no receiver can see;
 * what a game CAN see is its partner's answers arriving twice this late, once
 * for each direction. The protocols leave room for that because the hardware
 * needed it -- Pokemon waits two milliseconds before it answers and eight
 * before it gives up -- and a game polling the port from a timer interrupt
 * cannot resolve much under thirty microseconds to begin with.
 *
 * Smaller is a rendezvous between two machines that much more often. */
#define IR_LATENCY ((uint64_t)1024)

/* The horizon of a port nobody is using, about two milliseconds. A promise this
 * long is still standing when the port wakes, and is what the first flash after
 * that has to wait out -- once, at the head of a conversation, where every
 * protocol is waiting for a button to be pressed anyway. */
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
	: link_(0)
	, port_(0)
	, handle_(0)
	, selfId_(0)
	, peers_(0)
	, onBus_(false)
	, now_(0)
	, lastRaw_(0)
	, haveRaw_(false)
	, running_(false)
	, horizon_(IR_IDLE_HORIZON)
	, promise_(0)
	, rp_(0)
	, led_(false)
	, told_(false)
	, delay_(IR_LATENCY)
	, lastEdge_(0)
	, lastStamp_(0)
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

void LinkInfrared::setInterface(const struct retro_link_interface *link, unsigned port)
{
	link_ = link;
	port_ = port;
}

bool LinkInfrared::start()
{
	if (handle_)
		return true;
	if (!link_)
		return false;

	handle_ = link_->attach(port_, LINK_INFRARED_PROTOCOL, LINK_INFRARED_CLOCK_RATE);
	if (!handle_)
	{
		gambatte_log(RETRO_LOG_WARN, "Infrared: the frontend refused port %u\n", port_);
		return false;
	}

	/* A port attached afresh starts a timeline of its own, with its LED
	 * dark until the game next writes RP. */
	now_ = 0;
	haveRaw_ = false;
	running_ = false;
	promise_ = 0;
	rp_ = 0;
	led_ = false;
	delay_ = IR_LATENCY;
	lastStamp_ = 0;
	edgeSeen_ = false;
	overflowed_ = false;
	refreshPeers();
	gambatte_log(RETRO_LOG_INFO, "Infrared: attached to port %u, %u unit(s) in view\n",
	             port_, peers_);
	return true;
}

void LinkInfrared::stop()
{
	if (!handle_)
		return;

	link_->detach(handle_);
	handle_ = 0;
	peers_ = 0;
	onBus_ = false;
	selfId_ = 0;
	running_ = false;
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
	return edgeSeen_ && now_ - lastEdge_ < IR_HOLD;
}

void LinkInfrared::begin(unsigned long cc, bool doubleSpeed)
{
	(void)doubleSpeed;

	if (!running_)
		return;
	lastRaw_ = cc;
	haveRaw_ = true;
}

void LinkInfrared::note(unsigned long cc, bool doubleSpeed)
{
	/* One tick a cycle in double speed and two outside it. A step backwards is
	 * the counter having been rebased, which happens between chunks and which
	 * begin() covers: it is given up rather than guessed at. */
	if (haveRaw_ && cc > lastRaw_)
		now_ += (uint64_t)(cc - lastRaw_) << (doubleSpeed ? 0 : 1);

	lastRaw_ = cc;
	haveRaw_ = true;
}

void LinkInfrared::wire(bool on)
{
	unsigned char msg[IR_MSG_SIZE];
	uint64_t stamp;

	/* The promise made to the bus has to be kept, and it may reach further
	 * than the latency if it was made while the port was idle. Then the delay
	 * grows to meet it and stays grown: this edge and the next must be moved
	 * by the same amount. */
	stamp = now_ + delay_;
	if (stamp < promise_)
	{
		delay_ = promise_ - now_;
		stamp = promise_;
	}
	if (stamp < lastStamp_)
		stamp = lastStamp_;
	lastStamp_ = stamp;

	memset(msg, 0, sizeof(msg));
	msg[0] = IR_LEVEL;
	msg[1] = (unsigned char)selfId_;
	msg[2] = on ? 1 : 0;
	link_->send_msg(handle_, stamp, RETRO_LINK_BROADCAST, msg, sizeof(msg));
}

/* Sends the LED's state when it is not what the peers last heard. Only inside
 * a chunk: nothing may be sent outside retro_run, and an edge needs the
 * machine's clock. */
void LinkInfrared::tell()
{
	if (!handle_ || !running_ || peers_ < 2 || told_ == led_)
		return;

	told_ = led_;
	wire(led_);
}

void LinkInfrared::write(unsigned long cc, bool doubleSpeed, unsigned char rp)
{
	bool on = (rp & 1) != 0;

	if (running_)
		note(cc, doubleSpeed);
	rp_ = rp;

	if (on != led_)
	{
		led_ = on;

		/* A new burst, if the last edge was long enough ago that nothing can
		 * be timing the gap between the two. */
		if (!edgeSeen_ || now_ - lastEdge_ >= IR_HOLD)
			delay_ = IR_LATENCY;
		edgeSeen_ = true;
		lastEdge_ = now_;
	}

	tell();
}

void LinkInfrared::restored(unsigned char rp)
{
	/* No clock, and no chunk running: the cycle counter has just been
	 * replaced. The machine stands where the last chunk left it, and the
	 * peers hear of the LED as the next one starts. */
	haveRaw_ = false;
	rp_ = rp;
	led_ = (rp & 1) != 0;
}

bool LinkInfrared::lit(unsigned long cc, bool doubleSpeed)
{
	if (!handle_ || peers_ < 2)
		return false;

	if (running_)
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
	while (edgeCount_ > 0 && edges_[edgeHead_].tick <= now_)
	{
		apply(edges_[edgeHead_]);
		edgeHead_ = (edgeHead_ + 1) % EDGE_MAX;
		--edgeCount_;
	}
}

void LinkInfrared::drain()
{
	const unsigned char *buf;
	uint64_t tick;
	unsigned from;
	size_t len;

	/* Every message that can be handed out is taken, one this driver does
	 * not speak included, so none holds up those behind it. */
	while ((buf = (const unsigned char*)link_->peek(handle_, &tick, &from, &len)) != 0)
	{
		if (len == IR_MSG_SIZE && buf[0] == IR_LEVEL)
		{
			Edge edge;
			edge.tick = tick;
			edge.from = (unsigned char)(from < 255 ? from : 255);
			edge.on = buf[2] & 1;
			queue(edge);
		}
		if (!link_->recv_msg(handle_, &tick, &from, 0, &len))
			break;
	}
}

void LinkInfrared::refreshPeers()
{
	unsigned was = peers_;
	unsigned count = 0;
	int id;

	id = link_->peers(handle_, &count);
	onBus_ = id >= 0;
	if (id < 0)
	{
		selfId_ = 0;
		peers_ = 0;
	}
	else
	{
		selfId_ = id;
		peers_ = count;
	}

	/* Forget who was lit, and say so again if this machine is. What was in
	 * view a moment ago is not evidence about what is in view now, and a unit
	 * that came into view since has never been told anything. The count may
	 * not have changed -- one unit swapped for another leaves it at two -- so
	 * the count alone cannot be trusted to say so. */
	forgetPeers();
	told_ = false;

	if (peers_ != was)
		gambatte_log(RETRO_LOG_INFO, "Infrared: %u unit(s) in view, this one is %d\n",
		             peers_, selfId_);
}

void LinkInfrared::ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request)
{
	horizon_ = hot() ? IR_LATENCY : IR_IDLE_HORIZON;

	if (promise_ < tick + horizon_)
		promise_ = tick + horizon_;
	safe = promise_;

	/* Never more than its own horizon, which is what keeps two units from
	 * each waiting for the other to go first: whichever has promised the
	 * least is asking for no more than everybody has already promised it. */
	request = tick + horizon_;
	if (request > frameEnd)
		request = frameEnd;
}

void LinkInfrared::answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags)
{
	(void)grant;

	if (wakeFlags & RETRO_LINK_WAKE_TOPOLOGY)
		refreshPeers();
	drain();

	if (now_ < tick)
		now_ = tick;
	applyDue();
}

void LinkInfrared::running(uint64_t tick)
{
	running_ = true;
	haveRaw_ = false;
	if (now_ < tick)
		now_ = tick;
	applyDue();
	tell();
}

void LinkInfrared::stopped()
{
	running_ = false;
	haveRaw_ = false;
}
