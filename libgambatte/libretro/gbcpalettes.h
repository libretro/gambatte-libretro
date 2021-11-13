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

#include <array/rhmap.h>

namespace {

#define TO5BIT(c8) (((c8) * 0x1F * 2 + 0xFF) / (0xFF*2))
#define PACK15_1(rgb24) (TO5BIT((rgb24) & 0xFF) << 10 | TO5BIT((rgb24) >> 8 & 0xFF) << 5 | TO5BIT((rgb24) >> 16 & 0xFF))
#define PACK15_4(c0, c1, c2, c3) \
	PACK15_1(c0), PACK15_1(c1), PACK15_1(c2), PACK15_1(c3)

//
// Game Boy Palettes
//
static const unsigned short gbdmg[] = { // Original Game Boy
	PACK15_4(0x578200, 0x317400, 0x005121, 0x00420C),
	PACK15_4(0x578200, 0x317400, 0x005121, 0x00420C),
	PACK15_4(0x578200, 0x317400, 0x005121, 0x00420C)
};

static const unsigned short gbpoc[] = { // Game Boy Pocket
	PACK15_4(0xA7B19A, 0x86927C, 0x535f49, 0x2A3325),
	PACK15_4(0xA7B19A, 0x86927C, 0x535f49, 0x2A3325),
	PACK15_4(0xA7B19A, 0x86927C, 0x535f49, 0x2A3325)
};

static const unsigned short gblit[] = { // Game Boy Light
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D),
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D),
	PACK15_4(0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D)
};

//
// Game Boy Color Palettes
//
static const unsigned short p005[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000)
};

static const unsigned short p006[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000)
};

static const unsigned short p007[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000)
};

static const unsigned short p008[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000)
};

static const unsigned short p012[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p013[] = {
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF)
};

static const unsigned short p016[] = {
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000)
};

static const unsigned short p017[] = {
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000)
};

static const unsigned short p01B[] = {
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000)
};

static const unsigned short p100[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000)
};

static const unsigned short p10B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p10D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000)
};

static const unsigned short p110[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p11C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000)
};

static const unsigned short p20B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p20C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static const unsigned short p300[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000)
};

static const unsigned short p304[] = {
	PACK15_4(0xFFFFFF, 0x7BFF00, 0xB57300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p305[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p306[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p308[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000)
};

static const unsigned short p30A[] = {
	PACK15_4(0xB5B5FF, 0xFFFF94, 0xAD5A42, 0x000000),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A)
};

static const unsigned short p30C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static const unsigned short p30D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p30E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p30F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p312[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p319[] = {
	PACK15_4(0xFFE6C5, 0xCE9C84, 0x846B29, 0x5A3108),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p31C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p405[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p406[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p407[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p500[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p501[] = {
	PACK15_4(0xFFFF9C, 0x94B5FF, 0x639473, 0x003A3A),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p502[] = {
	PACK15_4(0x6BFF00, 0xFFFFFF, 0xFF524A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p503[] = {
	PACK15_4(0x52DE00, 0xFF8400, 0xFFFF00, 0xFFFFFF),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p508[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0x0000FF, 0xFFFFFF, 0xFFFF7B, 0x0084FF)
};

static const unsigned short p509[] = {
	PACK15_4(0xFFFFCE, 0x63EFEF, 0x9C8431, 0x5A5A5A),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p50B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF7B, 0x0084FF, 0xFF0000)
};

static const unsigned short p50C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p50D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p50E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p50F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p510[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p511[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x00FF00, 0x318400, 0x004A00),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p512[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p514[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFF00, 0xFF0000, 0x630000, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p515[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p518[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p51A[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0x7B4A00, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p51C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

// Extra palettes
static const unsigned short pExt1[] = {
	PACK15_4(0xE5EA93, 0xC4C641, 0x5E7C39, 0x21442A),
	PACK15_4(0xE5EA93, 0xC4C641, 0x5E7C39, 0x21442A),
	PACK15_4(0xE5EA93, 0xC4C641, 0x5E7C39, 0x21442A)
};

static const unsigned short pExt2[] = {
	PACK15_4(0xF8F8F8, 0x83C656, 0x187890, 0x000000),
	PACK15_4(0xF8F8F8, 0xE18096, 0x7F3848, 0x000000),
	PACK15_4(0xF8F8F8, 0xFFDA03, 0x958401, 0x000000)
};

static const unsigned short pExt3[] = {
	PACK15_4(0xF8F8F8, 0xA59E8C, 0x49726C, 0x000000),
	PACK15_4(0xF8F8F8, 0xE49685, 0x6E241E, 0x000000),
	PACK15_4(0xF8F8F8, 0xD7543C, 0x7D3023, 0x000000)
};

static const unsigned short pExt4[] = {
	PACK15_4(0x9CA684, 0x727C5A, 0x464A35, 0x181810),
	PACK15_4(0x9CA684, 0x727C5A, 0x464A35, 0x181810),
	PACK15_4(0x9CA684, 0x727C5A, 0x464A35, 0x181810)
};

//
// Super Game Boy Palettes
//
static const unsigned short sgb1A[] = {	// 1-A (SGB Default)
	PACK15_4(0xF8E8C8, 0xD89048, 0xA82820, 0x301850),
	PACK15_4(0xF8E8C8, 0xD89048, 0xA82820, 0x301850),
	PACK15_4(0xF8E8C8, 0xD89048, 0xA82820, 0x301850)
};

static const unsigned short sgb1B[] = {
	PACK15_4(0xD8D8C0, 0xC8B070, 0xB05010, 0x000000),
	PACK15_4(0xD8D8C0, 0xC8B070, 0xB05010, 0x000000),
	PACK15_4(0xD8D8C0, 0xC8B070, 0xB05010, 0x000000)
};

static const unsigned short sgb1C[] = {
	PACK15_4(0xF8C0F8, 0xE89850, 0x983860, 0x383898),
	PACK15_4(0xF8C0F8, 0xE89850, 0x983860, 0x383898),
	PACK15_4(0xF8C0F8, 0xE89850, 0x983860, 0x383898)
};

static const unsigned short sgb1D[] = {
	PACK15_4(0xF8F8A8, 0xC08048, 0xF80000, 0x501800),
	PACK15_4(0xF8F8A8, 0xC08048, 0xF80000, 0x501800),
	PACK15_4(0xF8F8A8, 0xC08048, 0xF80000, 0x501800)
};

static const unsigned short sgb1E[] = {
	PACK15_4(0xF8D8B0, 0x78C078, 0x688840, 0x583820),
	PACK15_4(0xF8D8B0, 0x78C078, 0x688840, 0x583820),
	PACK15_4(0xF8D8B0, 0x78C078, 0x688840, 0x583820)
};

static const unsigned short sgb1F[] = {
	PACK15_4(0xD8E8F8, 0xE08850, 0xA80000, 0x004010),
	PACK15_4(0xD8E8F8, 0xE08850, 0xA80000, 0x004010),
	PACK15_4(0xD8E8F8, 0xE08850, 0xA80000, 0x004010)
};

static const unsigned short sgb1G[] = {
	PACK15_4(0x000050, 0x00A0E8, 0x787800, 0xF8F858),
	PACK15_4(0x000050, 0x00A0E8, 0x787800, 0xF8F858),
	PACK15_4(0x000050, 0x00A0E8, 0x787800, 0xF8F858)
};

static const unsigned short sgb1H[] = {
	PACK15_4(0xF8E8E0, 0xF8B888, 0x804000, 0x301800),
	PACK15_4(0xF8E8E0, 0xF8B888, 0x804000, 0x301800),
	PACK15_4(0xF8E8E0, 0xF8B888, 0x804000, 0x301800)
};

static const unsigned short sgb2A[] = {
	PACK15_4(0xF0C8A0, 0xC08848, 0x287800, 0x000000),
	PACK15_4(0xF0C8A0, 0xC08848, 0x287800, 0x000000),
	PACK15_4(0xF0C8A0, 0xC08848, 0x287800, 0x000000)
};

static const unsigned short sgb2B[] = {
	PACK15_4(0xF8F8F8, 0xF8E850, 0xF83000, 0x500058),
	PACK15_4(0xF8F8F8, 0xF8E850, 0xF83000, 0x500058),
	PACK15_4(0xF8F8F8, 0xF8E850, 0xF83000, 0x500058)
};

static const unsigned short sgb2C[] = {
	PACK15_4(0xF8C0F8, 0xE88888, 0x7830E8, 0x282898),
	PACK15_4(0xF8C0F8, 0xE88888, 0x7830E8, 0x282898),
	PACK15_4(0xF8C0F8, 0xE88888, 0x7830E8, 0x282898)
};

static const unsigned short sgb2D[] = {
	PACK15_4(0xF8F8A0, 0x00F800, 0xF83000, 0x000050),
	PACK15_4(0xF8F8A0, 0x00F800, 0xF83000, 0x000050),
	PACK15_4(0xF8F8A0, 0x00F800, 0xF83000, 0x000050)
};

static const unsigned short sgb2E[] = {
	PACK15_4(0xF8C880, 0x90B0E0, 0x281060, 0x100810),
	PACK15_4(0xF8C880, 0x90B0E0, 0x281060, 0x100810),
	PACK15_4(0xF8C880, 0x90B0E0, 0x281060, 0x100810)
};

static const unsigned short sgb2F[] = {
	PACK15_4(0xD0F8F8, 0xF89050, 0xA00000, 0x180000),
	PACK15_4(0xD0F8F8, 0xF89050, 0xA00000, 0x180000),
	PACK15_4(0xD0F8F8, 0xF89050, 0xA00000, 0x180000)
};

static const unsigned short sgb2G[] = {
	PACK15_4(0x68B838, 0xE05040, 0xE0B880, 0x001800),
	PACK15_4(0x68B838, 0xE05040, 0xE0B880, 0x001800),
	PACK15_4(0x68B838, 0xE05040, 0xE0B880, 0x001800)
};

static const unsigned short sgb2H[] = {
	PACK15_4(0xF8F8F8, 0xB8B8B8, 0x707070, 0x000000),
	PACK15_4(0xF8F8F8, 0xB8B8B8, 0x707070, 0x000000),
	PACK15_4(0xF8F8F8, 0xB8B8B8, 0x707070, 0x000000)
};

static const unsigned short sgb3A[] = {
	PACK15_4(0xF8D098, 0x70C0C0, 0xF86028, 0x304860),
	PACK15_4(0xF8D098, 0x70C0C0, 0xF86028, 0x304860),
	PACK15_4(0xF8D098, 0x70C0C0, 0xF86028, 0x304860)
};

static const unsigned short sgb3B[] = {
	PACK15_4(0xD8D8C0, 0xE08020, 0x005000, 0x001010),
	PACK15_4(0xD8D8C0, 0xE08020, 0x005000, 0x001010),
	PACK15_4(0xD8D8C0, 0xE08020, 0x005000, 0x001010)
};

static const unsigned short sgb3C[] = {
	PACK15_4(0xE0A8C8, 0xF8F878, 0x00B8F8, 0x202058),
	PACK15_4(0xE0A8C8, 0xF8F878, 0x00B8F8, 0x202058),
	PACK15_4(0xE0A8C8, 0xF8F878, 0x00B8F8, 0x202058)
};

static const unsigned short sgb3D[] = {
	PACK15_4(0xF0F8B8, 0xE0A878, 0x08C800, 0x000000),
	PACK15_4(0xF0F8B8, 0xE0A878, 0x08C800, 0x000000),
	PACK15_4(0xF0F8B8, 0xE0A878, 0x08C800, 0x000000)
};

static const unsigned short sgb3E[] = {
	PACK15_4(0xF8F8C0, 0xE0B068, 0xB07820, 0x504870),
	PACK15_4(0xF8F8C0, 0xE0B068, 0xB07820, 0x504870),
	PACK15_4(0xF8F8C0, 0xE0B068, 0xB07820, 0x504870)
};

static const unsigned short sgb3F[] = {
	PACK15_4(0x7878C8, 0xF868F8, 0xF8D000, 0x404040),
	PACK15_4(0x7878C8, 0xF868F8, 0xF8D000, 0x404040),
	PACK15_4(0x7878C8, 0xF868F8, 0xF8D000, 0x404040)
};

static const unsigned short sgb3G[] = {
	PACK15_4(0x60D850, 0xF8F8F8, 0xC83038, 0x380000),
	PACK15_4(0x60D850, 0xF8F8F8, 0xC83038, 0x380000),
	PACK15_4(0x60D850, 0xF8F8F8, 0xC83038, 0x380000)
};

static const unsigned short sgb3H[] = {
	PACK15_4(0xE0F8A0, 0x78C838, 0x488818, 0x081800),
	PACK15_4(0xE0F8A0, 0x78C838, 0x488818, 0x081800),
	PACK15_4(0xE0F8A0, 0x78C838, 0x488818, 0x081800)
};

static const unsigned short sgb4A[] = {
	PACK15_4(0xF0A868, 0x78A8F8, 0xD000D0, 0x000078),
	PACK15_4(0xF0A868, 0x78A8F8, 0xD000D0, 0x000078),
	PACK15_4(0xF0A868, 0x78A8F8, 0xD000D0, 0x000078)
};

static const unsigned short sgb4B[] = {
	PACK15_4(0xF0E8F0, 0xE8A060, 0x407838, 0x180808),
	PACK15_4(0xF0E8F0, 0xE8A060, 0x407838, 0x180808),
	PACK15_4(0xF0E8F0, 0xE8A060, 0x407838, 0x180808)
};

static const unsigned short sgb4C[] = {
	PACK15_4(0xF8E0E0, 0xD8A0D0, 0x98A0E0, 0x080000),
	PACK15_4(0xF8E0E0, 0xD8A0D0, 0x98A0E0, 0x080000),
	PACK15_4(0xF8E0E0, 0xD8A0D0, 0x98A0E0, 0x080000)
};

static const unsigned short sgb4D[] = {
	PACK15_4(0xF8F8B8, 0x90C8C8, 0x486878, 0x082048),
	PACK15_4(0xF8F8B8, 0x90C8C8, 0x486878, 0x082048),
	PACK15_4(0xF8F8B8, 0x90C8C8, 0x486878, 0x082048)
};

static const unsigned short sgb4E[] = {
	PACK15_4(0xF8D8A8, 0xE0A878, 0x785888, 0x002030),
	PACK15_4(0xF8D8A8, 0xE0A878, 0x785888, 0x002030),
	PACK15_4(0xF8D8A8, 0xE0A878, 0x785888, 0x002030)
};

static const unsigned short sgb4F[] = {
	PACK15_4(0xB8D0D0, 0xD880D8, 0x8000A0, 0x380000),
	PACK15_4(0xB8D0D0, 0xD880D8, 0x8000A0, 0x380000),
	PACK15_4(0xB8D0D0, 0xD880D8, 0x8000A0, 0x380000)
};

static const unsigned short sgb4G[] = {
	PACK15_4(0xB0E018, 0xB82058, 0x281000, 0x008060),
	PACK15_4(0xB0E018, 0xB82058, 0x281000, 0x008060),
	PACK15_4(0xB0E018, 0xB82058, 0x281000, 0x008060)
};

static const unsigned short sgb4H[] = {
	PACK15_4(0xF8F8C8, 0xB8C058, 0x808840, 0x405028),
	PACK15_4(0xF8F8C8, 0xB8C058, 0x808840, 0x405028),
	PACK15_4(0xF8F8C8, 0xB8C058, 0x808840, 0x405028)
};

//
// Palettes by TheWolfBunny64 (TheWolfBunny on DeviantArt)
// https://www.deviantart.com/thewolfbunny/gallery/69987002/game-boy-palettes
//
static const unsigned short twb64_001_aqours_blue[] = {
	PACK15_4(0x00A0E9, 0x007AB2, 0x004B6D, 0x001C29),
	PACK15_4(0x00A0E9, 0x007AB2, 0x004B6D, 0x001C29),
	PACK15_4(0x00A0E9, 0x007AB2, 0x004B6D, 0x001C29)
};

static const unsigned short twb64_002_anime_expo_ver[] = {
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D),
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D),
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D)
};

static const unsigned short twb64_003_spongebob_yellow[] = {
	PACK15_4(0xF8E867, 0xBDB14E, 0x746D30, 0x2B2812),
	PACK15_4(0xF8E867, 0xBDB14E, 0x746D30, 0x2B2812),
	PACK15_4(0xF8E867, 0xBDB14E, 0x746D30, 0x2B2812)
};

static const unsigned short twb64_004_patrick_star_pink[] = {
	PACK15_4(0xFF7F8C, 0xC3616B, 0x783B41, 0x2D1618),
	PACK15_4(0xFF7F8C, 0xC3616B, 0x783B41, 0x2D1618),
	PACK15_4(0xFF7F8C, 0xC3616B, 0x783B41, 0x2D1618)
};

static const unsigned short twb64_005_neon_red[] = {
	PACK15_4(0xFF3C28, 0xC32D1E, 0x781C12, 0x2D0A07),
	PACK15_4(0xFF3C28, 0xC32D1E, 0x781C12, 0x2D0A07),
	PACK15_4(0xFF3C28, 0xC32D1E, 0x781C12, 0x2D0A07)
};

static const unsigned short twb64_006_neon_blue[] = {
	PACK15_4(0x0AB9E6, 0x078DAF, 0x04576C, 0x012028),
	PACK15_4(0x0AB9E6, 0x078DAF, 0x04576C, 0x012028),
	PACK15_4(0x0AB9E6, 0x078DAF, 0x04576C, 0x012028)
};

static const unsigned short twb64_007_neon_yellow[] = {
	PACK15_4(0xE6FF00, 0xAFC300, 0x6C7800, 0x282D00),
	PACK15_4(0xE6FF00, 0xAFC300, 0x6C7800, 0x282D00),
	PACK15_4(0xE6FF00, 0xAFC300, 0x6C7800, 0x282D00)
};

static const unsigned short twb64_008_neon_green[] = {
	PACK15_4(0x1EDC00, 0x16A800, 0x0E6700, 0x052600),
	PACK15_4(0x1EDC00, 0x16A800, 0x0E6700, 0x052600),
	PACK15_4(0x1EDC00, 0x16A800, 0x0E6700, 0x052600)
};

static const unsigned short twb64_009_neon_pink[] = {
	PACK15_4(0xFF3278, 0xC3265B, 0x781738, 0x2D0815),
	PACK15_4(0xFF3278, 0xC3265B, 0x781738, 0x2D0815),
	PACK15_4(0xFF3278, 0xC3265B, 0x781738, 0x2D0815)
};

static const unsigned short twb64_010_mario_red[] = {
	PACK15_4(0xE10F00, 0xAC0B00, 0x690700, 0x270200),
	PACK15_4(0xE10F00, 0xAC0B00, 0x690700, 0x270200),
	PACK15_4(0xE10F00, 0xAC0B00, 0x690700, 0x270200)
};

static const unsigned short twb64_011_nick_orange[] = {
	PACK15_4(0xFF6700, 0xC34E00, 0x783000, 0x2D1200),
	PACK15_4(0xFF6700, 0xC34E00, 0x783000, 0x2D1200),
	PACK15_4(0xFF6700, 0xC34E00, 0x783000, 0x2D1200)
};

static const unsigned short twb64_012_virtual_boy_ver[] = {
	PACK15_4(0xFF0000, 0xAA0000, 0x550000, 0x000000),
	PACK15_4(0xFF0000, 0xAA0000, 0x550000, 0x000000),
	PACK15_4(0xFF0000, 0xAA0000, 0x550000, 0x000000)
};

static const unsigned short twb64_013_golden_wild[] = {
	PACK15_4(0xB99F65, 0x8D794D, 0x574A2F, 0x201C11),
	PACK15_4(0xB99F65, 0x8D794D, 0x574A2F, 0x201C11),
	PACK15_4(0xB99F65, 0x8D794D, 0x574A2F, 0x201C11)
};

static const unsigned short twb64_014_builder_yellow[] = {
	PACK15_4(0xFACD00, 0xBF9C00, 0x756000, 0x2C2400),
	PACK15_4(0xFACD00, 0xBF9C00, 0x756000, 0x2C2400),
	PACK15_4(0xFACD00, 0xBF9C00, 0x756000, 0x2C2400)
};

static const unsigned short twb64_015_classic_blurple[] = {
	PACK15_4(0x7289DA, 0x5768A6, 0x354066, 0x141826),
	PACK15_4(0x7289DA, 0x5768A6, 0x354066, 0x141826),
	PACK15_4(0x7289DA, 0x5768A6, 0x354066, 0x141826)
};

static const unsigned short twb64_016_765_production_ver[] = {
	PACK15_4(0xBBC4E4, 0x8F95AE, 0x585C6B, 0x212228),
	PACK15_4(0xBBC4E4, 0x8F95AE, 0x585C6B, 0x212228),
	PACK15_4(0xBBC4E4, 0x8F95AE, 0x585C6B, 0x212228)
};

static const unsigned short twb64_017_superball_ivory[] = {
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432),
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432),
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432)
};

static const unsigned short twb64_018_crunchyroll_orange[] = {
	PACK15_4(0xF47522, 0xBA519A, 0x723710, 0x2B1406),
	PACK15_4(0xF47522, 0xBA519A, 0x723710, 0x2B1406),
	PACK15_4(0xF47522, 0xBA519A, 0x723710, 0x2B1406)
};

static const unsigned short twb64_019_muse_pink[] = {
	PACK15_4(0xE4007F, 0xAE0061, 0x6B003B, 0x280016),
	PACK15_4(0xE4007F, 0xAE0061, 0x6B003B, 0x280016),
	PACK15_4(0xE4007F, 0xAE0061, 0x6B003B, 0x280016)
};

static const unsigned short twb64_020_nijigasaki_yellow[] = {
	PACK15_4(0xFAB920, 0xBF8D18, 0x75570F, 0x2C2005),
	PACK15_4(0xFAB920, 0xBF8D18, 0x75570F, 0x2C2005),
	PACK15_4(0xFAB920, 0xBF8D18, 0x75570F, 0x2C2005)
};

static const unsigned short twb64_021_gamate_ver[] = {
	PACK15_4(0x6BA64A, 0x437A63, 0x255955, 0x12424C),
	PACK15_4(0x6BA64A, 0x437A63, 0x255955, 0x12424C),
	PACK15_4(0x6BA64A, 0x437A63, 0x255955, 0x12424C)
};

static const unsigned short twb64_022_greenscale_ver[] = {
	PACK15_4(0x9CBE0C, 0x6E870A, 0x2C6234, 0x0C360C),
	PACK15_4(0x9CBE0C, 0x6E870A, 0x2C6234, 0x0C360C),
	PACK15_4(0x9CBE0C, 0x6E870A, 0x2C6234, 0x0C360C)
};

static const unsigned short twb64_023_odyssey_gold[] = {
	PACK15_4(0xC2A000, 0x947A00, 0x5B4B00, 0x221C00),
	PACK15_4(0xC2A000, 0x947A00, 0x5B4B00, 0x221C00),
	PACK15_4(0xC2A000, 0x947A00, 0x5B4B00, 0x221C00)
};

static const unsigned short twb64_024_super_saiyan_god[] = {
	PACK15_4(0xDA0363, 0xA6024B, 0x66012E, 0x260011),
	PACK15_4(0xDA0363, 0xA6024B, 0x66012E, 0x260011),
	PACK15_4(0xDA0363, 0xA6024B, 0x66012E, 0x260011)
};

static const unsigned short twb64_025_super_saiyan_blue[] = {
	PACK15_4(0x05BCCC, 0x038F9C, 0x025860, 0x002124),
	PACK15_4(0x05BCCC, 0x038F9C, 0x025860, 0x002124),
	PACK15_4(0x05BCCC, 0x038F9C, 0x025860, 0x002124)
};

static const unsigned short twb64_026_bizarre_pink[] = {
	PACK15_4(0xEB008B, 0xB3006A, 0x6E0041, 0x290018),
	PACK15_4(0xEB008B, 0xB3006A, 0x6E0041, 0x290018),
	PACK15_4(0xEB008B, 0xB3006A, 0x6E0041, 0x290018)
};

static const unsigned short twb64_027_nintendo_switch_lite_ver[] = {
	PACK15_4(0xEFBE2D, 0x00B2B3, 0x646464, 0x3C3C3C),
	PACK15_4(0xEFBE2D, 0x00B2B3, 0x646464, 0x3C3C3C),
	PACK15_4(0xEFBE2D, 0x00B2B3, 0x646464, 0x3C3C3C)
};

static const unsigned short twb64_028_game_com_ver[] = {
	PACK15_4(0xA7BF6B, 0x6F8F4F, 0x0F4F2F, 0x000000),
	PACK15_4(0xA7BF6B, 0x6F8F4F, 0x0F4F2F, 0x000000),
	PACK15_4(0xA7BF6B, 0x6F8F4F, 0x0F4F2F, 0x000000)
};

static const unsigned short twb64_029_sanrio_pink[] = {
	PACK15_4(0xF9C2D0, 0xF485A1, 0xE74B5A, 0x83534D),
	PACK15_4(0xF9C2D0, 0xF485A1, 0xE74B5A, 0x83534D),
	PACK15_4(0xF9C2D0, 0xF485A1, 0xE74B5A, 0x83534D)
};

static const unsigned short twb64_030_bandai_namco_ver[] = {
	PACK15_4(0xFFBF00, 0xFF5C00, 0xE60000, 0x000000),
	PACK15_4(0xFFBF00, 0xFF5C00, 0xE60000, 0x000000),
	PACK15_4(0xFFBF00, 0xFF5C00, 0xE60000, 0x000000)
};

static const unsigned short twb64_031_cosmo_green[] = {
	PACK15_4(0x4EBD59, 0x3B9044, 0x245829, 0x0D210F),
	PACK15_4(0x4EBD59, 0x3B9044, 0x245829, 0x0D210F),
	PACK15_4(0x4EBD59, 0x3B9044, 0x245829, 0x0D210F)
};

static const unsigned short twb64_032_wanda_pink[] = {
	PACK15_4(0xD84C9F, 0xA53A79, 0x65234A, 0x260D1C),
	PACK15_4(0xD84C9F, 0xA53A79, 0x65234A, 0x260D1C),
	PACK15_4(0xD84C9F, 0xA53A79, 0x65234A, 0x260D1C)
};

static const unsigned short twb64_033_links_awakening_dx_ver[] = {
	PACK15_4(0xF8F8B0, 0x78C078, 0x688840, 0x583820),
	PACK15_4(0xF8F8B0, 0x78C078, 0x688840, 0x583820),
	PACK15_4(0xF8F8B0, 0x78C078, 0x688840, 0x583820)
};

static const unsigned short twb64_034_travel_wood[] = {
	PACK15_4(0xF8D8B0, 0xA08058, 0x705030, 0x482810),
	PACK15_4(0xF8D8B0, 0xA08058, 0x705030, 0x482810),
	PACK15_4(0xF8D8B0, 0xA08058, 0x705030, 0x482810)
};

static const unsigned short twb64_035_pokemon_ver[] = {
	PACK15_4(0xF8E8F8, 0xF0B088, 0x807098, 0x181010),
	PACK15_4(0xF8E8F8, 0xF0B088, 0x807098, 0x181010),
	PACK15_4(0xF8E8F8, 0xF0B088, 0x807098, 0x181010)
};

static const unsigned short twb64_036_game_grump_orange[] = {
	PACK15_4(0xE9762F, 0xB25A23, 0x6D3716, 0x291408),
	PACK15_4(0xE9762F, 0xB25A23, 0x6D3716, 0x291408),
	PACK15_4(0xE9762F, 0xB25A23, 0x6D3716, 0x291408)
};

static const unsigned short twb64_037_scooby_doo_mystery_ver[] = {
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31),
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31),
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31)
};

static const unsigned short twb64_038_pokemon_mini_ver[] = {
	PACK15_4(0xA5BEA5, 0x7E917E, 0x4D594D, 0x1D211D),
	PACK15_4(0xA5BEA5, 0x7E917E, 0x4D594D, 0x1D211D),
	PACK15_4(0xA5BEA5, 0x7E917E, 0x4D594D, 0x1D211D)
};

static const unsigned short twb64_039_supervision_ver[] = {
	PACK15_4(0x7CC67C, 0x54A68C, 0x2C6264, 0x0C322C),
	PACK15_4(0x7CC67C, 0x54A68C, 0x2C6264, 0x0C322C),
	PACK15_4(0x7CC67C, 0x54A68C, 0x2C6264, 0x0C322C)
};

static const unsigned short twb64_040_dmg_ver[] = {
	PACK15_4(0x7F860F, 0x577C44, 0x365D48, 0x2A453B),
	PACK15_4(0x7F860F, 0x577C44, 0x365D48, 0x2A453B),
	PACK15_4(0x7F860F, 0x577C44, 0x365D48, 0x2A453B)
};

static const unsigned short twb64_041_pocket_ver[] = {
	PACK15_4(0xC4CFA1, 0x8B956D, 0x4D533C, 0x1F1F1F),
	PACK15_4(0xC4CFA1, 0x8B956D, 0x4D533C, 0x1F1F1F),
	PACK15_4(0xC4CFA1, 0x8B956D, 0x4D533C, 0x1F1F1F)
};

static const unsigned short twb64_042_light_ver[] = {
	PACK15_4(0x00B581, 0x009A71, 0x00694A, 0x004F3B),
	PACK15_4(0x00B581, 0x009A71, 0x00694A, 0x004F3B),
	PACK15_4(0x00B581, 0x009A71, 0x00694A, 0x004F3B)
};

static const unsigned short twb64_043_miraitowa_blue[] = {
	PACK15_4(0x25BDEF, 0x1C90B6, 0x115870, 0x06212A),
	PACK15_4(0x25BDEF, 0x1C90B6, 0x115870, 0x06212A),
	PACK15_4(0x25BDEF, 0x1C90B6, 0x115870, 0x06212A)
};

static const unsigned short twb64_044_someity_pink[] = {
	PACK15_4(0xE50A84, 0xAF0764, 0x6B043E, 0x280117),
	PACK15_4(0xE50A84, 0xAF0764, 0x6B043E, 0x280117),
	PACK15_4(0xE50A84, 0xAF0764, 0x6B043E, 0x280117)
};

static const unsigned short twb64_045_pikachu_yellow[] = {
	PACK15_4(0xFFDC00, 0xC3A800, 0x786700, 0x2D2600),
	PACK15_4(0xFFDC00, 0xC3A800, 0x786700, 0x2D2600),
	PACK15_4(0xFFDC00, 0xC3A800, 0x786700, 0x2D2600)
};

static const unsigned short twb64_046_eevee_brown[] = {
	PACK15_4(0xC88D32, 0x986B26, 0x5E4217, 0x231808),
	PACK15_4(0xC88D32, 0x986B26, 0x5E4217, 0x231808),
	PACK15_4(0xC88D32, 0x986B26, 0x5E4217, 0x231808)
};

static const unsigned short twb64_047_microvision_ver[] = {
	PACK15_4(0xA0A0A0, 0x787878, 0x505050, 0x303030),
	PACK15_4(0xA0A0A0, 0x787878, 0x505050, 0x303030),
	PACK15_4(0xA0A0A0, 0x787878, 0x505050, 0x303030)
};

static const unsigned short twb64_048_ti83_ver[] = {
	PACK15_4(0x9CAA8C, 0x77826B, 0x495051, 0x1B1D18),
	PACK15_4(0x9CAA8C, 0x77826B, 0x495051, 0x1B1D18),
	PACK15_4(0x9CAA8C, 0x77826B, 0x495051, 0x1B1D18)
};

static const unsigned short twb64_049_aegis_cherry[] = {
	PACK15_4(0xDD3B64, 0xA92D4C, 0x681B2F, 0x270A11),
	PACK15_4(0xDD3B64, 0xA92D4C, 0x681B2F, 0x270A11),
	PACK15_4(0xDD3B64, 0xA92D4C, 0x681B2F, 0x270A11)
};

static const unsigned short twb64_050_labo_fawn[] = {
	PACK15_4(0xD7AA73, 0xA48257, 0x655036, 0x251E14),
	PACK15_4(0xD7AA73, 0xA48257, 0x655036, 0x251E14),
	PACK15_4(0xD7AA73, 0xA48257, 0x655036, 0x251E14)
};

static const unsigned short twb64_051_million_live_gold[] = {
	PACK15_4(0xCDB261, 0x9C884A, 0x60532D, 0x241F11),
	PACK15_4(0xCDB261, 0x9C884A, 0x60532D, 0x241F11),
	PACK15_4(0xCDB261, 0x9C884A, 0x60532D, 0x241F11)
};

static const unsigned short twb64_052_tokyo_midtown_ver[] = {
	PACK15_4(0x8FAD15, 0x4B8C2B, 0x44693D, 0x000000),
	PACK15_4(0x8FAD15, 0x4B8C2B, 0x44693D, 0x000000),
	PACK15_4(0x8FAD15, 0x4B8C2B, 0x44693D, 0x000000)
};

static const unsigned short twb64_053_vmu_ver[] = {
	PACK15_4(0x88CCA8, 0x689C80, 0x40604F, 0x081480),
	PACK15_4(0x88CCA8, 0x689C80, 0x40604F, 0x081480),
	PACK15_4(0x88CCA8, 0x689C80, 0x40604F, 0x081480)
};

static const unsigned short twb64_054_game_master_ver[] = {
	PACK15_4(0x829FA6, 0x63797E, 0x3D4A4E, 0x161C1D),
	PACK15_4(0x829FA6, 0x63797E, 0x3D4A4E, 0x161C1D),
	PACK15_4(0x829FA6, 0x63797E, 0x3D4A4E, 0x161C1D)
};

static const unsigned short twb64_055_android_green[] = {
	PACK15_4(0x3DDC84, 0x2EA864, 0x1C673E, 0x0A2617),
	PACK15_4(0x3DDC84, 0x2EA864, 0x1C673E, 0x0A2617),
	PACK15_4(0x3DDC84, 0x2EA864, 0x1C673E, 0x0A2617)
};

static const unsigned short twb64_056_ticketmaster_azure[] = {
	PACK15_4(0x026CDF, 0x0152AA, 0x003268, 0x001327),
	PACK15_4(0x026CDF, 0x0152AA, 0x003268, 0x001327),
	PACK15_4(0x026CDF, 0x0152AA, 0x003268, 0x001327)
};

static const unsigned short twb64_057_google_red[] = {
	PACK15_4(0xEA4335, 0xB23328, 0x6E1F18, 0x290B09),
	PACK15_4(0xEA4335, 0xB23328, 0x6E1F18, 0x290B09),
	PACK15_4(0xEA4335, 0xB23328, 0x6E1F18, 0x290B09)
};

static const unsigned short twb64_058_google_blue[] = {
	PACK15_4(0x4285F4, 0x3265BA, 0x1F3E72, 0x0B172B),
	PACK15_4(0x4285F4, 0x3265BA, 0x1F3E72, 0x0B172B),
	PACK15_4(0x4285F4, 0x3265BA, 0x1F3E72, 0x0B172B)
};

static const unsigned short twb64_059_google_yellow[] = {
	PACK15_4(0xFBBC05, 0xBF8F03, 0x765802, 0x2C2100),
	PACK15_4(0xFBBC05, 0xBF8F03, 0x765802, 0x2C2100),
	PACK15_4(0xFBBC05, 0xBF8F03, 0x765802, 0x2C2100)
};

static const unsigned short twb64_060_google_green[] = {
	PACK15_4(0x34A853, 0x27803F, 0x184F27, 0x091D0E),
	PACK15_4(0x34A853, 0x27803F, 0x184F27, 0x091D0E),
	PACK15_4(0x34A853, 0x27803F, 0x184F27, 0x091D0E)
};

static const unsigned short twb64_061_wonderswan_ver[] = {
	PACK15_4(0xFEFEFE, 0xC2C2C2, 0x686868, 0x1D1D1D),
	PACK15_4(0xFEFEFE, 0xC2C2C2, 0x686868, 0x1D1D1D),
	PACK15_4(0xFEFEFE, 0xC2C2C2, 0x686868, 0x1D1D1D)
};

static const unsigned short twb64_062_neo_geo_pocket_ver[] = {
	PACK15_4(0xF0F0F0, 0xB0B0B0, 0x707070, 0x101010),
	PACK15_4(0xF0F0F0, 0xB0B0B0, 0x707070, 0x101010),
	PACK15_4(0xF0F0F0, 0xB0B0B0, 0x707070, 0x101010)
};

static const unsigned short twb64_063_dew_green[] = {
	PACK15_4(0x97D700, 0x73A400, 0x476500, 0x1A2500),
	PACK15_4(0x97D700, 0x73A400, 0x476500, 0x1A2500),
	PACK15_4(0x97D700, 0x73A400, 0x476500, 0x1A2500)
};

static const unsigned short twb64_064_coca_cola_red[] = {
	PACK15_4(0xF40009, 0xBA0006, 0x720004, 0x2B0001),
	PACK15_4(0xF40009, 0xBA0006, 0x720004, 0x2B0001),
	PACK15_4(0xF40009, 0xBA0006, 0x720004, 0x2B0001)
};

static const unsigned short twb64_065_gameking_ver[] = {
	PACK15_4(0x8CCE94, 0x6B9C63, 0x506541, 0x184221),
	PACK15_4(0x8CCE94, 0x6B9C63, 0x506541, 0x184221),
	PACK15_4(0x8CCE94, 0x6B9C63, 0x506541, 0x184221)
};

static const unsigned short twb64_066_do_the_dew_ver[] = {
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C),
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C),
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C)
};

static const unsigned short twb64_067_digivice_ver[] = {
	PACK15_4(0x8C8C73, 0x6B6B57, 0x414136, 0x181814),
	PACK15_4(0x8C8C73, 0x6B6B57, 0x414136, 0x181814),
	PACK15_4(0x8C8C73, 0x6B6B57, 0x414136, 0x181814)
};

static const unsigned short twb64_068_bikini_bottom_ver[] = {
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000),
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000),
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000)
};

static const unsigned short twb64_069_blossom_pink[] = {
	PACK15_4(0xF09898, 0xB77474, 0x704747, 0x2A1A1A),
	PACK15_4(0xF09898, 0xB77474, 0x704747, 0x2A1A1A),
	PACK15_4(0xF09898, 0xB77474, 0x704747, 0x2A1A1A)
};

static const unsigned short twb64_070_bubbles_blue[] = {
	PACK15_4(0x88D0F0, 0x679FB7, 0x406170, 0x18242A),
	PACK15_4(0x88D0F0, 0x679FB7, 0x406170, 0x18242A),
	PACK15_4(0x88D0F0, 0x679FB7, 0x406170, 0x18242A)
};

static const unsigned short twb64_071_buttercup_green[] = {
	PACK15_4(0xB8E088, 0x8CAB68, 0x566940, 0x202718),
	PACK15_4(0xB8E088, 0x8CAB68, 0x566940, 0x202718),
	PACK15_4(0xB8E088, 0x8CAB68, 0x566940, 0x202718)
};

static const unsigned short twb64_072_nascar_ver[] = {
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000),
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000),
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000)
};

static const unsigned short twb64_073_lemon_lime_green[] = {
	PACK15_4(0xF1C545, 0x51A631, 0x336632, 0x142813),
	PACK15_4(0xF1C545, 0x51A631, 0x336632, 0x142813),
	PACK15_4(0xF1C545, 0x51A631, 0x336632, 0x142813)
};

static const unsigned short twb64_074_mega_man_v_ver[] = {
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030),
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030),
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030)
};

static const unsigned short twb64_075_tamagotchi_ver[] = {
	PACK15_4(0xF1F0F9, 0xB8B7BE, 0x717075, 0x3C3838),
	PACK15_4(0xF1F0F9, 0xB8B7BE, 0x717075, 0x3C3838),
	PACK15_4(0xF1F0F9, 0xB8B7BE, 0x717075, 0x3C3838)
};

static const unsigned short twb64_076_phantom_red[] = {
	PACK15_4(0xFF0000, 0xC30000, 0x780000, 0x2D0000),
	PACK15_4(0xFF0000, 0xC30000, 0x780000, 0x2D0000),
	PACK15_4(0xFF0000, 0xC30000, 0x780000, 0x2D0000)
};

static const unsigned short twb64_077_halloween_ver[] = {
	PACK15_4(0xFFCC00, 0xF68C00, 0x9540A5, 0x2C1331),
	PACK15_4(0xFFCC00, 0xF68C00, 0x9540A5, 0x2C1331),
	PACK15_4(0xFFCC00, 0xF68C00, 0x9540A5, 0x2C1331)
};

static const unsigned short twb64_078_christmas_ver[] = {
	PACK15_4(0xCBB96A, 0x20A465, 0xA03232, 0x300F0F),
	PACK15_4(0xCBB96A, 0x20A465, 0xA03232, 0x300F0F),
	PACK15_4(0xCBB96A, 0x20A465, 0xA03232, 0x300F0F)
};

static const unsigned short twb64_079_cardcaptor_pink[] = {
	PACK15_4(0xF2F4F7, 0xEAC3D6, 0xE10E82, 0x430427),
	PACK15_4(0xF2F4F7, 0xEAC3D6, 0xE10E82, 0x430427),
	PACK15_4(0xF2F4F7, 0xEAC3D6, 0xE10E82, 0x430427)
};

static const unsigned short twb64_080_pretty_guardian_gold[] = {
	PACK15_4(0xB4AA82, 0x898263, 0x54503D, 0x1F1E16),
	PACK15_4(0xB4AA82, 0x898263, 0x54503D, 0x1F1E16),
	PACK15_4(0xB4AA82, 0x898263, 0x54503D, 0x1F1E16)
};

static const unsigned short twb64_081_camoflauge_ver[] = {
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538),
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538),
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538)
};

static const unsigned short twb64_082_legendary_super_saiyan[] = {
	PACK15_4(0xA5AB5A, 0x7EA744, 0x4D672A, 0x1D260F),
	PACK15_4(0xA5AB5A, 0x7EA744, 0x4D672A, 0x1D260F),
	PACK15_4(0xA5AB5A, 0x7EA744, 0x4D672A, 0x1D260F)
};

static const unsigned short twb64_083_super_saiyan_rose[] = {
	PACK15_4(0xF4AFB2, 0xBA8588, 0x725253, 0x2B1E1F),
	PACK15_4(0xF4AFB2, 0xBA8588, 0x725253, 0x2B1E1F),
	PACK15_4(0xF4AFB2, 0xBA8588, 0x725253, 0x2B1E1F)
};

static const unsigned short twb64_084_super_saiyan[] = {
	PACK15_4(0xFEF47E, 0xC2BA60, 0x77723B, 0x2C2B16),
	PACK15_4(0xFEF47E, 0xC2BA60, 0x77723B, 0x2C2B16),
	PACK15_4(0xFEF47E, 0xC2BA60, 0x77723B, 0x2C2B16)
};

static const unsigned short twb64_085_mastered_ultra_instinct[] = {
	PACK15_4(0xC0C8D8, 0x9298A5, 0x5A5E65, 0x212326),
	PACK15_4(0xC0C8D8, 0x9298A5, 0x5A5E65, 0x212326),
	PACK15_4(0xC0C8D8, 0x9298A5, 0x5A5E65, 0x212326)
};

static const unsigned short twb64_086_saint_snow_red[] = {
	PACK15_4(0xCA3935, 0x9A2B28, 0x5F1A18, 0x230A09),
	PACK15_4(0xCA3935, 0x9A2B28, 0x5F1A18, 0x230A09),
	PACK15_4(0xCA3935, 0x9A2B28, 0x5F1A18, 0x230A09)
};

static const unsigned short twb64_087_yellow_banana[] = {
	PACK15_4(0xF8D808, 0xD89800, 0xA86838, 0x704800),
	PACK15_4(0xF8D808, 0xD89800, 0xA86838, 0x704800),
	PACK15_4(0xF8D808, 0xD89800, 0xA86838, 0x704800)
};

static const unsigned short twb64_088_green_banana[] = {
	PACK15_4(0x60D808, 0x489800, 0x386838, 0x204800),
	PACK15_4(0x60D808, 0x489800, 0x386838, 0x204800),
	PACK15_4(0x60D808, 0x489800, 0x386838, 0x204800)
};

static const unsigned short twb64_089_super_saiyan_3[] = {
	PACK15_4(0xF9CA3A, 0xBE9A2C, 0x755F1B, 0x2B230A),
	PACK15_4(0xF9CA3A, 0xBE9A2C, 0x755F1B, 0x2B230A),
	PACK15_4(0xF9CA3A, 0xBE9A2C, 0x755F1B, 0x2B230A)
};

static const unsigned short twb64_090_super_saiyan_blue_evolved[] = {
	PACK15_4(0x1B94D0, 0x14719F, 0x0C4561, 0x041A24),
	PACK15_4(0x1B94D0, 0x14719F, 0x0C4561, 0x041A24),
	PACK15_4(0x1B94D0, 0x14719F, 0x0C4561, 0x041A24)
};

static const unsigned short twb64_091_pocket_tales_ver[] = {
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000),
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000),
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000)
};

static const unsigned short twb64_092_investigation_yellow[] = {
	PACK15_4(0xFFFF00, 0xC3C300, 0x787800, 0x2D2D00),
	PACK15_4(0xFFFF00, 0xC3C300, 0x787800, 0x2D2D00),
	PACK15_4(0xFFFF00, 0xC3C300, 0x787800, 0x2D2D00)
};

static const unsigned short twb64_093_sees_blue[] = {
	PACK15_4(0x009AFF, 0x0075C3, 0x004878, 0x001B2D),
	PACK15_4(0x009AFF, 0x0075C3, 0x004878, 0x001B2D),
	PACK15_4(0x009AFF, 0x0075C3, 0x004878, 0x001B2D)
};

static const unsigned short twb64_094_game_awards_cyan[] = {
	PACK15_4(0x49E8D9, 0x37B1A5, 0x226D66, 0x0C2826),
	PACK15_4(0x49E8D9, 0x37B1A5, 0x226D66, 0x0C2826),
	PACK15_4(0x49E8D9, 0x37B1A5, 0x226D66, 0x0C2826)
};

static const unsigned short twb64_095_hokage_orange[] = {
	PACK15_4(0xEA8352, 0xB2643E, 0x6E3D26, 0x29170E),
	PACK15_4(0xEA8352, 0xB2643E, 0x6E3D26, 0x29170E),
	PACK15_4(0xEA8352, 0xB2643E, 0x6E3D26, 0x29170E)
};

static const unsigned short twb64_096_straw_hat_red[] = {
	PACK15_4(0xF8523C, 0xBD3E2D, 0x74261C, 0x2B0E0A),
	PACK15_4(0xF8523C, 0xBD3E2D, 0x74261C, 0x2B0E0A),
	PACK15_4(0xF8523C, 0xBD3E2D, 0x74261C, 0x2B0E0A)
};

static const unsigned short twb64_097_sword_art_cyan[] = {
	PACK15_4(0x59C3E2, 0x4495AC, 0x295B6A, 0x0F2227),
	PACK15_4(0x59C3E2, 0x4495AC, 0x295B6A, 0x0F2227),
	PACK15_4(0x59C3E2, 0x4495AC, 0x295B6A, 0x0F2227)
};

static const unsigned short twb64_098_deku_alpha_emerald[] = {
	PACK15_4(0x38AD9D, 0x2A8478, 0x1A5149, 0x091E1B),
	PACK15_4(0x38AD9D, 0x2A8478, 0x1A5149, 0x091E1B),
	PACK15_4(0x38AD9D, 0x2A8478, 0x1A5149, 0x091E1B)
};

static const unsigned short twb64_099_blue_stripes_ver[] = {
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5A5B5B, 0x232424),
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5A5B5B, 0x232424),
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5A5B5B, 0x232424)
};

static const unsigned short twb64_100_stone_orange[] = {
	PACK15_4(0xF6821E, 0xBC6316, 0x733D0E, 0x2B1605),
	PACK15_4(0xF6821E, 0xBC6316, 0x733D0E, 0x2B1605),
	PACK15_4(0xF6821E, 0xBC6316, 0x733D0E, 0x2B1605)
};

static const unsigned short twb64_101_765pro_pink[] = {
	PACK15_4(0xF34F6D, 0xB93C53, 0x722533, 0x2A0D13),
	PACK15_4(0xF34F6D, 0xB93C53, 0x722533, 0x2A0D13),
	PACK15_4(0xF34F6D, 0xB93C53, 0x722533, 0x2A0D13)
};

static const unsigned short twb64_102_cinderella_blue[] = {
	PACK15_4(0x2681C8, 0x1D6298, 0x113C5E, 0x061623),
	PACK15_4(0x2681C8, 0x1D6298, 0x113C5E, 0x061623),
	PACK15_4(0x2681C8, 0x1D6298, 0x113C5E, 0x061623)
};

static const unsigned short twb64_103_million_yellow[] = {
	PACK15_4(0xFFC30B, 0xC39508, 0x785B05, 0x2D2201),
	PACK15_4(0xFFC30B, 0xC39508, 0x785B05, 0x2D2201),
	PACK15_4(0xFFC30B, 0xC39508, 0x785B05, 0x2D2201)
};

static const unsigned short twb64_104_sidem_green[] = {
	PACK15_4(0x0FBE76, 0x0B915A, 0x075937, 0x022114),
	PACK15_4(0x0FBE76, 0x0B915A, 0x075937, 0x022114),
	PACK15_4(0x0FBE76, 0x0B915A, 0x075937, 0x022114)
};

static const unsigned short twb64_105_shiny_sky_blue[] = {
	PACK15_4(0x8DBBFF, 0x6B8EC3, 0x425878, 0x18212D),
	PACK15_4(0x8DBBFF, 0x6B8EC3, 0x425878, 0x18212D),
	PACK15_4(0x8DBBFF, 0x6B8EC3, 0x425878, 0x18212D)
};

static const unsigned short twb64_106_angry_volcano_ver[] = {
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000),
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000),
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000)
};

static const unsigned short twb64_107_yokai_pink[] = {
	PACK15_4(0xE08AB8, 0xAB698C, 0x694056, 0x271820),
	PACK15_4(0xE08AB8, 0xAB698C, 0x694056, 0x271820),
	PACK15_4(0xE08AB8, 0xAB698C, 0x694056, 0x271820)
};

static const unsigned short twb64_108_yokai_green[] = {
	PACK15_4(0x66B83C, 0x4E8C2D, 0x30561C, 0x11200A),
	PACK15_4(0x66B83C, 0x4E8C2D, 0x30561C, 0x11200A),
	PACK15_4(0x66B83C, 0x4E8C2D, 0x30561C, 0x11200A)
};

static const unsigned short twb64_109_yokai_blue[] = {
	PACK15_4(0x33B6EA, 0x278BB2, 0x18556E, 0x092029),
	PACK15_4(0x33B6EA, 0x278BB2, 0x18556E, 0x092029),
	PACK15_4(0x33B6EA, 0x278BB2, 0x18556E, 0x092029)
};

static const unsigned short twb64_110_yokai_purple[] = {
	PACK15_4(0x938AC1, 0x706993, 0x45405A, 0x191822),
	PACK15_4(0x938AC1, 0x706993, 0x45405A, 0x191822),
	PACK15_4(0x938AC1, 0x706993, 0x45405A, 0x191822)
};

static const unsigned short twb64_111_aquatic_iro[] = {
	PACK15_4(0xA0D8EF, 0x2CA9E1, 0x3E62AD, 0x192F60),
	PACK15_4(0xA0D8EF, 0x2CA9E1, 0x3E62AD, 0x192F60),
	PACK15_4(0xA0D8EF, 0x2CA9E1, 0x3E62AD, 0x192F60)
};

static const unsigned short twb64_112_tea_midori[] = {
	PACK15_4(0xD6E9CA, 0x88CB7F, 0x028760, 0x333631),
	PACK15_4(0xD6E9CA, 0x88CB7F, 0x028760, 0x333631),
	PACK15_4(0xD6E9CA, 0x88CB7F, 0x028760, 0x333631)
};

static const unsigned short twb64_113_sakura_pink[] = {
	PACK15_4(0xFDEFF2, 0xEEBBCB, 0xE7609E, 0xA25768),
	PACK15_4(0xFDEFF2, 0xEEBBCB, 0xE7609E, 0xA25768),
	PACK15_4(0xFDEFF2, 0xEEBBCB, 0xE7609E, 0xA25768)
};

static const unsigned short twb64_114_wisteria_murasaki[] = {
	PACK15_4(0xDBD0E6, 0xA59ACA, 0x7058A3, 0x2E2930),
	PACK15_4(0xDBD0E6, 0xA59ACA, 0x7058A3, 0x2E2930),
	PACK15_4(0xDBD0E6, 0xA59ACA, 0x7058A3, 0x2E2930)
};

static const unsigned short twb64_115_oni_aka[] = {
	PACK15_4(0xEC6D71, 0xD9333F, 0xA22041, 0x640125),
	PACK15_4(0xEC6D71, 0xD9333F, 0xA22041, 0x640125),
	PACK15_4(0xEC6D71, 0xD9333F, 0xA22041, 0x640125)
};

static const unsigned short twb64_116_golden_kiiro[] = {
	PACK15_4(0xF8E58C, 0xDCCB18, 0xA69425, 0x6A5D21),
	PACK15_4(0xF8E58C, 0xDCCB18, 0xA69425, 0x6A5D21),
	PACK15_4(0xF8E58C, 0xDCCB18, 0xA69425, 0x6A5D21)
};

static const unsigned short twb64_117_silver_shiro[] = {
	PACK15_4(0xDCDDDD, 0xAFAFB0, 0x727171, 0x383C3C),
	PACK15_4(0xDCDDDD, 0xAFAFB0, 0x727171, 0x383C3C),
	PACK15_4(0xDCDDDD, 0xAFAFB0, 0x727171, 0x383C3C)
};

static const unsigned short twb64_118_fruity_orange[] = {
	PACK15_4(0xF3BF88, 0xF08300, 0x9F563A, 0x241A08),
	PACK15_4(0xF3BF88, 0xF08300, 0x9F563A, 0x241A08),
	PACK15_4(0xF3BF88, 0xF08300, 0x9F563A, 0x241A08)
};

static const unsigned short twb64_119_akb48_pink[] = {
	PACK15_4(0xF596B4, 0xBB7289, 0x734654, 0x2B1A1F),
	PACK15_4(0xF596B4, 0xBB7289, 0x734654, 0x2B1A1F),
	PACK15_4(0xF596B4, 0xBB7289, 0x734654, 0x2B1A1F)
};

static const unsigned short twb64_120_miku_blue[] = {
	PACK15_4(0x11ADD5, 0x0D84A2, 0x085164, 0x031E25),
	PACK15_4(0x11ADD5, 0x0D84A2, 0x085164, 0x031E25),
	PACK15_4(0x11ADD5, 0x0D84A2, 0x085164, 0x031E25)
};

static const unsigned short twb64_121_fairy_tail_red[] = {
	PACK15_4(0xC7000A, 0x980007, 0x5D0004, 0x230001),
	PACK15_4(0xC7000A, 0x980007, 0x5D0004, 0x230001),
	PACK15_4(0xC7000A, 0x980007, 0x5D0004, 0x230001)
};

static const unsigned short twb64_122_survey_corps_brown[] = {
	PACK15_4(0xAB7D57, 0x825F42, 0x503A28, 0x1E160F),
	PACK15_4(0xAB7D57, 0x825F42, 0x503A28, 0x1E160F),
	PACK15_4(0xAB7D57, 0x825F42, 0x503A28, 0x1E160F)
};

static const unsigned short twb64_123_island_green[] = {
	PACK15_4(0x009B7E, 0x007660, 0x00483B, 0x001B16),
	PACK15_4(0x009B7E, 0x007660, 0x00483B, 0x001B16),
	PACK15_4(0x009B7E, 0x007660, 0x00483B, 0x001B16)
};

static const unsigned short twb64_124_mania_plus_green[] = {
	PACK15_4(0x79C14E, 0x5C933B, 0x385A24, 0x15220D),
	PACK15_4(0x79C14E, 0x5C933B, 0x385A24, 0x15220D),
	PACK15_4(0x79C14E, 0x5C933B, 0x385A24, 0x15220D)
};

static const unsigned short twb64_125_ninja_turtle_green[] = {
	PACK15_4(0x8CC63F, 0x6B9730, 0x415D1D, 0x18220B),
	PACK15_4(0x8CC63F, 0x6B9730, 0x415D1D, 0x18220B),
	PACK15_4(0x8CC63F, 0x6B9730, 0x415D1D, 0x18220B)
};

static const unsigned short twb64_126_slime_blue[] = {
	PACK15_4(0x2F8CCC, 0x236B9C, 0x164160, 0x081824),
	PACK15_4(0x2F8CCC, 0x236B9C, 0x164160, 0x081824),
	PACK15_4(0x2F8CCC, 0x236B9C, 0x164160, 0x081824)
};

static const unsigned short twb64_127_lime_midori[] = {
	PACK15_4(0xE0EBAF, 0xAACF53, 0x7B8D42, 0x475950),
	PACK15_4(0xE0EBAF, 0xAACF53, 0x7B8D42, 0x475950),
	PACK15_4(0xE0EBAF, 0xAACF53, 0x7B8D42, 0x475950)
};

static const unsigned short twb64_128_ghostly_aoi[] = {
	PACK15_4(0x84A2D4, 0x5A79BA, 0x19448E, 0x0F2350),
	PACK15_4(0x84A2D4, 0x5A79BA, 0x19448E, 0x0F2350),
	PACK15_4(0x84A2D4, 0x5A79BA, 0x19448E, 0x0F2350)
};

static const unsigned short twb64_129_retro_bogeda[] = {
	PACK15_4(0xFBFD1B, 0xFF6CFF, 0x6408FF, 0x000000),
	PACK15_4(0xFBFD1B, 0xFF6CFF, 0x6408FF, 0x000000),
	PACK15_4(0xFBFD1B, 0xFF6CFF, 0x6408FF, 0x000000)
};

static const unsigned short twb64_130_royal_blue[] = {
	PACK15_4(0x4655F5, 0x3540BB, 0x202773, 0x0C0E2B),
	PACK15_4(0x4655F5, 0x3540BB, 0x202773, 0x0C0E2B),
	PACK15_4(0x4655F5, 0x3540BB, 0x202773, 0x0C0E2B)
};

static const unsigned short twb64_131_neon_purple[] = {
	PACK15_4(0xB400E6, 0x8900AF, 0x54006C, 0x1F0028),
	PACK15_4(0xB400E6, 0x8900AF, 0x54006C, 0x1F0028),
	PACK15_4(0xB400E6, 0x8900AF, 0x54006C, 0x1F0028)
};

static const unsigned short twb64_132_neon_orange[] = {
	PACK15_4(0xFAA005, 0xBF7A03, 0x754B02, 0x2C1C00),
	PACK15_4(0xFAA005, 0xBF7A03, 0x754B02, 0x2C1C00),
	PACK15_4(0xFAA005, 0xBF7A03, 0x754B02, 0x2C1C00)
};

static const unsigned short twb64_133_moonlight_vision[] = {
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010),
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010),
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010)
};

static const unsigned short twb64_134_tokyo_red[] = {
	PACK15_4(0xB11D33, 0x871626, 0x530D17, 0x1F0508),
	PACK15_4(0xB11D33, 0x871626, 0x530D17, 0x1F0508),
	PACK15_4(0xB11D33, 0x871626, 0x530D17, 0x1F0508)
};

static const unsigned short twb64_135_paris_gold[] = {
	PACK15_4(0xD7C378, 0xA4955B, 0x655B38, 0x252215),
	PACK15_4(0xD7C378, 0xA4955B, 0x655B38, 0x252215),
	PACK15_4(0xD7C378, 0xA4955B, 0x655B38, 0x252215)
};

static const unsigned short twb64_136_beijing_blue[] = {
	PACK15_4(0x3AADDD, 0x2C84A9, 0x1B5168, 0x0A1E27),
	PACK15_4(0x3AADDD, 0x2C84A9, 0x1B5168, 0x0A1E27),
	PACK15_4(0x3AADDD, 0x2C84A9, 0x1B5168, 0x0A1E27)
};

static const unsigned short twb64_137_pacman_yellow[] = {
	PACK15_4(0xFFE300, 0xC3AD00, 0x786A00, 0x2D2800),
	PACK15_4(0xFFE300, 0xC3AD00, 0x786A00, 0x2D2800),
	PACK15_4(0xFFE300, 0xC3AD00, 0x786A00, 0x2D2800)
};

static const unsigned short twb64_138_irish_green[] = {
	PACK15_4(0x45BE76, 0x34915A, 0x205937, 0x0C2114),
	PACK15_4(0x45BE76, 0x34915A, 0x205937, 0x0C2114),
	PACK15_4(0x45BE76, 0x34915A, 0x205937, 0x0C2114)
};

static const unsigned short twb64_139_kakarot_orange[] = {
	PACK15_4(0xE7612C, 0xB04A21, 0x6C2D14, 0x281107),
	PACK15_4(0xE7612C, 0xB04A21, 0x6C2D14, 0x281107),
	PACK15_4(0xE7612C, 0xB04A21, 0x6C2D14, 0x281107)
};

static const unsigned short twb64_140_dragon_ball_orange[] = {
	PACK15_4(0xFF9518, 0xC37112, 0x78460B, 0x2D1A04),
	PACK15_4(0xFF9518, 0xC37112, 0x78460B, 0x2D1A04),
	PACK15_4(0xFF9518, 0xC37112, 0x78460B, 0x2D1A04)
};

static const unsigned short twb64_141_christmas_gold[] = {
	PACK15_4(0xC4B56C, 0x958A52, 0x5C5532, 0x221F13),
	PACK15_4(0xC4B56C, 0x958A52, 0x5C5532, 0x221F13),
	PACK15_4(0xC4B56C, 0x958A52, 0x5C5532, 0x221F13)
};

static const unsigned short twb64_142_pepsi_blue[] = {
	PACK15_4(0x005CB4, 0x004689, 0x002B54, 0x00101F),
	PACK15_4(0x005CB4, 0x004689, 0x002B54, 0x00101F),
	PACK15_4(0x005CB4, 0x004689, 0x002B54, 0x00101F)
};

static const unsigned short twb64_143_bubblun_green[] = {
	PACK15_4(0x6ADC31, 0x51A825, 0x316717, 0x122608),
	PACK15_4(0x6ADC31, 0x51A825, 0x316717, 0x122608),
	PACK15_4(0x6ADC31, 0x51A825, 0x316717, 0x122608)
};

static const unsigned short twb64_144_bobblun_blue[] = {
	PACK15_4(0x1FD1FD, 0x179FC1, 0x0E6277, 0x05242C),
	PACK15_4(0x1FD1FD, 0x179FC1, 0x0E6277, 0x05242C),
	PACK15_4(0x1FD1FD, 0x179FC1, 0x0E6277, 0x05242C)
};

static const unsigned short twb64_145_baja_blast_storm[] = {
	PACK15_4(0x61C0B4, 0x4A9289, 0x2D5A54, 0x11211F),
	PACK15_4(0x61C0B4, 0x4A9289, 0x2D5A54, 0x11211F),
	PACK15_4(0x61C0B4, 0x4A9289, 0x2D5A54, 0x11211F)
};

static const unsigned short twb64_146_olympic_gold[] = {
	PACK15_4(0xD1C260, 0x9F9449, 0x625B2D, 0x242210),
	PACK15_4(0xD1C260, 0x9F9449, 0x625B2D, 0x242210),
	PACK15_4(0xD1C260, 0x9F9449, 0x625B2D, 0x242210)
};

static const unsigned short twb64_147_value_orange[] = {
	PACK15_4(0xE68E00, 0xAF6C00, 0x6C4200, 0x281900),
	PACK15_4(0xE68E00, 0xAF6C00, 0x6C4200, 0x281900),
	PACK15_4(0xE68E00, 0xAF6C00, 0x6C4200, 0x281900)
};

static const unsigned short twb64_148_liella_purple[] = {
	PACK15_4(0xA5469B, 0x7E3576, 0x4D2048, 0x1D0C1B),
	PACK15_4(0xA5469B, 0x7E3576, 0x4D2048, 0x1D0C1B),
	PACK15_4(0xA5469B, 0x7E3576, 0x4D2048, 0x1D0C1B)
};

static const unsigned short twb64_149_olympic_silver[] = {
	PACK15_4(0xA6AAB1, 0x7E8287, 0x4E4F53, 0x1D1E1F),
	PACK15_4(0xA6AAB1, 0x7E8287, 0x4E4F53, 0x1D1E1F),
	PACK15_4(0xA6AAB1, 0x7E8287, 0x4E4F53, 0x1D1E1F)
};

static const unsigned short twb64_150_olympic_bronze[] = {
	PACK15_4(0xB57B54, 0x8A5E40, 0x553927, 0x1F150E),
	PACK15_4(0xB57B54, 0x8A5E40, 0x553927, 0x1F150E),
	PACK15_4(0xB57B54, 0x8A5E40, 0x553927, 0x1F150E)
};

static const unsigned short twb64_151_ana_sky_blue[] = {
	PACK15_4(0x00B3F0, 0x0088B7, 0x005470, 0x001F2A),
	PACK15_4(0x00B3F0, 0x0088B7, 0x005470, 0x001F2A),
	PACK15_4(0x00B3F0, 0x0088B7, 0x005470, 0x001F2A)
};

static const unsigned short twb64_152_nijigasaki_orange[] = {
	PACK15_4(0xF39800, 0xB97400, 0x724700, 0x2A1A00),
	PACK15_4(0xF39800, 0xB97400, 0x724700, 0x2A1A00),
	PACK15_4(0xF39800, 0xB97400, 0x724700, 0x2A1A00)
};

static const unsigned short twb64_153_holoblue[] = {
	PACK15_4(0x5EDEEC, 0x47A9B4, 0x2C686F, 0x102729),
	PACK15_4(0x5EDEEC, 0x47A9B4, 0x2C686F, 0x102729),
	PACK15_4(0x5EDEEC, 0x47A9B4, 0x2C686F, 0x102729)
};

static const unsigned short twb64_154_wrestling_red[] = {
	PACK15_4(0xD7182A, 0xA41220, 0x650B13, 0x250407),
	PACK15_4(0xD7182A, 0xA41220, 0x650B13, 0x250407),
	PACK15_4(0xD7182A, 0xA41220, 0x650B13, 0x250407)
};

static const unsigned short twb64_155_yoshi_egg_green[] = {
	PACK15_4(0x66C430, 0x4E9524, 0x305C16, 0x112208),
	PACK15_4(0x66C430, 0x4E9524, 0x305C16, 0x112208),
	PACK15_4(0x66C430, 0x4E9524, 0x305C16, 0x112208)
};

static const unsigned short twb64_156_pokedex_red[] = {
	PACK15_4(0xEA5450, 0xB2403D, 0x6E2725, 0x290E0E),
	PACK15_4(0xEA5450, 0xB2403D, 0x6E2725, 0x290E0E),
	PACK15_4(0xEA5450, 0xB2403D, 0x6E2725, 0x290E0E)
};

static const unsigned short twb64_157_disney_dream_blue[] = {
	PACK15_4(0x1786EB, 0x1166B3, 0x0A3F6E, 0x041729),
	PACK15_4(0x1786EB, 0x1166B3, 0x0A3F6E, 0x041729),
	PACK15_4(0x1786EB, 0x1166B3, 0x0A3F6E, 0x041729)
};

static const unsigned short twb64_158_xbox_green[] = {
	PACK15_4(0x92C83E, 0x6F982F, 0x445E1D, 0x19230A),
	PACK15_4(0x92C83E, 0x6F982F, 0x445E1D, 0x19230A),
	PACK15_4(0x92C83E, 0x6F982F, 0x445E1D, 0x19230A)
};

static const unsigned short twb64_159_sonic_mega_blue[] = {
	PACK15_4(0x4084D9, 0x3064A5, 0x1E3E66, 0x0B1726),
	PACK15_4(0x4084D9, 0x3064A5, 0x1E3E66, 0x0B1726),
	PACK15_4(0x4084D9, 0x3064A5, 0x1E3E66, 0x0B1726)
};

static const unsigned short twb64_160_g4_orange[] = {
	PACK15_4(0xF89D21, 0xBD7819, 0x74490F, 0x2B1B05),
	PACK15_4(0xF89D21, 0xBD7819, 0x74490F, 0x2B1B05),
	PACK15_4(0xF89D21, 0xBD7819, 0x74490F, 0x2B1B05)
};

static const unsigned short twb64_161_scarlett_green[] = {
	PACK15_4(0x9BF00B, 0x76B708, 0x487005, 0x1B2A01),
	PACK15_4(0x9BF00B, 0x76B708, 0x487005, 0x1B2A01),
	PACK15_4(0x9BF00B, 0x76B708, 0x487005, 0x1B2A01)
};

static const unsigned short twb64_162_glitchy_blue[] = {
	PACK15_4(0x3C7AE6, 0x2D5DAF, 0x1C396C, 0x0A1528),
	PACK15_4(0x3C7AE6, 0x2D5DAF, 0x1C396C, 0x0A1528),
	PACK15_4(0x3C7AE6, 0x2D5DAF, 0x1C396C, 0x0A1528)
};

static const unsigned short twb64_163_classic_lcd[] = {
	PACK15_4(0xC6CBAD, 0x979B84, 0x5D5F51, 0x22231E),
	PACK15_4(0xC6CBAD, 0x979B84, 0x5D5F51, 0x22231E),
	PACK15_4(0xC6CBAD, 0x979B84, 0x5D5F51, 0x22231E)
};

static const unsigned short twb64_164_3ds_virtual_console_ver[] = {
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918),
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918),
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918)
};

static const unsigned short twb64_165_pocketstation_ver[] = {
	PACK15_4(0x969687, 0x727267, 0x46463F, 0x1A1A17),
	PACK15_4(0x969687, 0x727267, 0x46463F, 0x1A1A17),
	PACK15_4(0x969687, 0x727267, 0x46463F, 0x1A1A17)
};

static const unsigned short twb64_166_game_and_gold[] = {
	PACK15_4(0xC8AA50, 0x98823D, 0x5E4F25, 0x231E0E),
	PACK15_4(0xC8AA50, 0x98823D, 0x5E4F25, 0x231E0E),
	PACK15_4(0xC8AA50, 0x98823D, 0x5E4F25, 0x231E0E)
};

static const unsigned short twb64_167_smurfy_blue[] = {
	PACK15_4(0x2CB9EF, 0x218DB6, 0x145570, 0x07202A),
	PACK15_4(0x2CB9EF, 0x218DB6, 0x145570, 0x07202A),
	PACK15_4(0x2CB9EF, 0x218DB6, 0x145570, 0x07202A)
};

static const unsigned short twb64_168_swampy_ogre_green[] = {
	PACK15_4(0xC1D62E, 0x93A323, 0x5A6415, 0x222508),
	PACK15_4(0xC1D62E, 0x93A323, 0x5A6415, 0x222508),
	PACK15_4(0xC1D62E, 0x93A323, 0x5A6415, 0x222508)
};

static const unsigned short twb64_169_sailor_spinach_green[] = {
	PACK15_4(0x7BB03C, 0x5E862D, 0x39521C, 0x151F0A),
	PACK15_4(0x7BB03C, 0x5E862D, 0x39521C, 0x151F0A),
	PACK15_4(0x7BB03C, 0x5E862D, 0x39521C, 0x151F0A)
};

static const unsigned short twb64_170_shenron_green[] = {
	PACK15_4(0x5AC34A, 0x449538, 0x2A5B22, 0x0F220D),
	PACK15_4(0x5AC34A, 0x449538, 0x2A5B22, 0x0F220D),
	PACK15_4(0x5AC34A, 0x449538, 0x2A5B22, 0x0F220D)
};

static const unsigned short twb64_171_berserk_blood[] = {
	PACK15_4(0xBB1414, 0x8F0F0F, 0x570909, 0x200303),
	PACK15_4(0xBB1414, 0x8F0F0F, 0x570909, 0x200303),
	PACK15_4(0xBB1414, 0x8F0F0F, 0x570909, 0x200303)
};

static const unsigned short twb64_172_super_star_pink[] = {
	PACK15_4(0xF3A5AA, 0xB97E82, 0x724D4F, 0x2A1D1D),
	PACK15_4(0xF3A5AA, 0xB97E82, 0x724D4F, 0x2A1D1D),
	PACK15_4(0xF3A5AA, 0xB97E82, 0x724D4F, 0x2A1D1D)
};

static const unsigned short twb64_173_gamebuino_classic_ver[] = {
	PACK15_4(0x81A17E, 0x627B60, 0x3C4B3B, 0x161C16),
	PACK15_4(0x81A17E, 0x627B60, 0x3C4B3B, 0x161C16),
	PACK15_4(0x81A17E, 0x627B60, 0x3C4B3B, 0x161C16)
};

static const unsigned short twb64_174_barbie_pink[] = {
	PACK15_4(0xE94397, 0xB23373, 0x6D1F47, 0x290B1A),
	PACK15_4(0xE94397, 0xB23373, 0x6D1F47, 0x290B1A),
	PACK15_4(0xE94397, 0xB23373, 0x6D1F47, 0x290B1A)
};

static const unsigned short twb64_175_star_command_green[] = {
	PACK15_4(0xB2E781, 0x88B062, 0x536C3C, 0x1F2816),
	PACK15_4(0xB2E781, 0x88B062, 0x536C3C, 0x1F2816),
	PACK15_4(0xB2E781, 0x88B062, 0x536C3C, 0x1F2816)
};

static const unsigned short twb64_176_nokia_3310_ver[] = {
	PACK15_4(0x73A684, 0x577E64, 0x364E3E, 0x141D17),
	PACK15_4(0x73A684, 0x577E64, 0x364E3E, 0x141D17),
	PACK15_4(0x73A684, 0x577E64, 0x364E3E, 0x141D17)
};

static const unsigned short twb64_177_clover_green[] = {
	PACK15_4(0x39C63A, 0x2B972C, 0x1A5D1B, 0x0A220A),
	PACK15_4(0x39C63A, 0x2B972C, 0x1A5D1B, 0x0A220A),
	PACK15_4(0x39C63A, 0x2B972C, 0x1A5D1B, 0x0A220A)
};

static const unsigned short twb64_178_crash_orange[] = {
	PACK15_4(0xF37C30, 0xB95E24, 0x723A16, 0x2A1508),
	PACK15_4(0xF37C30, 0xB95E24, 0x723A16, 0x2A1508),
	PACK15_4(0xF37C30, 0xB95E24, 0x723A16, 0x2A1508)
};

static const unsigned short twb64_179_famicom_disk_yellow[] = {
	PACK15_4(0xF3C200, 0xB99400, 0x725B00, 0x2A2200),
	PACK15_4(0xF3C200, 0xB99400, 0x725B00, 0x2A2200),
	PACK15_4(0xF3C200, 0xB99400, 0x725B00, 0x2A2200)
};

static const unsigned short twb64_180_team_rocket_red[] = {
	PACK15_4(0xB83020, 0x8C2418, 0x56160F, 0x200805),
	PACK15_4(0xB83020, 0x8C2418, 0x56160F, 0x200805),
	PACK15_4(0xB83020, 0x8C2418, 0x56160F, 0x200805)
};

static const unsigned short twb64_181_seiko_timer_yellow[] = {
	PACK15_4(0xFCC800, 0xC09800, 0x765E00, 0x2C2300),
	PACK15_4(0xFCC800, 0xC09800, 0x765E00, 0x2C2300),
	PACK15_4(0xFCC800, 0xC09800, 0x765E00, 0x2C2300)
};

static const unsigned short twb64_182_pink109[] = {
	PACK15_4(0xFD87B2, 0xC16788, 0x773F53, 0x2C171F),
	PACK15_4(0xFD87B2, 0xC16788, 0x773F53, 0x2C171F),
	PACK15_4(0xFD87B2, 0xC16788, 0x773F53, 0x2C171F)
};

static const unsigned short twb64_183_doraemon_blue[] = {
	PACK15_4(0x00A8F4, 0x0080BA, 0x004F72, 0x001D2B),
	PACK15_4(0x00A8F4, 0x0080BA, 0x004F72, 0x001D2B),
	PACK15_4(0x00A8F4, 0x0080BA, 0x004F72, 0x001D2B)
};

static const unsigned short twb64_184_fury_blue[] = {
	PACK15_4(0x2B5F98, 0x204874, 0x142C47, 0x07101A),
	PACK15_4(0x2B5F98, 0x204874, 0x142C47, 0x07101A),
	PACK15_4(0x2B5F98, 0x204874, 0x142C47, 0x07101A)
};

static const unsigned short twb64_185_rockstar_orange[] = {
	PACK15_4(0xFCAF17, 0xC08511, 0x76520A, 0x2C1E04),
	PACK15_4(0xFCAF17, 0xC08511, 0x76520A, 0x2C1E04),
	PACK15_4(0xFCAF17, 0xC08511, 0x76520A, 0x2C1E04)
};

static const unsigned short twb64_186_puyo_puyo_green[] = {
	PACK15_4(0x48E236, 0x37AC29, 0x216A19, 0x0C2709),
	PACK15_4(0x48E236, 0x37AC29, 0x216A19, 0x0C2709),
	PACK15_4(0x48E236, 0x37AC29, 0x216A19, 0x0C2709)
};

static const unsigned short twb64_187_susan_g_pink[] = {
	PACK15_4(0xEA77AF, 0xB25A85, 0x6E3852, 0x29151E),
	PACK15_4(0xEA77AF, 0xB25A85, 0x6E3852, 0x29151E),
	PACK15_4(0xEA77AF, 0xB25A85, 0x6E3852, 0x29151E)
};

static const unsigned short twb64_188_pizza_hut_red[] = {
	PACK15_4(0xE3383E, 0xAD2A2F, 0x6A1A1D, 0x28090A),
	PACK15_4(0xE3383E, 0xAD2A2F, 0x6A1A1D, 0x28090A),
	PACK15_4(0xE3383E, 0xAD2A2F, 0x6A1A1D, 0x28090A)
};

static const unsigned short twb64_189_plumbob_green[] = {
	PACK15_4(0x5EEA03, 0x47B202, 0x2C6E01, 0x102900),
	PACK15_4(0x5EEA03, 0x47B202, 0x2C6E01, 0x102900),
	PACK15_4(0x5EEA03, 0x47B202, 0x2C6E01, 0x102900)
};

static const unsigned short twb64_190_grand_ivory[] = {
	PACK15_4(0xD9D6BE, 0xA5A391, 0x666459, 0x262521),
	PACK15_4(0xD9D6BE, 0xA5A391, 0x666459, 0x262521),
	PACK15_4(0xD9D6BE, 0xA5A391, 0x666459, 0x262521)
};

static const unsigned short twb64_191_demons_gold[] = {
	PACK15_4(0xBAAF56, 0x8E8541, 0x575228, 0x201E0F),
	PACK15_4(0xBAAF56, 0x8E8541, 0x575228, 0x201E0F),
	PACK15_4(0xBAAF56, 0x8E8541, 0x575228, 0x201E0F)
};

static const unsigned short twb64_192_sega_tokyo_blue[] = {
	PACK15_4(0x0082D4, 0x0063A2, 0x003D63, 0x001625),
	PACK15_4(0x0082D4, 0x0063A2, 0x003D63, 0x001625),
	PACK15_4(0x0082D4, 0x0063A2, 0x003D63, 0x001625)
};

static const unsigned short twb64_193_champion_blue[] = {
	PACK15_4(0x009EDD, 0x0078A9, 0x004A68, 0x001B27),
	PACK15_4(0x009EDD, 0x0078A9, 0x004A68, 0x001B27),
	PACK15_4(0x009EDD, 0x0078A9, 0x004A68, 0x001B27)
};

static const unsigned short twb64_194_dk_barrel_brown[] = {
	PACK15_4(0xC3742F, 0x955823, 0x5B3616, 0x221408),
	PACK15_4(0xC3742F, 0x955823, 0x5B3616, 0x221408),
	PACK15_4(0xC3742F, 0x955823, 0x5B3616, 0x221408)
};

static const unsigned short twb64_195_evangelion_green[] = {
	PACK15_4(0x54CF54, 0x409E40, 0x276127, 0x0E240E),
	PACK15_4(0x54CF54, 0x409E40, 0x276127, 0x0E240E),
	PACK15_4(0x54CF54, 0x409E40, 0x276127, 0x0E240E)
};

static const unsigned short twb64_196_equestrian_purple[] = {
	PACK15_4(0xA672B0, 0x7E5786, 0x4E3552, 0x1D141F),
	PACK15_4(0xA672B0, 0x7E5786, 0x4E3552, 0x1D141F),
	PACK15_4(0xA672B0, 0x7E5786, 0x4E3552, 0x1D141F)
};

static const unsigned short twb64_197_autobot_red[] = {
	PACK15_4(0xC31F3C, 0x95172D, 0x5B0E1C, 0x22050A),
	PACK15_4(0xC31F3C, 0x95172D, 0x5B0E1C, 0x22050A),
	PACK15_4(0xC31F3C, 0x95172D, 0x5B0E1C, 0x22050A)
};

static const unsigned short twb64_198_niconico_sea_green[] = {
	PACK15_4(0x19C3A4, 0x13957D, 0x0B5B4D, 0x04221C),
	PACK15_4(0x19C3A4, 0x13957D, 0x0B5B4D, 0x04221C),
	PACK15_4(0x19C3A4, 0x13957D, 0x0B5B4D, 0x04221C)
};

static const unsigned short twb64_199_duracell_copper[] = {
	PACK15_4(0xC8895D, 0x986847, 0x5E402B, 0x231810),
	PACK15_4(0xC8895D, 0x986847, 0x5E402B, 0x231810),
	PACK15_4(0xC8895D, 0x986847, 0x5E402B, 0x231810)
};

static const unsigned short twb64_200_tokyo_skytree_cloudy_blue[] = {
	PACK15_4(0x82B5C7, 0x638A98, 0x3D555D, 0x161F23),
	PACK15_4(0x82B5C7, 0x638A98, 0x3D555D, 0x161F23),
	PACK15_4(0x82B5C7, 0x638A98, 0x3D555D, 0x161F23)
};

#undef PACK15_4
#undef PACK15_1
#undef TO5BIT

struct GbcPaletteEntry { const char *title; const unsigned short *p; };

static const GbcPaletteEntry gbcDirPalettes[] = {
	{ "GB - DMG", gbdmg },    // Original Game Boy
	{ "GB - Light", gblit },  // Original Game Boy Light
	{ "GB - Pocket", gbpoc }, // Original Game Boy Pocket
	{ "GBC - Blue", p518 },       // Left
	{ "GBC - Brown", p012 },      // Up
	{ "GBC - Dark Blue", p50D },  // A + Left
	{ "GBC - Dark Brown", p319 }, // B + Up
	{ "GBC - Dark Green", p31C }, // A + Right (default GBC)
	{ "GBC - Grayscale", p016 },  // B + Left
	{ "GBC - Green", p005 },      // Right
	{ "GBC - Inverted", p013 },   // B + Right
	{ "GBC - Orange", p007 },     // A + Down
	{ "GBC - Pastel Mix", p017 }, // Down
	{ "GBC - Red", p510 },        // A + Up
	{ "GBC - Yellow", p51A },     // B + Down
	{ "SGB - 1A", sgb1A }, // 1-A (default SGB)
	{ "SGB - 1B", sgb1B }, // (NB: don't think these
	{ "SGB - 1C", sgb1C }, // palettes have 'official'
	{ "SGB - 1D", sgb1D }, // names -> leave as colour
	{ "SGB - 1E", sgb1E }, // codes...)
	{ "SGB - 1F", sgb1F },
	{ "SGB - 1G", sgb1G },
	{ "SGB - 1H", sgb1H },
	{ "SGB - 2A", sgb2A },
	{ "SGB - 2B", sgb2B },
	{ "SGB - 2C", sgb2C },
	{ "SGB - 2D", sgb2D },
	{ "SGB - 2E", sgb2E },
	{ "SGB - 2F", sgb2F },
	{ "SGB - 2G", sgb2G },
	{ "SGB - 2H", sgb2H },
	{ "SGB - 3A", sgb3A },
	{ "SGB - 3B", sgb3B },
	{ "SGB - 3C", sgb3C },
	{ "SGB - 3D", sgb3D },
	{ "SGB - 3E", sgb3E },
	{ "SGB - 3F", sgb3F },
	{ "SGB - 3G", sgb3G },
	{ "SGB - 3H", sgb3H },
	{ "SGB - 4A", sgb4A },
	{ "SGB - 4B", sgb4B },
	{ "SGB - 4C", sgb4C },
	{ "SGB - 4D", sgb4D },
	{ "SGB - 4E", sgb4E },
	{ "SGB - 4F", sgb4F },
	{ "SGB - 4G", sgb4G },
	{ "SGB - 4H", sgb4H },
	{ "Special 1", pExt1 },
	{ "Special 2", pExt2 },
	{ "Special 3", pExt3 },
	{ "Special 4 (TI-83 Legacy)", pExt4 },
	{ "TWB64 001 - Aqours Blue", twb64_001_aqours_blue },
	{ "TWB64 002 - Anime Expo Ver.", twb64_002_anime_expo_ver },
	{ "TWB64 003 - SpongeBob Yellow", twb64_003_spongebob_yellow },
	{ "TWB64 004 - Patrick Star Pink", twb64_004_patrick_star_pink },
	{ "TWB64 005 - Neon Red", twb64_005_neon_red },
	{ "TWB64 006 - Neon Blue", twb64_006_neon_blue },
	{ "TWB64 007 - Neon Yellow", twb64_007_neon_yellow },
	{ "TWB64 008 - Neon Green", twb64_008_neon_green },
	{ "TWB64 009 - Neon Pink", twb64_009_neon_pink },
	{ "TWB64 010 - Mario Red", twb64_010_mario_red },
	{ "TWB64 011 - Nick Orange", twb64_011_nick_orange },
	{ "TWB64 012 - Virtual Boy Ver.", twb64_012_virtual_boy_ver },
	{ "TWB64 013 - Golden Wild", twb64_013_golden_wild },
	{ "TWB64 014 - Builder Yellow", twb64_014_builder_yellow },
	{ "TWB64 015 - Classic Blurple", twb64_015_classic_blurple },
	{ "TWB64 016 - 765 Production Ver.", twb64_016_765_production_ver },
	{ "TWB64 017 - Superball Ivory", twb64_017_superball_ivory },
	{ "TWB64 018 - Crunchyroll Orange", twb64_018_crunchyroll_orange },
	{ "TWB64 019 - Muse Pink", twb64_019_muse_pink },
	{ "TWB64 020 - Nijigasaki Yellow", twb64_020_nijigasaki_yellow },
	{ "TWB64 021 - Gamate Ver.", twb64_021_gamate_ver },
	{ "TWB64 022 - Greenscale Ver.", twb64_022_greenscale_ver },
	{ "TWB64 023 - Odyssey Gold", twb64_023_odyssey_gold },
	{ "TWB64 024 - Super Saiyan God", twb64_024_super_saiyan_god },
	{ "TWB64 025 - Super Saiyan Blue", twb64_025_super_saiyan_blue },
	{ "TWB64 026 - Bizarre Pink", twb64_026_bizarre_pink },
	{ "TWB64 027 - Nintendo Switch Lite Ver.", twb64_027_nintendo_switch_lite_ver },
	{ "TWB64 028 - Game.com Ver.", twb64_028_game_com_ver },
	{ "TWB64 029 - Sanrio Pink", twb64_029_sanrio_pink },
	{ "TWB64 030 - BANDAI NAMCO Ver.", twb64_030_bandai_namco_ver },
	{ "TWB64 031 - Cosmo Green", twb64_031_cosmo_green },
	{ "TWB64 032 - Wanda Pink", twb64_032_wanda_pink },
	{ "TWB64 033 - Link's Awakening DX Ver.", twb64_033_links_awakening_dx_ver },
	{ "TWB64 034 - Travel Wood", twb64_034_travel_wood },
	{ "TWB64 035 - Pokemon Ver.", twb64_035_pokemon_ver },
	{ "TWB64 036 - Game Grump Orange", twb64_036_game_grump_orange },
	{ "TWB64 037 - Scooby-Doo Mystery Ver.", twb64_037_scooby_doo_mystery_ver },
	{ "TWB64 038 - Pokemon mini Ver.", twb64_038_pokemon_mini_ver },
	{ "TWB64 039 - Supervision Ver.", twb64_039_supervision_ver },
	{ "TWB64 040 - DMG Ver.", twb64_040_dmg_ver },
	{ "TWB64 041 - Pocket Ver.", twb64_041_pocket_ver },
	{ "TWB64 042 - Light Ver.", twb64_042_light_ver },
	{ "TWB64 043 - Miraitowa Blue", twb64_043_miraitowa_blue },
	{ "TWB64 044 - Someity Pink", twb64_044_someity_pink },
	{ "TWB64 045 - Pikachu Yellow", twb64_045_pikachu_yellow },
	{ "TWB64 046 - Eevee Brown", twb64_046_eevee_brown },
	{ "TWB64 047 - Microvision Ver.", twb64_047_microvision_ver },
	{ "TWB64 048 - TI-83 Ver.", twb64_048_ti83_ver },
	{ "TWB64 049 - Aegis Cherry", twb64_049_aegis_cherry },
	{ "TWB64 050 - Labo Fawn", twb64_050_labo_fawn },
	{ "TWB64 051 - MILLION LIVE GOLD!", twb64_051_million_live_gold },
	{ "TWB64 052 - Tokyo Midtown Ver.", twb64_052_tokyo_midtown_ver },
	{ "TWB64 053 - VMU Ver.", twb64_053_vmu_ver },
	{ "TWB64 054 - Game Master Ver.", twb64_054_game_master_ver },
	{ "TWB64 055 - Android Green", twb64_055_android_green },
	{ "TWB64 056 - Ticketmaster Azure", twb64_056_ticketmaster_azure },
	{ "TWB64 057 - Google Red", twb64_057_google_red },
	{ "TWB64 058 - Google Blue", twb64_058_google_blue },
	{ "TWB64 059 - Google Yellow", twb64_059_google_yellow },
	{ "TWB64 060 - Google Green", twb64_060_google_green },
	{ "TWB64 061 - WonderSwan Ver.", twb64_061_wonderswan_ver },
	{ "TWB64 062 - Neo Geo Pocket Ver.", twb64_062_neo_geo_pocket_ver },
	{ "TWB64 063 - Dew Green", twb64_063_dew_green },
	{ "TWB64 064 - Coca-Cola Red", twb64_064_coca_cola_red },
	{ "TWB64 065 - GameKing Ver.", twb64_065_gameking_ver },
	{ "TWB64 066 - Do The Dew Ver.", twb64_066_do_the_dew_ver },
	{ "TWB64 067 - Digivice Ver.", twb64_067_digivice_ver },
	{ "TWB64 068 - Bikini Bottom Ver.", twb64_068_bikini_bottom_ver },
	{ "TWB64 069 - Blossom Pink", twb64_069_blossom_pink },
	{ "TWB64 070 - Bubbles Blue", twb64_070_bubbles_blue },
	{ "TWB64 071 - Buttercup Green", twb64_071_buttercup_green },
	{ "TWB64 072 - NASCAR Ver.", twb64_072_nascar_ver },
	{ "TWB64 073 - Lemon-Lime Green", twb64_073_lemon_lime_green },
	{ "TWB64 074 - Mega Man V Ver.", twb64_074_mega_man_v_ver },
	{ "TWB64 075 - Tamagotchi Ver.", twb64_075_tamagotchi_ver },
	{ "TWB64 076 - Phantom Red", twb64_076_phantom_red },
	{ "TWB64 077 - Halloween Ver.", twb64_077_halloween_ver },
	{ "TWB64 078 - Christmas Ver.", twb64_078_christmas_ver },
	{ "TWB64 079 - Cardcaptor Pink", twb64_079_cardcaptor_pink },
	{ "TWB64 080 - Pretty Guardian Gold", twb64_080_pretty_guardian_gold },
	{ "TWB64 081 - Camouflage Ver.", twb64_081_camoflauge_ver },
	{ "TWB64 082 - Legendary Super Saiyan", twb64_082_legendary_super_saiyan },
	{ "TWB64 083 - Super Saiyan Rose", twb64_083_super_saiyan_rose },
	{ "TWB64 084 - Super Saiyan", twb64_084_super_saiyan },
	{ "TWB64 085 - Mastered Ultra Instinct", twb64_085_mastered_ultra_instinct },
	{ "TWB64 086 - Saint Snow Red", twb64_086_saint_snow_red },
	{ "TWB64 087 - Yellow Banana", twb64_087_yellow_banana },
	{ "TWB64 088 - Green Banana", twb64_088_green_banana },
	{ "TWB64 089 - Super Saiyan 3", twb64_089_super_saiyan_3 },
	{ "TWB64 090 - Super Saiyan Blue Evolved", twb64_090_super_saiyan_blue_evolved },
	{ "TWB64 091 - Pocket Tales Ver.", twb64_091_pocket_tales_ver },
	{ "TWB64 092 - Investigation Yellow", twb64_092_investigation_yellow },
	{ "TWB64 093 - S.E.E.S. Blue", twb64_093_sees_blue },
	{ "TWB64 094 - Game Awards Cyan", twb64_094_game_awards_cyan },
	{ "TWB64 095 - Hokage Orange", twb64_095_hokage_orange },
	{ "TWB64 096 - Straw Hat Red", twb64_096_straw_hat_red },
	{ "TWB64 097 - Sword Art Cyan", twb64_097_sword_art_cyan },
	{ "TWB64 098 - Deku Alpha Emerald", twb64_098_deku_alpha_emerald },
	{ "TWB64 099 - Blue Stripes Ver.", twb64_099_blue_stripes_ver },
	{ "TWB64 100 - Stone Orange", twb64_100_stone_orange },
	{ "TWB64 101 - 765PRO Pink", twb64_101_765pro_pink },
	{ "TWB64 102 - CINDERELLA Blue", twb64_102_cinderella_blue },
	{ "TWB64 103 - MILLION Yellow!", twb64_103_million_yellow },
	{ "TWB64 104 - SideM Green", twb64_104_sidem_green },
	{ "TWB64 105 - SHINY Sky Blue", twb64_105_shiny_sky_blue },
	{ "TWB64 106 - Angry Volcano Ver.", twb64_106_angry_volcano_ver },
	{ "TWB64 107 - Yo-kai Pink", twb64_107_yokai_pink },
	{ "TWB64 108 - Yo-kai Green", twb64_108_yokai_green },
	{ "TWB64 109 - Yo-kai Blue", twb64_109_yokai_blue },
	{ "TWB64 110 - Yo-kai Purple", twb64_110_yokai_purple },
	{ "TWB64 111 - Aquatic Iro", twb64_111_aquatic_iro },
	{ "TWB64 112 - Tea Midori", twb64_112_tea_midori },
	{ "TWB64 113 - Sakura Pink", twb64_113_sakura_pink },
	{ "TWB64 114 - Wisteria Murasaki", twb64_114_wisteria_murasaki },
	{ "TWB64 115 - Oni Aka", twb64_115_oni_aka },
	{ "TWB64 116 - Golden Kiiro", twb64_116_golden_kiiro },
	{ "TWB64 117 - Silver Shiro", twb64_117_silver_shiro },
	{ "TWB64 118 - Fruity Orange", twb64_118_fruity_orange },
	{ "TWB64 119 - AKB48 Pink", twb64_119_akb48_pink },
	{ "TWB64 120 - Miku Blue", twb64_120_miku_blue },
	{ "TWB64 121 - Fairy Tail Red", twb64_121_fairy_tail_red },
	{ "TWB64 122 - Survey Corps Brown", twb64_122_survey_corps_brown },
	{ "TWB64 123 - Island Green", twb64_123_island_green },
	{ "TWB64 124 - Mania Plus Green", twb64_124_mania_plus_green },
	{ "TWB64 125 - Ninja Turtle Green", twb64_125_ninja_turtle_green },
	{ "TWB64 126 - Slime Blue", twb64_126_slime_blue },
	{ "TWB64 127 - Lime Midori", twb64_127_lime_midori },
	{ "TWB64 128 - Ghostly Aoi", twb64_128_ghostly_aoi },
	{ "TWB64 129 - Retro Bogeda", twb64_129_retro_bogeda },
	{ "TWB64 130 - Royal Blue", twb64_130_royal_blue },
	{ "TWB64 131 - Neon Purple", twb64_131_neon_purple },
	{ "TWB64 132 - Neon Orange", twb64_132_neon_orange },
	{ "TWB64 133 - Moonlight Vision", twb64_133_moonlight_vision },
	{ "TWB64 134 - Tokyo Red", twb64_134_tokyo_red },
	{ "TWB64 135 - Paris Gold", twb64_135_paris_gold },
	{ "TWB64 136 - Beijing Blue", twb64_136_beijing_blue },
	{ "TWB64 137 - Pac-Man Yellow", twb64_137_pacman_yellow },
	{ "TWB64 138 - Irish Green", twb64_138_irish_green },
	{ "TWB64 139 - Kakarot Orange", twb64_139_kakarot_orange },
	{ "TWB64 140 - Dragon Ball Orange", twb64_140_dragon_ball_orange },
	{ "TWB64 141 - Christmas Gold", twb64_141_christmas_gold },
	{ "TWB64 142 - Pepsi Blue", twb64_142_pepsi_blue },
	{ "TWB64 143 - Bubblun Green", twb64_143_bubblun_green },
	{ "TWB64 144 - Bobblun Blue", twb64_144_bobblun_blue },
	{ "TWB64 145 - Baja Blast Storm", twb64_145_baja_blast_storm },
	{ "TWB64 146 - Olympic Gold", twb64_146_olympic_gold },
	{ "TWB64 147 - Value Orange", twb64_147_value_orange },
	{ "TWB64 148 - Liella Purple!", twb64_148_liella_purple },
	{ "TWB64 149 - Olympic Silver", twb64_149_olympic_silver },
	{ "TWB64 150 - Olympic Bronze", twb64_150_olympic_bronze },
	{ "TWB64 151 - ANA Sky Blue", twb64_151_ana_sky_blue },
	{ "TWB64 152 - Nijigasaki Orange", twb64_152_nijigasaki_orange },
	{ "TWB64 153 - HoloBlue", twb64_153_holoblue },
	{ "TWB64 154 - Wrestling Red", twb64_154_wrestling_red },
	{ "TWB64 155 - Yoshi Egg Green", twb64_155_yoshi_egg_green },
	{ "TWB64 156 - Pokedex Red", twb64_156_pokedex_red },
	{ "TWB64 157 - Disney Dream Blue", twb64_157_disney_dream_blue },
	{ "TWB64 158 - Xbox Green", twb64_158_xbox_green },
	{ "TWB64 159 - Sonic Mega Blue", twb64_159_sonic_mega_blue },
	{ "TWB64 160 - G4 Orange", twb64_160_g4_orange },
	{ "TWB64 161 - Scarlett Green", twb64_161_scarlett_green },
	{ "TWB64 162 - Glitchy Blue", twb64_162_glitchy_blue },
	{ "TWB64 163 - Classic LCD", twb64_163_classic_lcd },
	{ "TWB64 164 - 3DS Virtual Console Ver.", twb64_164_3ds_virtual_console_ver },
	{ "TWB64 165 - PocketStation Ver.", twb64_165_pocketstation_ver },
	{ "TWB64 166 - Game and Gold", twb64_166_game_and_gold },
	{ "TWB64 167 - Smurfy Blue", twb64_167_smurfy_blue },
	{ "TWB64 168 - Swampy Ogre Green", twb64_168_swampy_ogre_green },
	{ "TWB64 169 - Sailor Spinach Green", twb64_169_sailor_spinach_green },
	{ "TWB64 170 - Shenron Green", twb64_170_shenron_green },
	{ "TWB64 171 - Berserk Blood", twb64_171_berserk_blood },
	{ "TWB64 172 - Super Star Pink", twb64_172_super_star_pink },
	{ "TWB64 173 - Gamebuino Classic Ver.", twb64_173_gamebuino_classic_ver },
	{ "TWB64 174 - Barbie Pink", twb64_174_barbie_pink },
	{ "TWB64 175 - Star Command Green", twb64_175_star_command_green },
	{ "TWB64 176 - Nokia 3310 Ver.", twb64_176_nokia_3310_ver },
	{ "TWB64 177 - Clover Green", twb64_177_clover_green },
	{ "TWB64 178 - Crash Orange", twb64_178_crash_orange },
	{ "TWB64 179 - Famicom Disk Yellow", twb64_179_famicom_disk_yellow },
	{ "TWB64 180 - Team Rocket Red", twb64_180_team_rocket_red },
	{ "TWB64 181 - SEIKO Timer Yellow", twb64_181_seiko_timer_yellow },
	{ "TWB64 182 - PINK109", twb64_182_pink109 },
	{ "TWB64 183 - Doraemon Blue", twb64_183_doraemon_blue },
	{ "TWB64 184 - Fury Blue", twb64_184_fury_blue },
	{ "TWB64 185 - Rockstar Orange", twb64_185_rockstar_orange },
	{ "TWB64 186 - Puyo Puyo Green", twb64_186_puyo_puyo_green },
	{ "TWB64 187 - Susan G. Pink", twb64_187_susan_g_pink },
	{ "TWB64 188 - Pizza Hut Red", twb64_188_pizza_hut_red },
	{ "TWB64 189 - Plumbob Green", twb64_189_plumbob_green },
	{ "TWB64 190 - Grand Ivory", twb64_190_grand_ivory },
	{ "TWB64 191 - Demon's Gold", twb64_191_demons_gold },
	{ "TWB64 192 - SEGA Tokyo Blue", twb64_192_sega_tokyo_blue },
	{ "TWB64 193 - Champion Blue", twb64_193_champion_blue },
	{ "TWB64 194 - DK Barrel Brown", twb64_194_dk_barrel_brown },
	{ "TWB64 195 - Evangelion Green", twb64_195_evangelion_green },
	{ "TWB64 196 - Equestrian Purple", twb64_196_equestrian_purple },
	{ "TWB64 197 - Autobot Red", twb64_197_autobot_red },
	{ "TWB64 198 - Niconico Sea Green", twb64_198_niconico_sea_green },
	{ "TWB64 199 - Duracell Copper", twb64_199_duracell_copper },
	{ "TWB64 200 - TOKYO SKYTREE CLOUDY BLUE", twb64_200_tokyo_skytree_cloudy_blue },
};

static const GbcPaletteEntry gbcTitlePalettes[] = {
	{ "ALLEY WAY", p008 },
	{ "ASTEROIDS", p30E },	// unofficial ("ASTEROIDS/MISCMD" alt.)
	{ "ASTEROIDS/MISCMD", p30E },
	{ "ATOMIC PUNK", p30F },	// unofficial ("DYNABLASTER" alt.)
	{ "BA.TOSHINDEN", p50F },
	{ "BALLOON KID", p006 },
	{ "BASEBALL", p503 },
	{ "BATTLETOADS", p312 },	// unofficial
	{ "BOMBER MAN GB", p31C },	// unofficial ("WARIO BLAST" alt.)
	{ "BOY AND BLOB GB1", p512 },
	{ "BOY AND BLOB GB2", p512 },
	{ "BT2RAGNAROKWORLD", p312 },
	{ "CENTIPEDE", p31C },	// unofficial ("MILLI/CENTI/PEDE" alt.)
	{ "DEFENDER/JOUST", p50F },
	{ "DMG FOOTBALL", p30E },
	{ "DONKEY KONG", p306 },
	{ "DONKEYKONGLAND", p50C },
	{ "DONKEYKONGLAND 2", p50C },
	{ "DONKEYKONGLAND 3", p50C },
	{ "DONKEYKONGLAND95", p501 },
	{ "DR.MARIO", p20B },
	{ "DYNABLASTER", p30F },
	{ "EMPIRE STRIKES", p512 },	// unofficial
	{ "F1RACE", p012 },
	{ "FOOTBALL INT'L", p502 },	// unofficial ("SOCCER" alt.)
	{ "G&W GALLERY", p304 },
	{ "GALAGA&GALAXIAN", p013 },
	{ "GAME&WATCH", p012 },
	{ "Game and Watch 2", p304 },
	{ "GAMEBOY GALLERY", p304 },
	{ "GAMEBOY GALLERY2", p304 },
	{ "GBWARS", p500 },
	{ "GBWARST", p500 },	// unofficial ("GBWARS" alt.)
	{ "GOLF", p30E },
	{ "HOSHINOKA-BI", p508 },
	{ "JAMES  BOND  007", p11C },
	{ "KAERUNOTAMENI", p10D },
	{ "KEN GRIFFEY JR", p31C },
	{ "KID ICARUS", p30D },
	{ "KILLERINSTINCT95", p50D },
	{ "KINGOFTHEZOO", p30F },
	{ "KIRAKIRA KIDS", p012 },
	{ "KIRBY BLOCKBALL", p508 },
	{ "KIRBY DREAM LAND", p508 },
	{ "KIRBY'S PINBALL", p308 },
	{ "KIRBY2", p508 },
	{ "LOLO", p50F },	// unofficial ("LOLO2" alt.)
	{ "LOLO2", p50F },
	{ "MAGNETIC SOCCER", p50E },
	{ "MANSELL", p012 },
	{ "MARIO & YOSHI", p305 },
	{ "MARIO'S PICROSS", p012 },
	{ "MARIOLAND2", p509 },
	{ "MEGA MAN 2", p50F },
	{ "MEGAMAN", p50F },
	{ "MEGAMAN3", p50F },
	{ "MEGAMAN4", p50F },	// unofficial
	{ "MEGAMAN5", p50F },	// unofficial
	{ "METROID2", p514 },
	{ "MILLI/CENTI/PEDE", p31C },
	{ "MISSILE COMMAND", p30E },	// unofficial ("ASTEROIDS/MISCMD" alt.)
	{ "MOGURANYA", p300 },
	{ "MYSTIC QUEST", p50E },
	{ "NETTOU KOF 95", p50F },
	{ "NETTOU KOF 96", p50F },	// unofficial
	{ "NETTOU TOSHINDEN", p50F },	// unofficial ("BA.TOSHINDEN" alt.)
	{ "NEW CHESSMASTER", p30F },
	{ "OTHELLO", p50E },
	{ "PAC-IN-TIME", p51C },
	{ "PENGUIN WARS", p30F },	// unofficial ("KINGOFTHEZOO" alt.)
	{ "PENGUINKUNWARSVS", p30F },	// unofficial ("KINGOFTHEZOO" alt.)
	{ "PICROSS 2", p012 },
	{ "PINOCCHIO", p20C },
	{ "POKEBOM", p30C },
	{ "POKEMON BLUE", p10B },
	{ "POKEMON GREEN", p11C },
	{ "POKEMON RED", p110 },
	{ "POKEMON YELLOW", p007 },
	{ "QIX", p407 },
	{ "RADARMISSION", p100 },
	{ "ROCKMAN WORLD", p50F },
	{ "ROCKMAN WORLD2", p50F },
	{ "ROCKMANWORLD3", p50F },
	{ "ROCKMANWORLD4", p50F },	// unofficial
	{ "ROCKMANWORLD5", p50F },	// unofficial
	{ "SEIKEN DENSETSU", p50E },
	{ "SOCCER", p502 },
	{ "SOLARSTRIKER", p013 },
	{ "SPACE INVADERS", p013 },
	{ "STAR STACKER", p012 },
	{ "STAR WARS", p512 },
	{ "STAR WARS-NOA", p512 },
	{ "STREET FIGHTER 2", p50F },
	{ "SUPER BOMBLISS  ", p006 },	// unofficial ("TETRIS BLAST" alt.)
	{ "SUPER MARIOLAND", p30A },
	{ "SUPER RC PRO-AM", p50F },
	{ "SUPERDONKEYKONG", p501 },
	{ "SUPERMARIOLAND3", p500 },
	{ "TENNIS", p502 },
	{ "TETRIS", p007 },
	{ "TETRIS ATTACK", p405 },
	{ "TETRIS BLAST", p006 },
	{ "TETRIS FLASH", p407 },
	{ "TETRIS PLUS", p31C },
	{ "TETRIS2", p407 },
	{ "THE CHESSMASTER", p30F },
	{ "TOPRANKINGTENNIS", p502 },
	{ "TOPRANKTENNIS", p502 },
	{ "TOY STORY", p30E },
	{ "VEGAS STAKES", p50E },
	{ "WARIO BLAST", p31C },
	{ "WARIOLAND2", p515 },
	{ "WAVERACE", p50B },
	{ "WORLD CUP", p30E },
	{ "X", p016 },
	{ "YAKUMAN", p012 },
	{ "YOSHI'S COOKIE", p406 },
	{ "YOSSY NO COOKIE", p406 },
	{ "YOSSY NO PANEPON", p405 },
	{ "YOSSY NO TAMAGO", p305 },
	{ "ZELDA", p511 },
};

static const GbcPaletteEntry sgbTitlePalettes[] = {
	{ "ALLEY WAY", sgb3F },
	{ "BALLOON KID", sgb1A },	// unofficial ("BALLÃ´Ã´N KID" alt.)
	{ "BALLÃ´Ã´N KID", sgb1A },
	{ "BASEBALL", sgb2G },
	{ "CASINO FUNPAK", sgb1A },	// unofficial (Nintendo Power Issue #67)
	{ "CONTRA ALIEN WAR", sgb1F },	// unofficial (Nintendo Power Issue #66)
	{ "CONTRA SPIRITS", sgb1F },	// unofficial ("CONTRA ALIEN WAR" alt.)
	{ "CUTTHROAT ISLAND", sgb3E },	// unofficial (Nintendo Power Issue #82)
	{ "DMG FOOTBALL", sgb4B },	// unofficial
	{ "DR.MARIO", sgb3B },
	{ "F1RACE", sgb4B },	// unofficial
	{ "FRANK THOMAS BB", sgb1B },	// unofficial (Nintendo Power Issue #80)
	{ "GBWARS", sgb3E },
	{ "GBWARST", sgb3E },	// unofficial ("GBWARS" alt.)
	{ "GOLF", sgb3H },
	{ "HOSHINOKA-BI", sgb2C },
	{ "ITCHY & SCRATCHY", sgb4B },	// unofficial (Nintendo Power Issue #63)
	{ "JEOPARDY", sgb2F },	// unofficial
	{ "JEOPARDY SPORTS", sgb2F },	// unofficial (Nintendo Power Issue #62)
	{ "JUNGLE BOOK", sgb4B },	// unofficial (Nintendo Power Issue #62)
	{ "KAERUNOTAMENI", sgb2A },
	{ "KID ICARUS", sgb2F },
	{ "KIRBY BLOCKBALL", sgb4D },	// unofficial (Nintendo Power Issue #83)
	{ "KIRBY DREAM LAND", sgb2C },
	{ "KIRBY'S PINBALL", sgb1C },
	{ "MARIO & YOSHI", sgb2D },
	{ "MARIOLAND2", sgb3D },
	{ "METROID2", sgb4G },
	{ "MORTAL KOMBAT", sgb4D },	// unofficial
	{ "MORTAL KOMBAT 3", sgb1B },	// unofficial (Nintendo Power Issue #79)
	{ "MORTAL KOMBAT II", sgb4D },	// unofficial (Nintendo Power Issue #65)
	{ "MORTALKOMBAT DUO", sgb4D },	// unofficial
	{ "MORTALKOMBATI&II", sgb4D },	// unofficial
	{ "NBA JAM", sgb2F },	// unofficial (Nintendo Power Issue #68)
	{ "NBA JAM TE", sgb2F },	// unofficial (Nintendo Power Issue #68)
	{ "PLAT JEOPARDY!", sgb2F },	// unofficial
	{ "POCAHONTAS", sgb4D },	// unofficial (Nintendo Power Issue #83)
	{ "PROBOTECTOR 2", sgb1F },	// unofficial ("CONTRA ALIEN WAR" alt.)
	{ "QIX", sgb4A },
	{ "RADARMISSION", sgb4B},	// unofficial
	{ "RVT             ", sgb4B },	// unofficial (Nintendo Power Issue #63)
	{ "SOLARSTRIKER", sgb1G },
	{ "SPACE INVADERS", sgb4D },	// unofficial (Nintendo Power Issue #62)
	{ "SUPER MARIOLAND", sgb1F },
	{ "SUPERMARIOLAND3", sgb1B },
	{ "TARZAN", sgb2A },	// unofficial (Nintendo Power Issue #62)
	{ "TAZ-MANIA", sgb1A },	// unofficial (Nintendo Power Issue #64)
	{ "TEEN JEOPARDY!", sgb2F },	// unofficial
	{ "TENNIS", sgb3G },
	{ "TETRIS", sgb3A },
	{ "TETRIS FLASH", sgb2B },	// unofficial ("TETRIS2" alt.)
	{ "TETRIS2", sgb2B },	// unofficial
	{ "THE GETAWAY     ", sgb1B },	// unofficial (Nintendo Power Issue #80)
	{ "TOPRANKINGTENNIS", sgb4B },	// unofficial
	{ "TOPRANKTENNIS", sgb4B },	// unofficial
	{ "WAVERACE", sgb4C },	// unofficial
	{ "WORLD CUP", sgb4H },	// unofficial
	{ "X", sgb4D },
	{ "YAKUMAN", sgb3C },
	{ "YOGIS GOLDRUSH", sgb3B },	// unofficial (Nintendo Power Issue #65)
	{ "YOSHI'S COOKIE", sgb1D },
	{ "YOSSY NO COOKIE", sgb1D },
	{ "YOSSY NO TAMAGO", sgb2D },
	{ "ZELDA", sgb1E },
};

static const unsigned short **gbcDirPaletteMap   = NULL;
static const unsigned short **gbcTitlePaletteMap = NULL;
static const unsigned short **sgbTitlePaletteMap = NULL;

static void initPaletteMaps(void)
{
	unsigned i;

	// gbcDirPalettes
	for (i = 0; i < (sizeof gbcDirPalettes) / (sizeof gbcDirPalettes[0]); i++)
		RHMAP_SET_STR(gbcDirPaletteMap, gbcDirPalettes[i].title, gbcDirPalettes[i].p);

	// gbcTitlePalettes
	for (i = 0; i < (sizeof gbcTitlePalettes) / (sizeof gbcTitlePalettes[0]); i++)
		RHMAP_SET_STR(gbcTitlePaletteMap, gbcTitlePalettes[i].title, gbcTitlePalettes[i].p);

	// sgbTitlePalettes
	for (i = 0; i < (sizeof sgbTitlePalettes) / (sizeof sgbTitlePalettes[0]); i++)
		RHMAP_SET_STR(sgbTitlePaletteMap, sgbTitlePalettes[i].title, sgbTitlePalettes[i].p);
}

static void freePaletteMaps(void)
{
	RHMAP_FREE(gbcDirPaletteMap);
	RHMAP_FREE(gbcTitlePaletteMap);
	RHMAP_FREE(sgbTitlePaletteMap);
}

static const unsigned short *findGbcDirPal(const char *const title)
{
	return RHMAP_GET_STR(gbcDirPaletteMap, title);
}

static const unsigned short *findGbcTitlePal(const char *const title)
{
	return RHMAP_GET_STR(gbcTitlePaletteMap, title);
}

static const unsigned short *findSgbTitlePal(const char *const title)
{
	return RHMAP_GET_STR(sgbTitlePaletteMap, title);
}

static const unsigned short *findGbcPal(const char *const title)
{
	const unsigned short *const pal = findGbcDirPal(title);

	if (pal)
		return pal;
	
	return findGbcTitlePal(title);
}

}
