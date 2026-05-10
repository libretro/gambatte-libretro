/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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
#ifndef INITSTATE_H
#define INITSTATE_H

namespace gambatte {
/* clearSram defaults to true to preserve the historical behavior
 * for callers that load a fresh cart (uninitialized SRAM should
 * read as 0xFF). Pass false from a soft-reset path so battery-
 * backed cartridge RAM survives the reset, matching real
 * hardware behavior. */
void setInitState(struct SaveState &state, bool cgb, bool gbaCgbMode, bool clearSram = true);
}

#endif
