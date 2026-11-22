#ifndef _LINK_PORTS_H
#define _LINK_PORTS_H

#include <stddef.h>
#include <stdint.h>

#include <libretro.h>

/* The fields of a save state record, little-endian. Without a buffer the
 * writer only counts, which is how a record's size is found. */
class LinkStateOut
{
	public:
		explicit LinkStateOut(unsigned char *to) : to_(to), size_(0) {}

		void u8(unsigned value)
		{
			if (to_)
				to_[size_] = (unsigned char)value;
			size_++;
		}

		void u32(uint32_t value)
		{
			unsigned i;

			for (i = 0; i < 4; i++)
				u8((value >> (8 * i)) & 0xFF);
		}

		void u64(uint64_t value)
		{
			u32((uint32_t)value);
			u32((uint32_t)(value >> 32));
		}

		void zeros(size_t count)
		{
			while (count--)
				u8(0);
		}

		size_t size() const { return size_; }

	private:
		unsigned char *to_;
		size_t size_;
};

class LinkStateIn
{
	public:
		explicit LinkStateIn(const unsigned char *from) : from_(from) {}

		unsigned u8() { return *from_++; }

		uint32_t u32()
		{
			uint32_t value = 0;
			unsigned i;

			for (i = 0; i < 4; i++)
				value |= (uint32_t)u8() << (8 * i);
			return value;
		}

		uint64_t u64()
		{
			uint64_t value = u32();

			return value | ((uint64_t)u32() << 32);
		}

		void skip(size_t count) { from_ += count; }

	private:
		const unsigned char *from_;
};

/* One port of the frontend's link bus, driven by LinkPorts. Ticks are link
 * ticks of 1/8388608 s, four to a sample of the machine's sound. */
class LinkPort
{
	public:
		virtual ~LinkPort() {}

		/* NULL when not attached. */
		virtual retro_link_port_t *handle() const = 0;

		/* Whether the port is on a bus, i.e. peers() gives it an index. */
		virtual bool onBus() const = 0;

		/* The promise and request of an advance made with the machine at
		 * `tick`, in a span ending at `frameEnd`. */
		virtual void ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request) = 0;

		/* The advance returned `grant`; takes every message it made
		 * receivable. */
		virtual void answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags) = 0;

		/* The machine runs from `tick` until stopped(). Anything the machine
		 * reports outside such a chunk, as while a state is saved, is not
		 * emulation and must not move the link. */
		virtual void running(uint64_t tick) = 0;
		virtual void stopped() = 0;

		/* The tick at which the port next has something to do that no
		 * report from the machine brings on, such as a received message to
		 * apply; RETRO_LINK_UNBOUNDED when nothing waits. The machine stops
		 * there, and running() of the next chunk does it. */
		virtual uint64_t dueAt() const = 0;

		/* What the port holds at a frame edge, as a record of one size
		 * whatever it holds: a state loaded in another process must go on
		 * as this one would have. */
		virtual void saveState(LinkStateOut &out) const = 0;
		virtual void loadState(LinkStateIn &in) = 0;

		/* A state with no record of the port was loaded. The port keeps
		 * its ticks, and forgets what its peers sent and what it took
		 * from them. */
		virtual void forget() = 0;
};

/* The machine's clock on the link while a port is attached: every retro_run
 * is one Game Boy frame of SPAN ticks, run in chunks no longer than the
 * grants allow, between a frame-start advance and a frame-end publish. */
class LinkPorts
{
	public:
		enum { SAMPLE_TICKS = 4, FRAME_SAMPLES = 35112, SPAN = FRAME_SAMPLES * SAMPLE_TICKS };

		LinkPorts();

		void setInterface(const struct retro_link_interface *link);
		void add(LinkPort *port);
		bool attached() const;

		/* Where the machine stands: the frame-start tick plus every sample it
		 * has run since, the carry from the previous frame included. */
		uint64_t tick() const;

		/* A game is loaded: the clock starts again at 0. */
		void restart();

		void frameStart();

		/* How many samples the machine may run next, at most `wanted`; 0 once
		 * it has reached the end of the span. Advances as often as it needs. */
		unsigned budget(unsigned wanted);
		void produced(unsigned samples);

		void frameEnd();

		/* The clock and every port at a frame edge, for a save state: the
		 * frame-start tick, the carry, then each port's record, or zeros
		 * for a port that is not attached. */
		size_t stateSize() const;
		void saveState(unsigned char *out) const;
		void loadState(const unsigned char *in);

		/* A state taken with no port attached was loaded. */
		void forget();

	private:
		enum { MAX_PORTS = 2 };

		void save(LinkStateOut &out) const;
		size_t recordSize(unsigned i) const;

		void advance(bool frameStart);
		uint64_t reported(unsigned i, uint64_t now) const;

		const struct retro_link_interface *link_;
		LinkPort *ports_[MAX_PORTS];
		uint64_t grant_[MAX_PORTS];
		unsigned count_;

		/* The frame-start tick, the samples run past the previous span's end
		 * and the samples run in this span. */
		uint64_t start_;
		unsigned long carry_;
		unsigned long ran_;
};

#endif
