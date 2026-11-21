#include "link_ports.h"

LinkPorts::LinkPorts()
	: link_(0)
	, count_(0)
	, start_(0)
	, carry_(0)
	, ran_(0)
{
	unsigned i;

	for (i = 0; i < MAX_PORTS; i++)
	{
		ports_[i] = 0;
		grant_[i] = RETRO_LINK_UNBOUNDED;
	}
}

void LinkPorts::setInterface(const struct retro_link_interface *link)
{
	link_ = link;
}

void LinkPorts::add(LinkPort *port)
{
	unsigned i;

	for (i = 0; i < count_; i++)
		if (ports_[i] == port)
			return;
	if (count_ < MAX_PORTS)
		ports_[count_++] = port;
}

bool LinkPorts::attached() const
{
	unsigned i;

	for (i = 0; i < count_; i++)
		if (ports_[i]->handle())
			return true;
	return false;
}

uint64_t LinkPorts::tick() const
{
	return start_ + (uint64_t)(carry_ + ran_) * SAMPLE_TICKS;
}

void LinkPorts::restart()
{
	unsigned i;

	start_ = 0;
	carry_ = 0;
	ran_ = 0;
	for (i = 0; i < MAX_PORTS; i++)
		grant_[i] = RETRO_LINK_UNBOUNDED;
}

/* An instruction run past a grant or the span's end counts as stopping there,
 * so no port is told it stands past either. */
uint64_t LinkPorts::reported(unsigned i, uint64_t now) const
{
	uint64_t end = start_ + SPAN;
	uint64_t local = now < end ? now : end;

	if (grant_[i] != RETRO_LINK_UNBOUNDED && grant_[i] < local)
		local = grant_[i];
	return local;
}

void LinkPorts::advance(bool frameStart)
{
	struct retro_link_wait waits[MAX_PORTS];
	unsigned listed[MAX_PORTS];
	uint64_t now = tick();
	uint64_t end = start_ + SPAN;
	uint64_t earliest = RETRO_LINK_UNBOUNDED;
	unsigned n = 0;
	unsigned i;

	for (i = 0; i < count_; i++)
	{
		struct retro_link_wait *w = &waits[n];

		if (!(w->handle = ports_[i]->handle()))
			continue;
		w->local_tick = frameStart ? start_ : reported(i, now);
		ports_[i]->ask(now, end, w->safe_tick, w->request_tick);
		w->grant = RETRO_LINK_UNBOUNDED;
		/* Any value but RETRO_LINK_WAKE_NONE asks for reports. */
		w->wake_flags = RETRO_LINK_WAKE_TOPOLOGY;
		if (ports_[i]->onBus() && w->safe_tick < earliest)
			earliest = w->safe_tick;
		listed[n++] = i;
	}

	if (n == 1)
		waits[0].grant = link_->advance(waits[0].handle, waits[0].local_tick,
				waits[0].safe_tick, waits[0].request_tick, &waits[0].wake_flags);
	else if (n > 1)
	{
		for (i = 0; i < n; i++)
			if (waits[i].request_tick > earliest)
				waits[i].request_tick = earliest > waits[i].local_tick
					? earliest : waits[i].local_tick;
		link_->advance_multi(waits, n);
	}

	for (i = 0; i < n; i++)
	{
		grant_[listed[i]] = waits[i].grant;
		ports_[listed[i]]->answered(now, waits[i].grant, waits[i].wake_flags);
	}
}

void LinkPorts::frameStart()
{
	ran_ = 0;
	advance(true);
}

unsigned LinkPorts::budget(unsigned wanted)
{
	uint64_t end = start_ + SPAN;

	for (;;)
	{
		uint64_t now = tick();
		uint64_t limit = end;
		unsigned i;

		for (i = 0; i < count_; i++)
			if (ports_[i]->handle() && grant_[i] != RETRO_LINK_UNBOUNDED && grant_[i] < limit)
				limit = grant_[i];

		if (limit >= now + SAMPLE_TICKS)
		{
			uint64_t fit = (limit - now) / SAMPLE_TICKS;
			unsigned samples = fit < wanted ? (unsigned)fit : wanted;

			for (i = 0; i < count_; i++)
				if (ports_[i]->handle())
					ports_[i]->running(now);
			return samples;
		}
		if (now >= end && limit == end)
			return 0;
		advance(false);
	}
}

void LinkPorts::produced(unsigned samples)
{
	unsigned i;

	ran_ += samples;
	for (i = 0; i < count_; i++)
		if (ports_[i]->handle())
			ports_[i]->stopped();
}

/* The frame-end publish: budget() has run the machine to the span's end and
 * every grant to it, so each port reports the end as its tick and asks for
 * nothing more. What an instruction ran past the end is the next span's. */
void LinkPorts::frameEnd()
{
	advance(false);
	carry_ = carry_ + ran_ - FRAME_SAMPLES;
	ran_ = 0;
	start_ += SPAN;
}
