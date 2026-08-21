//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef SERIAL_IO_H
#define SERIAL_IO_H

namespace gambatte {

class SerialIO
{
	public:
		virtual ~SerialIO() {};

		virtual bool check(unsigned char out, unsigned char& in, bool& fastCgb) = 0;
		virtual unsigned char send(unsigned char data, bool fastCgb) = 0;

		/** Where this machine's clock stands, and what its serial registers
		  * hold. Called at every event boundary, and again after either register
		  * is written, so an implementation that has to place a transfer on a
		  * timeline it shares with another machine can, without having to guess
		  * when the write happened.
		  *
		  * Not pure, and a no-op by default: a serial link over a socket has
		  * nobody to agree a clock with and wants none of this.
		  *
		  * @param cc cycle counter, in CPU cycles since the last rebase
		  * @param doubleSpeed whether a CPU cycle is currently a half-length one
		  * @param sb SB, 0xFF01, the serial transfer register
		  * @param sc SC, 0xFF02, the serial control register
		  */
		virtual void tick(unsigned long cc, bool doubleSpeed,
		                  unsigned char sb, unsigned char sc) {
			(void)cc; (void)doubleSpeed; (void)sb; (void)sc;
		}
};

}

#endif
