#ifndef _LINK_PORT_BASE_H
#define _LINK_PORT_BASE_H

#include <stddef.h>
#include <stdint.h>

#include <libretro.h>

#include "link_ports.h"

/* What the drivers of the link ports keep and do alike: the attachment, the
 * membership last read, the machine's clock in link ticks, and the promise
 * and the stamps given. A driver adds the wire it emulates. */
class LinkPortBase : public LinkPort
{
	public:
		/* Remember the bus. Called once, from retro_init, whether or not the
		 * port is ever attached: the interface stays valid for the life of
		 * the core and holding it costs nothing. */
		void setInterface(const struct retro_link_interface *link, unsigned port);
		bool available() const { return link_ != 0; }

		bool start();
		void stop();

		/* LinkPort. */
		virtual retro_link_port_t *handle() const { return handle_; }
		virtual bool onBus() const { return handle_ != 0 && onBus_; }
		virtual void answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags);
		virtual void running(uint64_t tick);
		virtual void stopped();
		virtual uint64_t dueAt() const { return RETRO_LINK_UNBOUNDED; }

	protected:
		/* `name` starts the port's log lines; `company` is what the ones
		 * that count the bus call the count ("2 machine(s) on the wire"). */
		LinkPortBase(const char *protocol, uint64_t clockRate,
		             const char *name, const char *company);

		/* The port was attached afresh: the driver's own state starts over.
		 * The membership is read after this. */
		virtual void attached() = 0;
		virtual void detached() = 0;

		/* Whether the driver can speak on a bus of `count`. On one it
		 * cannot, the port behaves as if nobody else were there. */
		virtual bool carries(unsigned count) { (void)count; return true; }

		/* The membership was read again; `changed` when the count the
		 * driver sees is not what it was. */
		virtual void membership(bool changed) = 0;

		/* A message the last advance made receivable. `data` is the bus's
		 * own copy, gone when this returns. */
		virtual void received(uint64_t tick, unsigned from,
		                      const unsigned char *data, size_t len) = 0;

		/* The clock was moved up to the machine's tick: act on whatever has
		 * come due. */
		virtual void due() = 0;

		unsigned peers() const { return peers_; }
		int busIndex() const { return index_; }

		uint64_t now() const { return now_; }
		bool inChunk() const { return running_; }

		/* anchor(): the cycle counter reads `cc` at the tick the clock
		 * stands at. note(): it reads `cc` now, so the clock moves on by the
		 * cycles since the last reading. Neither does anything outside a
		 * chunk. */
		void anchor(unsigned long cc);
		void note(unsigned long cc, bool doubleSpeed);

		/* The cycle counter was replaced, as a reset or a loaded state
		 * replaces it: the next reading only anchors. */
		void unanchor() { haveRaw_ = false; }

		/* Keeps the furthest promise given, and returns it. */
		uint64_t promiseTo(uint64_t tick);
		uint64_t promise() const { return promise_; }

		/* Sends to every peer, stamped `tick` or as soon after as the
		 * promise and the earlier stamps allow. */
		void broadcast(uint64_t tick, const unsigned char *msg, size_t len);

	private:
		void refreshPeers();
		void drain();

		const struct retro_link_interface *link_;
		unsigned port_;
		const char *protocol_;
		uint64_t clockRate_;
		const char *name_;
		const char *company_;
		retro_link_port_t *handle_;

		/* The membership as last read: this port's index on its bus, how
		 * many ports the driver takes to be on it, and whether it is on one
		 * at all. */
		int index_;
		unsigned peers_;
		bool onBus_;

		/* This machine's position on the link timeline, in ticks of a clock
		 * running at twice the Game Boy's normal 4.194304 MHz.
		 *
		 * Twice, so that double speed is expressible. gambatte's cycle counter
		 * is not a real-time clock: it advances one tick per CPU cycle and the
		 * CPU cycle itself halves in double speed, which is exactly what makes a
		 * serial transfer's fixed cycle count come out twice as fast on a Game
		 * Boy Color running flat out, the way the hardware does. Handing those
		 * raw cycles to the bus as a fixed rate would have it believe a machine
		 * in double speed is running at twice real time and throttle it to half,
		 * so the doubling is undone here instead.
		 *
		 * A chunk of emulation starts at the tick LinkPorts gives it, and the
		 * cycle counter moves it on from the first reading in the chunk, or
		 * from where the driver anchors it as the chunk starts. Readings
		 * outside a chunk, as while a state is saved, are not emulation and
		 * are ignored. */
		uint64_t now_;
		unsigned long lastRaw_;
		bool haveRaw_;
		bool running_;

		/* The highest promise published, below which nothing is stamped, and
		 * the last stamp, below which nothing later is. */
		uint64_t promise_;
		uint64_t lastStamp_;
};

#endif
