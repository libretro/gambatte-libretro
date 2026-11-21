#ifndef _LINK_INFRARED_H
#define _LINK_INFRARED_H

#include <gambatte.h>
#include <stdint.h>

#include <libretro.h>

#include "link_ports.h"

/* Names the emulated wire, and keeps it apart from the link cable's: the two
 * share a bus implementation and nothing else, and a core that attached its
 * infrared port to a serial lead would be refused rather than heard. */
#define LINK_INFRARED_PROTOCOL "gbc-ir"

/* Twice 4194304, the Game Boy's normal CPU clock. See LinkSerial::now_. */
#define LINK_INFRARED_CLOCK_RATE 8388608

/* A Game Boy Color's infrared port carried by the frontend's link bus.
 *
 * The same bus the link cable rides, on a port of its own, because it is a
 * different thing joining different machines: a lead goes where it is pushed
 * in, and light goes wherever the top of the handheld is pointing. What the
 * port is joined to is the frontend's decision here as there -- two units
 * facing each other is its to judge -- and with nothing in view the bus grants
 * without bound and the receiver sees the dark it always saw.
 *
 * WHAT CROSSES. One message: this machine's LED changed, and when. There is no
 * byte, no clock and no handshake to carry, because the hardware has none --
 * RP is a lamp and an eye, and every protocol over it is the game flashing the
 * one and timing the other against its own timer. Pokemon tells a 0 from a 1 by
 * whether the gap between two flashes is 183 or 366 microseconds, so the one
 * thing this must not do is move two edges by different amounts.
 *
 * Which is why every edge is stamped the same distance ahead. The delay is the
 * promise the bus needs anyway, and a flash arriving fifteen microseconds late
 * is still exactly as long as it was sent.
 *
 * HOW MANY. The receiver reads lit while the LED of any unit on the bus is,
 * however many the frontend joins.
 *
 * WHAT DOES NOT. The receiver's own behaviour -- it warms up, and it stops
 * seeing a light that stays on -- is not modelled, nor is a unit seeing its own
 * LED. Neither is anything that is not another Game Boy Color: a television
 * remote, a Pocket Pikachu, or the port a HuC1 or HuC3 cartridge carries.
 */
class LinkInfrared : public gambatte::InfraredIO, public LinkPort
{
	public:
		LinkInfrared();
		virtual ~LinkInfrared();

		/* Remember the bus. Called once, from retro_init. */
		void setInterface(const struct retro_link_interface *link, unsigned port);

		bool start();
		void stop();

		/* LinkPort. */
		virtual retro_link_port_t *handle() const { return handle_; }
		virtual bool onBus() const { return handle_ != 0 && onBus_; }
		virtual void ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request);
		virtual void answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags);
		virtual void running(uint64_t tick);
		virtual void stopped();

		/* InfraredIO. */
		virtual void begin(unsigned long cc, bool doubleSpeed);
		virtual void write(unsigned long cc, bool doubleSpeed, unsigned char rp);
		virtual bool lit(unsigned long cc, bool doubleSpeed);
		virtual void restored(unsigned char rp);

	private:
		/* A peer's LED changing, held until this machine's clock reaches the
		 * tick it was stamped with. WHEN the message arrived is a wall-clock
		 * accident; when its tick comes round is not. */
		struct Edge
		{
			uint64_t tick;
			unsigned char from;
			unsigned char on;
		};

		/* Room for a peer toggling its LED as fast as a Game Boy can write a
		 * register, for as long as an idle horizon lasts. */
		enum { EDGE_MAX = 1024 };

		void note(unsigned long cc, bool doubleSpeed);
		void tell();
		void wire(bool on);
		void drain();
		void queue(const Edge &edge);
		void apply(const Edge &edge);
		void applyDue();
		void refreshPeers();
		void forgetPeers();
		bool hot() const;

		const struct retro_link_interface *link_;
		unsigned port_;
		retro_link_port_t *handle_;

		int selfId_;
		unsigned peers_;
		bool onBus_;

		/* This machine's position on the link timeline, in ticks of a clock
		 * running at twice the Game Boy's normal 4.194304 MHz -- see
		 * LinkSerial::now_ for why twice. A chunk of emulation starts at the
		 * tick LinkPorts gives it, and the cycle counter moves it on from the
		 * reading begin() takes as the chunk starts. Outside a chunk nothing
		 * moves it. */
		uint64_t now_;
		unsigned long lastRaw_;
		bool haveRaw_;
		bool running_;

		/* How far ahead this machine promises not to flash, and the furthest
		 * tick it has promised that for.
		 *
		 * Short while the port is in use and long while it is not, because a
		 * horizon is paid for in rendezvous and two handhelds left facing each
		 * other on a shelf should cost nearly nothing. The promise outlives the
		 * horizon that made it: one given while the port was idle still binds
		 * the first flash after it wakes. */
		uint64_t horizon_;
		uint64_t promise_;

		/* RP as last written, the LED out of it, and the LED as the peers
		 * were last told it is. The two differ when RP changed with no chunk
		 * running, as a reset or a loaded state changes it, or with nobody in
		 * view; the peers are told as the next chunk starts. */
		unsigned char rp_;
		bool led_;
		bool told_;

		/* How far ahead an edge is stamped, and the edges that decide it.
		 *
		 * The latency, unless a promise made while idle reaches further -- and
		 * then it STAYS further for the rest of that burst, because the next
		 * edge has to be moved by exactly as much as this one or the flash
		 * between them changes length. It goes back only across a silence no
		 * protocol times. */
		uint64_t delay_;
		uint64_t lastEdge_;
		uint64_t lastStamp_;
		bool edgeSeen_;

		/* Which peers' LEDs are lit, one bit a peer, as of this machine's clock. */
		unsigned lit_;

		Edge edges_[EDGE_MAX];
		unsigned edgeHead_;
		unsigned edgeCount_;
		bool overflowed_;
};

#endif
