#include "link_port_base.h"

#include "gambatte_log.h"

LinkPortBase::LinkPortBase(const char *protocol, uint64_t clockRate,
                           const char *name, const char *company)
	: link_(0)
	, port_(0)
	, protocol_(protocol)
	, clockRate_(clockRate)
	, name_(name)
	, company_(company)
	, handle_(0)
	, index_(0)
	, peers_(0)
	, onBus_(false)
	, now_(0)
	, lastRaw_(0)
	, haveRaw_(false)
	, running_(false)
	, promise_(0)
	, lastStamp_(0)
{
}

void LinkPortBase::setInterface(const struct retro_link_interface *link, unsigned port)
{
	link_ = link;
	port_ = port;
}

bool LinkPortBase::start()
{
	if (handle_)
		return true;
	if (!link_)
		return false;

	handle_ = link_->attach(port_, protocol_, clockRate_);
	if (!handle_)
	{
		gambatte_log(RETRO_LOG_WARN, "%s: the frontend refused port %u\n", name_, port_);
		return false;
	}

	/* A port attached afresh has promised and stamped nothing. Its clock
	 * takes the machine's tick from LinkPorts with the first advance. */
	now_ = 0;
	haveRaw_ = false;
	running_ = false;
	promise_ = 0;
	lastStamp_ = 0;
	attached();
	refreshPeers();
	gambatte_log(RETRO_LOG_INFO, "%s: attached to port %u, %u %s\n",
	             name_, port_, peers_, company_);
	return true;
}

void LinkPortBase::stop()
{
	if (!handle_)
		return;

	link_->detach(handle_);
	handle_ = 0;
	peers_ = 0;
	onBus_ = false;
	index_ = 0;
	running_ = false;
	detached();
}

void LinkPortBase::refreshPeers()
{
	unsigned was = peers_;
	unsigned count = 0;
	int id;

	id = link_->peers(handle_, &count);
	onBus_ = id >= 0;
	if (id < 0 || !carries(count))
	{
		index_ = 0;
		peers_ = 0;
	}
	else
	{
		index_ = id;
		peers_ = count;
	}

	membership(peers_ != was);
	if (peers_ != was)
		gambatte_log(RETRO_LOG_INFO, "%s: %u %s, this one is %d\n",
		             name_, peers_, company_, index_);
}

void LinkPortBase::anchor(unsigned long cc)
{
	if (!running_)
		return;
	lastRaw_ = cc;
	haveRaw_ = true;
}

void LinkPortBase::note(unsigned long cc, bool doubleSpeed)
{
	if (!running_)
		return;

	/* One tick a cycle in double speed and two outside it. A step backwards
	 * is bookkeeping rather than time travel: gambatte rebases its cycle
	 * counter whenever it passes 2^31. */
	if (haveRaw_ && cc > lastRaw_)
		now_ += (uint64_t)(cc - lastRaw_) << (doubleSpeed ? 0 : 1);

	lastRaw_ = cc;
	haveRaw_ = true;
}

uint64_t LinkPortBase::promiseTo(uint64_t tick)
{
	if (promise_ < tick)
		promise_ = tick;
	return promise_;
}

void LinkPortBase::broadcast(uint64_t tick, const unsigned char *msg, size_t len)
{
	/* Never before the promise this port has published, which peers may
	 * already have run up to, nor before an earlier message. */
	if (tick < promise_)
		tick = promise_;
	if (tick < lastStamp_)
		tick = lastStamp_;
	lastStamp_ = tick;

	link_->send_msg(handle_, tick, RETRO_LINK_BROADCAST, msg, len);
}

void LinkPortBase::drain()
{
	const unsigned char *buf;
	uint64_t tick;
	unsigned from;
	size_t len;

	/* Every message that can be handed out is taken, one the driver does
	 * not speak included, so none holds up those behind it. */
	while ((buf = (const unsigned char*)link_->peek(handle_, &tick, &from, &len)) != 0)
	{
		received(tick, from, buf, len);
		if (!link_->recv_msg(handle_, &tick, &from, 0, &len))
			break;
	}
}

void LinkPortBase::answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags)
{
	(void)grant;

	if (wakeFlags & RETRO_LINK_WAKE_TOPOLOGY)
		refreshPeers();
	drain();

	if (now_ < tick)
		now_ = tick;
	due();
}

void LinkPortBase::running(uint64_t tick)
{
	running_ = true;
	haveRaw_ = false;
	if (now_ < tick)
		now_ = tick;
	due();
}

void LinkPortBase::stopped()
{
	running_ = false;
	haveRaw_ = false;
}
