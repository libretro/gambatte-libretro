//
//   Copyright (C) 2026 by Ryan McClelland <rymcclel@gmail.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef INFRARED_IO_H
#define INFRARED_IO_H

namespace gambatte {

/** The far end of the Game Boy Color's infrared port.
  *
  * RP, 0xFF56, is an LED and a photodiode and nothing else: bit 0 lights the
  * LED, and bit 1 reads 0 while light is falling on the receiver, provided bits
  * 6 and 7 have switched the receiver on. There is no clock, no shift register
  * and no interrupt -- every protocol spoken over it is bit-banged by the game,
  * which times the other machine's flashes against its own timer. So what has
  * to cross is only WHEN the LED changed, and what has to be answered is only
  * whether, at this cycle, anybody's is lit.
  *
  * With nothing installed the port behaves as it always has: the register keeps
  * what was written and the receiver sees the dark.
  */
class InfraredIO
{
	public:
		virtual ~InfraredIO() {}

		/** A run of emulation is starting, with the cycle counter at `cc`.
		  *
		  * The counter is rebased between runs and never inside one, so this is
		  * the one reading an implementation can measure the rest of the run
		  * from. Without it the first access of a run has nothing to be timed
		  * against, and a flash is told apart from a longer flash by tens of
		  * microseconds.
		  *
		  * @param cc cycle counter, in CPU cycles since the last rebase
		  * @param doubleSpeed whether a CPU cycle is currently a half-length one
		  */
		virtual void begin(unsigned long cc, bool doubleSpeed) = 0;

		/** RP has just been written.
		  *
		  * @param rp the value written, whose bit 0 is the LED
		  */
		virtual void write(unsigned long cc, bool doubleSpeed, unsigned char rp) = 0;

		/** Whether light is falling on the receiver at `cc`. Only asked while the
		  * receiver is switched on. */
		virtual bool lit(unsigned long cc, bool doubleSpeed) = 0;

		/** RP as a reset or a loaded state left it. Not a write the guest made at
		  * any moment on the clock, so it comes without one: the cycle counter
		  * has just been replaced and says nothing about where the machine is. */
		virtual void restored(unsigned char rp) = 0;
};

}

#endif
