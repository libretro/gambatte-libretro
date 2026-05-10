/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef STATESAVER_H
#define STATESAVER_H

#include "gbint.h"
#include <string>
#include <stddef.h>

namespace gambatte {

struct SaveState;

class StateSaver {
	StateSaver();
	
public:
	enum { SS_SHIFT = 2 };
	enum { SS_DIV = 1 << 2 };
	enum { SS_WIDTH = 160 >> SS_SHIFT };
	enum { SS_HEIGHT = 144 >> SS_SHIFT };
	
   static void saveState(const SaveState &state, void *data);
   /* size is the size of the input buffer in bytes. The parser
    * uses it to bound every read so a malformed savestate cannot
    * cause an OOB read past the end of the buffer. */
   static bool loadState(SaveState &state, const void *data, size_t size);
   static size_t stateSize(const SaveState &state);
};

}

#endif
