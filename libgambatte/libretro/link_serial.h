#ifndef _LINK_SERIAL_H
#define _LINK_SERIAL_H

#include <gambatte.h>
#include <stdint.h>

#include <libretro.h>

#include "link_ports.h"

/* Names the emulated wire. Peers whose ids differ are never joined, which is
 * what keeps a Game Boy link lead out of a Game Boy Advance's socket: mGBA's
 * driver calls its cable "gba-sio" and the two protocols have nothing in
 * common but the shape of the connector. */
#define LINK_SERIAL_PROTOCOL "gb-sio"

/* Twice 4194304, the Game Boy's normal CPU clock. See the note on now_. */
#define LINK_SERIAL_CLOCK_RATE 8388608

/* A Game Boy link cable carried by the frontend's link bus.
 *
 * NetSerial's job done without a socket. The network version works and is the
 * only way two frontends on two machines can be joined, but it pays for that
 * with a blocking socket read in the middle of the emulation loop and with a
 * wire protocol that has no shared clock: whichever end asks first waits for
 * the other, and how long it waits is wall-clock luck. Two cores in ONE
 * process can do better, and have to -- a frontend that draws every handheld
 * into one frame cannot afford one of them stalling on a socket, and a netplay
 * session replaying the same inputs on two peers has to reach the same answer
 * both times.
 *
 * What a core cannot do is find the other core by itself. A frontend that runs
 * several at once generally loads each from its own copy of the shared library,
 * so two Game Boys share no globals whatsoever; the frontend is the only thing
 * they have in common, which is why the bus lives there and is reached through
 * RETRO_ENVIRONMENT_GET_LINK_INTERFACE.
 *
 * WHAT CROSSES. A Game Boy's serial port is two machines and one wire, and
 * unlike a Game Boy Advance's it has no notion of a parent: whichever unit
 * selects the internal clock (SC bit 0) drives the transfer, and that is a
 * choice the GUEST makes, both ends being identical hardware. So there are only
 * two messages. A machine says what its serial register holds and whether it is
 * armed (LS_STATE), and a machine that clocks a transfer says so (LS_CLOCK).
 * Everything else -- which end is master, how long a byte takes, when the
 * interrupt fires -- each end decides for itself out of what it already knows,
 * exactly as the hardware does.
 *
 * WHAT DOES NOT. The four-player adapter, the Super Game Boy's link and the
 * Game Boy Printer are not carried: gambatte emulates none of them, and a bus
 * that quietly joined three Game Boys would produce transfers no real cable
 * could. Anything past a pair is refused, and both machines fall back to the
 * 0xFF an unanswered cable gives.
 */
class LinkSerial : public gambatte::SerialIO, public LinkPort
{
	public:
		LinkSerial();
		virtual ~LinkSerial();

		/* Remember the bus. Called once, from retro_init, whether or not the
		 * cable is ever switched on: the interface stays valid for the life of
		 * the core and holding it costs nothing. */
		void setInterface(const struct retro_link_interface *link, unsigned port);
		bool available() const { return link_ != 0; }

		bool start();
		void stop();

		/* The console's reset button. The cable stays plugged in and the link
		 * keeps its ticks, its promise, the messages received and what the
		 * peer last latched; only this machine's own side of the wire starts
		 * again, with its registers. */
		void reset();

		/* LinkPort. */
		virtual retro_link_port_t *handle() const { return handle_; }
		virtual bool onBus() const { return handle_ != 0 && onBus_; }
		virtual void ask(uint64_t tick, uint64_t frameEnd, uint64_t &safe, uint64_t &request);
		virtual void answered(uint64_t tick, uint64_t grant, uint32_t wakeFlags);
		virtual void running(uint64_t tick);
		virtual void stopped();

		/* SerialIO. */
		virtual bool check(unsigned char out, unsigned char &in, bool &fastCgb);
		virtual unsigned char send(unsigned char data, bool fastCgb);
		virtual void tick(unsigned long cc, bool doubleSpeed,
		                  unsigned char sb, unsigned char sc);

	private:
		/* A message from a peer, held until this machine's clock reaches the
		 * tick it was stamped with.
		 *
		 * Acting on one the moment it arrives is the whole difference between a
		 * link that replays and one that does not: WHEN it arrives is a
		 * wall-clock accident, when its tick comes round is not. */
		struct Pending
		{
			uint64_t tick;
			unsigned char type;
			unsigned char sb;
			unsigned char flags;
		};

		enum { PENDING_MAX = 16 };

		void publish(unsigned char sb, unsigned char sc);
		void drain();
		void queue(const Pending &msg);
		void apply(const Pending &msg);
		void applyDue();
		void settle();
		void refreshPeers();
		void note(unsigned long cc, bool doubleSpeed);
		uint64_t transferTicks(bool fastCgb) const;
		void wire(uint64_t tick, unsigned char type, unsigned char sb, unsigned char flags);

		const struct retro_link_interface *link_;
		unsigned port_;
		retro_link_port_t *handle_;

		int selfId_;
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
		 * cycle counter moves it on from the chunk's first reading. Readings
		 * outside a chunk, as while a state is saved, are not emulation and
		 * are ignored. */
		uint64_t now_;
		unsigned long lastRaw_;
		bool haveRaw_;
		bool doubleSpeed_;
		bool running_;

		/* How far ahead of itself this machine promises not to originate
		 * anything, and how far it asks to run between rendezvous.
		 *
		 * Both follow the transfer speed, because the shortest transfer does: a
		 * normal one is 8192 ticks and a Game Boy Color fast one is 256, and a
		 * horizon that suited the first would swallow the second whole. */
		uint64_t horizon_;
		uint64_t grain_;
		bool fastSeen_;

		/* The highest promise published, below which nothing is stamped, and
		 * the last stamp, below which nothing later is. */
		uint64_t promise_;
		uint64_t lastStamp_;

		/* The peer's serial register, as of the last state it published that
		 * this machine's clock has reached. */
		unsigned char peerSb_;
		bool peerArmed_;

		/* What this machine last told the bus, so a game polling its own serial
		 * registers in a loop does not flood the wire with identical states. */
		unsigned char lastSb_;
		unsigned char lastFlags_;
		bool published_;

		/* A transfer this machine is in the middle of, whether it clocked it or
		 * was clocked: while one runs, the serial register is being shifted
		 * through and holds nothing a peer should read. */
		uint64_t busyUntil_;
		bool busy_;

		/* A transfer a peer has clocked, waiting for this machine's own clock to
		 * reach the moment it starts. */
		bool clockReady_;
		uint64_t clockAt_;
		unsigned char clockSb_;
		bool clockFast_;

		Pending pending_[PENDING_MAX];
		unsigned pendingCount_;
};

#endif
