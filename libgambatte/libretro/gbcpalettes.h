/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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

#include <cstring>
#include <algorithm>

namespace {

#define TO5BIT(c8) (((c8) * 0x1F * 2 + 0xFF) / (0xFF*2))
#define PACK15_1(rgb24) (TO5BIT((rgb24) & 0xFF) << 10 | TO5BIT((rgb24) >> 8 & 0xFF) << 5 | TO5BIT((rgb24) >> 16 & 0xFF))
#define PACK15_4(c0, c1, c2, c3) \
	PACK15_1(c0), PACK15_1(c1), PACK15_1(c2), PACK15_1(c3)

//
// Game Boy Palettes
//
static const unsigned short gbdmg[] = { // Original Game Boy
	PACK15_4(0x819A32, 0x61873E, 0x365D48, 0x2A453C),
	PACK15_4(0x819A32, 0x61873E, 0x365D48, 0x2A453C),
	PACK15_4(0x819A32, 0x61873E, 0x365D48, 0x2A453C)
};

static const unsigned short gbpoc[] = { // Game Boy Pocket
	PACK15_4(0x949B8B, 0x7C8870, 0x525F4B, 0x394038),
	PACK15_4(0x949B8B, 0x7C8870, 0x525F4B, 0x394038),
	PACK15_4(0x949B8B, 0x7C8870, 0x525F4B, 0x394038)
};

static const unsigned short gblit[] = { // Game Boy Light
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D),
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D),
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D)
};

#undef PACK15_4
#undef PACK15_1
#undef TO5BIT

struct GbcPaletteEntry { const char *title; const unsigned short *p; };

// Note: These entries must be in alphabetical order
static const GbcPaletteEntry gbcDirPalettes[] = {
	{ "GB - DMG", gbdmg }, // Original Game Boy
	{ "GB - Light", gblit }, // Game Boy Light
	{ "GB - Pocket", gbpoc }, // Game Boy Pocket
};

static inline std::size_t gbcDirPalettesSize() { return (sizeof gbcDirPalettes) / (sizeof gbcDirPalettes[0]); }
static inline const struct GbcPaletteEntry * gbcDirPalettesEnd() { return gbcDirPalettes + gbcDirPalettesSize(); }

struct GbcPaletteEntryLess {
	bool operator()(const GbcPaletteEntry &lhs, const char *const rhstitle) const {
		return std::strcmp(lhs.title, rhstitle) < 0;
	}
};

static const unsigned short * findGbcDirPal(const char *const title) {
	const GbcPaletteEntry *const r = std::lower_bound(gbcDirPalettes, gbcDirPalettesEnd(), title, GbcPaletteEntryLess());
	return r < gbcDirPalettesEnd() && !std::strcmp(r->title, title) ? r->p : 0;
}

}
