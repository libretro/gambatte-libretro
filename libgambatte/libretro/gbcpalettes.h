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
// Palettes by TheWolfBunny64 (TheWolfBunny64 on DeviantArt)
// https://www.deviantart.com/thewolfbunny64/gallery/69987002/game-boy-palettes
//
static const unsigned short twb64_001_aqours_blue[] = {
	PACK15_4(0x00A0E9, 0x0080BA, 0x005074, 0x003045),
	PACK15_4(0x00A0E9, 0x0080BA, 0x005074, 0x003045),
	PACK15_4(0x00A0E9, 0x0080BA, 0x005074, 0x003045)
};

static const unsigned short twb64_002_anime_expo_ver[] = {
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D),
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D),
	PACK15_4(0xE5EAEB, 0x9BA3A6, 0x656E72, 0x242A2D)
};

static const unsigned short twb64_003_spongebob_yellow[] = {
	PACK15_4(0xF7E948, 0xC5BA39, 0x7B7424, 0x4A4515),
	PACK15_4(0xF7E948, 0xC5BA39, 0x7B7424, 0x4A4515),
	PACK15_4(0xF7E948, 0xC5BA39, 0x7B7424, 0x4A4515)
};

static const unsigned short twb64_004_patrick_star_pink[] = {
	PACK15_4(0xFF808B, 0xCC666F, 0x7F4045, 0x4C2629),
	PACK15_4(0xFF808B, 0xCC666F, 0x7F4045, 0x4C2629),
	PACK15_4(0xFF808B, 0xCC666F, 0x7F4045, 0x4C2629)
};

static const unsigned short twb64_005_neon_red[] = {
	PACK15_4(0xFF3C28, 0xCC3020, 0x7F1E14, 0x4C120C),
	PACK15_4(0xFF3C28, 0xCC3020, 0x7F1E14, 0x4C120C),
	PACK15_4(0xFF3C28, 0xCC3020, 0x7F1E14, 0x4C120C)
};

static const unsigned short twb64_006_neon_blue[] = {
	PACK15_4(0x0AB9E6, 0x0894B8, 0x055C73, 0x033745),
	PACK15_4(0x0AB9E6, 0x0894B8, 0x055C73, 0x033745),
	PACK15_4(0x0AB9E6, 0x0894B8, 0x055C73, 0x033745)
};

static const unsigned short twb64_007_neon_yellow[] = {
	PACK15_4(0xE6FF00, 0xB8CC00, 0x737F00, 0x454C00),
	PACK15_4(0xE6FF00, 0xB8CC00, 0x737F00, 0x454C00),
	PACK15_4(0xE6FF00, 0xB8CC00, 0x737F00, 0x454C00)
};

static const unsigned short twb64_008_neon_green[] = {
	PACK15_4(0x1EDC00, 0x18B000, 0x0F6E00, 0x094200),
	PACK15_4(0x1EDC00, 0x18B000, 0x0F6E00, 0x094200),
	PACK15_4(0x1EDC00, 0x18B000, 0x0F6E00, 0x094200)
};

static const unsigned short twb64_009_neon_pink[] = {
	PACK15_4(0xFF3278, 0xCC2860, 0x7F193C, 0x4C0F24),
	PACK15_4(0xFF3278, 0xCC2860, 0x7F193C, 0x4C0F24),
	PACK15_4(0xFF3278, 0xCC2860, 0x7F193C, 0x4C0F24)
};

static const unsigned short twb64_010_mario_red[] = {
	PACK15_4(0xE10F00, 0xB40C00, 0x700700, 0x430400),
	PACK15_4(0xE10F00, 0xB40C00, 0x700700, 0x430400),
	PACK15_4(0xE10F00, 0xB40C00, 0x700700, 0x430400)
};

static const unsigned short twb64_011_nick_orange[] = {
	PACK15_4(0xFF6700, 0xCC5200, 0x7F3300, 0x4C1E00),
	PACK15_4(0xFF6700, 0xCC5200, 0x7F3300, 0x4C1E00),
	PACK15_4(0xFF6700, 0xCC5200, 0x7F3300, 0x4C1E00)
};

static const unsigned short twb64_012_virtual_vision[] = {
	PACK15_4(0x4C0000, 0x7F0000, 0xCC0000, 0xFF0000),
	PACK15_4(0x1E0000, 0x780000, 0xC30000, 0xFF0000),
	PACK15_4(0x1E0000, 0x780000, 0xC30000, 0xFF0000)
};

static const unsigned short twb64_013_golden_wild[] = {
	PACK15_4(0xB99F65, 0x947F50, 0x5C4F32, 0x372F1E),
	PACK15_4(0xB99F65, 0x947F50, 0x5C4F32, 0x372F1E),
	PACK15_4(0xB99F65, 0x947F50, 0x5C4F32, 0x372F1E)
};

static const unsigned short twb64_014_dmg_099[] = {
	PACK15_4(0x84B510, 0x6BAD19, 0x3F642F, 0x313231),
	PACK15_4(0x84B510, 0x6BAD19, 0x3F642F, 0x313231),
	PACK15_4(0x84B510, 0x6BAD19, 0x3F642F, 0x313231)
};

static const unsigned short twb64_015_classic_blurple[] = {
	PACK15_4(0x7289DA, 0x5B6DAE, 0x38446D, 0x222941),
	PACK15_4(0x7289DA, 0x5B6DAE, 0x38446D, 0x222941),
	PACK15_4(0x7289DA, 0x5B6DAE, 0x38446D, 0x222941)
};

static const unsigned short twb64_016_765_production_ver[] = {
	PACK15_4(0xBBC4E4, 0x959CB6, 0x5D6272, 0x383A44),
	PACK15_4(0xBBC4E4, 0x959CB6, 0x5D6272, 0x383A44),
	PACK15_4(0xBBC4E4, 0x959CB6, 0x5D6272, 0x383A44)
};

static const unsigned short twb64_017_superball_ivory[] = {
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432),
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432),
	PACK15_4(0xEEF0BC, 0xBCBC8A, 0x828250, 0x646432)
};

static const unsigned short twb64_018_crunchyroll_orange[] = {
	PACK15_4(0xF47522, 0xC35D1B, 0x7A3A11, 0x49230A),
	PACK15_4(0xF47522, 0xC35D1B, 0x7A3A11, 0x49230A),
	PACK15_4(0xF47522, 0xC35D1B, 0x7A3A11, 0x49230A)
};

static const unsigned short twb64_019_muse_pink[] = {
	PACK15_4(0xE4007F, 0xB60065, 0x72003F, 0x440026),
	PACK15_4(0xE4007F, 0xB60065, 0x72003F, 0x440026),
	PACK15_4(0xE4007F, 0xB60065, 0x72003F, 0x440026)
};

static const unsigned short twb64_020_school_idol_blue[] = {
	PACK15_4(0xF9F9F8, 0x87ADF5, 0x3960D7, 0x283066),
	PACK15_4(0xF9F9F8, 0x87ADF5, 0x3960D7, 0x283066),
	PACK15_4(0xF9F9F8, 0x87ADF5, 0x3960D7, 0x283066)
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
	PACK15_4(0xC2A000, 0x9B8000, 0x615000, 0x3A3000),
	PACK15_4(0xC2A000, 0x9B8000, 0x615000, 0x3A3000),
	PACK15_4(0xC2A000, 0x9B8000, 0x615000, 0x3A3000)
};

static const unsigned short twb64_024_super_saiyan_god[] = {
	PACK15_4(0xD70362, 0xAC024E, 0x6B0131, 0x40001D),
	PACK15_4(0xD70362, 0xAC024E, 0x6B0131, 0x40001D),
	PACK15_4(0xD70362, 0xAC024E, 0x6B0131, 0x40001D)
};

static const unsigned short twb64_025_super_saiyan_blue[] = {
	PACK15_4(0x05BCCC, 0x0496A3, 0x025D66, 0x01383D),
	PACK15_4(0x05BCCC, 0x0496A3, 0x025D66, 0x01383D),
	PACK15_4(0x05BCCC, 0x0496A3, 0x025D66, 0x01383D)
};

static const unsigned short twb64_026_animax_blue[] = {
	PACK15_4(0x3499E8, 0x297AB9, 0x1A4C74, 0x0F2D45),
	PACK15_4(0x3499E8, 0x297AB9, 0x1A4C74, 0x0F2D45),
	PACK15_4(0x3499E8, 0x297AB9, 0x1A4C74, 0x0F2D45)
};

static const unsigned short twb64_027_bmo_ver[] = {
	PACK15_4(0xC0FFCC, 0x99CCA3, 0x607F66, 0x394C3D),
	PACK15_4(0xC0FFCC, 0x99CCA3, 0x607F66, 0x394C3D),
	PACK15_4(0xC0FFCC, 0x99CCA3, 0x607F66, 0x394C3D)
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

static const unsigned short twb64_030_timmy_turner_pink[] = {
	PACK15_4(0xBC486D, 0x963957, 0x5E2436, 0x381520),
	PACK15_4(0xBC486D, 0x963957, 0x5E2436, 0x381520),
	PACK15_4(0xBC486D, 0x963957, 0x5E2436, 0x381520)
};

static const unsigned short twb64_031_fairly_oddpalette[] = {
	PACK15_4(0x7BB850, 0xCE5A99, 0x7B365B, 0x3D1B2D),
	PACK15_4(0x7BB850, 0xCE5A99, 0x7B365B, 0x3D1B2D),
	PACK15_4(0x7BB850, 0xCE5A99, 0x7B365B, 0x3D1B2D)
};

static const unsigned short twb64_032_danny_phantom_silver[] = {
	PACK15_4(0xABBBCC, 0x8895A3, 0x555D66, 0x33383D),
	PACK15_4(0xABBBCC, 0x8895A3, 0x555D66, 0x33383D),
	PACK15_4(0xABBBCC, 0x8895A3, 0x555D66, 0x33383D)
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
	PACK15_4(0xE9762F, 0xBA5E25, 0x743B17, 0x45230E),
	PACK15_4(0xE9762F, 0xBA5E25, 0x743B17, 0x45230E),
	PACK15_4(0xE9762F, 0xBA5E25, 0x743B17, 0x45230E)
};

static const unsigned short twb64_037_scooby_doo_mystery_ver[] = {
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31),
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31),
	PACK15_4(0xC6DE31, 0xF79321, 0x8F59A5, 0x2A1A31)
};

static const unsigned short twb64_038_pokemon_mini_ver[] = {
	PACK15_4(0xA5BEA5, 0x849884, 0x525F52, 0x313931),
	PACK15_4(0xA5BEA5, 0x849884, 0x525F52, 0x313931),
	PACK15_4(0xA5BEA5, 0x849884, 0x525F52, 0x313931)
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

static const unsigned short twb64_043_all_might_hero_palette[] = {
	PACK15_4(0xEFF0F0, 0xF3DB43, 0xD12021, 0x212F79),
	PACK15_4(0xEFF0F0, 0xF3DB43, 0xD12021, 0x212F79),
	PACK15_4(0xEFF0F0, 0xF3DB43, 0xD12021, 0x212F79)
};

static const unsigned short twb64_044_ua_high_school_uniform[] = {
	PACK15_4(0xEDEDED, 0xA4AAAF, 0xA02929, 0x0C4856),
	PACK15_4(0xEDEDED, 0xA4AAAF, 0xA02929, 0x0C4856),
	PACK15_4(0xEDEDED, 0xA4AAAF, 0xA02929, 0x0C4856)
};

static const unsigned short twb64_045_pikachu_yellow[] = {
	PACK15_4(0xFFDC00, 0xCCB000, 0x7F6E00, 0x4C4200),
	PACK15_4(0xFFDC00, 0xCCB000, 0x7F6E00, 0x4C4200),
	PACK15_4(0xFFDC00, 0xCCB000, 0x7F6E00, 0x4C4200)
};

static const unsigned short twb64_046_eevee_brown[] = {
	PACK15_4(0xC88D32, 0xA07028, 0x644619, 0x3C2A0F),
	PACK15_4(0xC88D32, 0xA07028, 0x644619, 0x3C2A0F),
	PACK15_4(0xC88D32, 0xA07028, 0x644619, 0x3C2A0F)
};

static const unsigned short twb64_047_microvision_ver[] = {
	PACK15_4(0xA0A0A0, 0x808080, 0x505050, 0x303030),
	PACK15_4(0xA0A0A0, 0x808080, 0x505050, 0x303030),
	PACK15_4(0xA0A0A0, 0x808080, 0x505050, 0x303030)
};

static const unsigned short twb64_048_ti83_ver[] = {
	PACK15_4(0x9CAA8C, 0x7C8870, 0x4E5546, 0x2E332A),
	PACK15_4(0x9CAA8C, 0x7C8870, 0x4E5546, 0x2E332A),
	PACK15_4(0x9CAA8C, 0x7C8870, 0x4E5546, 0x2E332A)
};

static const unsigned short twb64_049_aegis_cherry[] = {
	PACK15_4(0xDD3B64, 0xB02F50, 0x6E1D32, 0x42111E),
	PACK15_4(0xDD3B64, 0xB02F50, 0x6E1D32, 0x42111E),
	PACK15_4(0xDD3B64, 0xB02F50, 0x6E1D32, 0x42111E)
};

static const unsigned short twb64_050_labo_fawn[] = {
	PACK15_4(0xD7AA73, 0xAC885C, 0x6B5539, 0x403322),
	PACK15_4(0xD7AA73, 0xAC885C, 0x6B5539, 0x403322),
	PACK15_4(0xD7AA73, 0xAC885C, 0x6B5539, 0x403322)
};

static const unsigned short twb64_051_million_live_gold[] = {
	PACK15_4(0xCDB261, 0xA48E4D, 0x665930, 0x3D351D),
	PACK15_4(0xCDB261, 0xA48E4D, 0x665930, 0x3D351D),
	PACK15_4(0xCDB261, 0xA48E4D, 0x665930, 0x3D351D)
};

static const unsigned short twb64_052_squidward_sea_foam_green[] = {
	PACK15_4(0xB9D7CD, 0x94ACA4, 0x5C6B66, 0x37403D),
	PACK15_4(0xB9D7CD, 0x94ACA4, 0x5C6B66, 0x37403D),
	PACK15_4(0xB9D7CD, 0x94ACA4, 0x5C6B66, 0x37403D)
};

static const unsigned short twb64_053_vmu_ver[] = {
	PACK15_4(0x88CCA8, 0x6CA386, 0x446654, 0x081480),
	PACK15_4(0x88CCA8, 0x6CA386, 0x446654, 0x081480),
	PACK15_4(0x88CCA8, 0x6CA386, 0x446654, 0x081480)
};

static const unsigned short twb64_054_game_master_ver[] = {
	PACK15_4(0x829FA6, 0x687F84, 0x414F53, 0x272F31),
	PACK15_4(0x829FA6, 0x687F84, 0x414F53, 0x272F31),
	PACK15_4(0x829FA6, 0x687F84, 0x414F53, 0x272F31)
};

static const unsigned short twb64_055_android_green[] = {
	PACK15_4(0x3DDC84, 0x30B069, 0x1E6E42, 0x124227),
	PACK15_4(0x3DDC84, 0x30B069, 0x1E6E42, 0x124227),
	PACK15_4(0x3DDC84, 0x30B069, 0x1E6E42, 0x124227)
};

static const unsigned short twb64_056_amazon_vision[] = {
	PACK15_4(0xFFFFFF, 0xFF9900, 0x008296, 0x252F3E),
	PACK15_4(0xFFFFFF, 0xFF9900, 0x008296, 0x252F3E),
	PACK15_4(0xFFFFFF, 0xFF9900, 0x008296, 0x252F3E)
};

static const unsigned short twb64_057_google_red[] = {
	PACK15_4(0xEA4335, 0xBB352A, 0x75211A, 0x46140F),
	PACK15_4(0xEA4335, 0xBB352A, 0x75211A, 0x46140F),
	PACK15_4(0xEA4335, 0xBB352A, 0x75211A, 0x46140F)
};

static const unsigned short twb64_058_google_blue[] = {
	PACK15_4(0x4285F4, 0x346AC3, 0x21427A, 0x132749),
	PACK15_4(0x4285F4, 0x346AC3, 0x21427A, 0x132749),
	PACK15_4(0x4285F4, 0x346AC3, 0x21427A, 0x132749)
};

static const unsigned short twb64_059_google_yellow[] = {
	PACK15_4(0xFBBC05, 0xC89604, 0x7D5E02, 0x4B3801),
	PACK15_4(0xFBBC05, 0xC89604, 0x7D5E02, 0x4B3801),
	PACK15_4(0xFBBC05, 0xC89604, 0x7D5E02, 0x4B3801)
};

static const unsigned short twb64_060_google_green[] = {
	PACK15_4(0x34A853, 0x298642, 0x1A5429, 0x0F3218),
	PACK15_4(0x34A853, 0x298642, 0x1A5429, 0x0F3218),
	PACK15_4(0x34A853, 0x298642, 0x1A5429, 0x0F3218)
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
	PACK15_4(0x97D700, 0x78AC00, 0x4B6B00, 0x2D4000),
	PACK15_4(0x97D700, 0x78AC00, 0x4B6B00, 0x2D4000),
	PACK15_4(0x97D700, 0x78AC00, 0x4B6B00, 0x2D4000)
};

static const unsigned short twb64_064_coca_cola_vision[] = {
	PACK15_4(0xFFFFFF, 0xD7D7D7, 0xF40009, 0x000000),
	PACK15_4(0xFFFFFF, 0xD7D7D7, 0xF40009, 0x000000),
	PACK15_4(0xFFFFFF, 0xD7D7D7, 0xF40009, 0x000000)
};

static const unsigned short twb64_065_gameking_ver[] = {
	PACK15_4(0x8CCE94, 0x6B9C63, 0x405D3B, 0x184421),
	PACK15_4(0x8CCE94, 0x6B9C63, 0x405D3B, 0x184421),
	PACK15_4(0x8CCE94, 0x6B9C63, 0x405D3B, 0x184421)
};

static const unsigned short twb64_066_do_the_dew_ver[] = {
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C),
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C),
	PACK15_4(0xFFFFFF, 0xA1D23F, 0xD82A34, 0x29673C)
};

static const unsigned short twb64_067_digivice_ver[] = {
	PACK15_4(0x8C8C73, 0x70705C, 0x464639, 0x2A2A22),
	PACK15_4(0x8C8C73, 0x70705C, 0x464639, 0x2A2A22),
	PACK15_4(0x8C8C73, 0x70705C, 0x464639, 0x2A2A22)
};

static const unsigned short twb64_068_bikini_bottom_ver[] = {
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000),
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000),
	PACK15_4(0xF8F880, 0x48F8E0, 0x2098F0, 0x606000)
};

static const unsigned short twb64_069_blossom_pink[] = {
	PACK15_4(0xF59BB2, 0xC47C8E, 0x7A4D59, 0x492E35),
	PACK15_4(0xF59BB2, 0xC47C8E, 0x7A4D59, 0x492E35),
	PACK15_4(0xF59BB2, 0xC47C8E, 0x7A4D59, 0x492E35)
};

static const unsigned short twb64_070_bubbles_blue[] = {
	PACK15_4(0x64C4E9, 0x509CBA, 0x326274, 0x1E3A45),
	PACK15_4(0x64C4E9, 0x509CBA, 0x326274, 0x1E3A45),
	PACK15_4(0x64C4E9, 0x509CBA, 0x326274, 0x1E3A45)
};

static const unsigned short twb64_071_buttercup_green[] = {
	PACK15_4(0xBEDC8D, 0x98B070, 0x5F6E46, 0x39422A),
	PACK15_4(0xBEDC8D, 0x98B070, 0x5F6E46, 0x39422A),
	PACK15_4(0xBEDC8D, 0x98B070, 0x5F6E46, 0x39422A)
};

static const unsigned short twb64_072_nascar_ver[] = {
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000),
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000),
	PACK15_4(0xFFD659, 0xE4002B, 0x007AC2, 0x000000)
};

static const unsigned short twb64_073_lemon_lime_green[] = {
	PACK15_4(0xF1C545, 0x51A631, 0x30631D, 0x18310E),
	PACK15_4(0xF1C545, 0x51A631, 0x30631D, 0x18310E),
	PACK15_4(0xF1C545, 0x51A631, 0x30631D, 0x18310E)
};

static const unsigned short twb64_074_mega_man_v_ver[] = {
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030),
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030),
	PACK15_4(0xD0D0D0, 0x70A0E0, 0x406890, 0x082030)
};

static const unsigned short twb64_075_tamagotchi_ver[] = {
	PACK15_4(0xF1F0F9, 0xC0C0C7, 0x78787C, 0x3C3838),
	PACK15_4(0xF1F0F9, 0xC0C0C7, 0x78787C, 0x3C3838),
	PACK15_4(0xF1F0F9, 0xC0C0C7, 0x78787C, 0x3C3838)
};

static const unsigned short twb64_076_phantom_red[] = {
	PACK15_4(0xFD2639, 0xCA1E2D, 0x7E131C, 0x4B0B11),
	PACK15_4(0xFD2639, 0xCA1E2D, 0x7E131C, 0x4B0B11),
	PACK15_4(0xFD2639, 0xCA1E2D, 0x7E131C, 0x4B0B11)
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
	PACK15_4(0xB4AA82, 0x908868, 0x5A5541, 0x363327),
	PACK15_4(0xB4AA82, 0x908868, 0x5A5541, 0x363327),
	PACK15_4(0xB4AA82, 0x908868, 0x5A5541, 0x363327)
};

static const unsigned short twb64_081_camoflauge_ver[] = {
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538),
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538),
	PACK15_4(0xBCAB90, 0xAC7E54, 0x79533D, 0x373538)
};

static const unsigned short twb64_082_legendary_super_saiyan[] = {
	PACK15_4(0xA6DA5B, 0x84AE48, 0x536D2D, 0x31411B),
	PACK15_4(0xA6DA5B, 0x84AE48, 0x536D2D, 0x31411B),
	PACK15_4(0xA6DA5B, 0x84AE48, 0x536D2D, 0x31411B)
};

static const unsigned short twb64_083_super_saiyan_rose[] = {
	PACK15_4(0xF7AFB3, 0xC58C8F, 0x7B5759, 0x4A3435),
	PACK15_4(0xF7AFB3, 0xC58C8F, 0x7B5759, 0x4A3435),
	PACK15_4(0xF7AFB3, 0xC58C8F, 0x7B5759, 0x4A3435)
};

static const unsigned short twb64_084_super_saiyan[] = {
	PACK15_4(0xFEFCC1, 0xCBC99A, 0x7F7E60, 0x4C4B39),
	PACK15_4(0xFEFCC1, 0xCBC99A, 0x7F7E60, 0x4C4B39),
	PACK15_4(0xFEFCC1, 0xCBC99A, 0x7F7E60, 0x4C4B39)
};

static const unsigned short twb64_085_perfected_ultra_instinct[] = {
	PACK15_4(0xC0C8D8, 0x99A0AC, 0x60646C, 0x393C40),
	PACK15_4(0xC0C8D8, 0x99A0AC, 0x60646C, 0x393C40),
	PACK15_4(0xC0C8D8, 0x99A0AC, 0x60646C, 0x393C40)
};

static const unsigned short twb64_086_saint_snow_red[] = {
	PACK15_4(0xBF3936, 0x982D2B, 0x5F1C1B, 0x391110),
	PACK15_4(0xBF3936, 0x982D2B, 0x5F1C1B, 0x391110),
	PACK15_4(0xBF3936, 0x982D2B, 0x5F1C1B, 0x391110)
};

static const unsigned short twb64_087_yellow_banana[] = {
	PACK15_4(0xFFDF08, 0xDE9E00, 0xAD6939, 0x734900),
	PACK15_4(0xFFDF08, 0xDE9E00, 0xAD6939, 0x734900),
	PACK15_4(0xFFDF08, 0xDE9E00, 0xAD6939, 0x734900)
};

static const unsigned short twb64_088_green_banana[] = {
	PACK15_4(0x63DF08, 0x4A9E00, 0x396939, 0x214900),
	PACK15_4(0x63DF08, 0x4A9E00, 0x396939, 0x214900),
	PACK15_4(0x63DF08, 0x4A9E00, 0x396939, 0x214900)
};

static const unsigned short twb64_089_super_saiyan_3[] = {
	PACK15_4(0xF8C838, 0xC6A02C, 0x7C641C, 0x4A3C10),
	PACK15_4(0xF8C838, 0xC6A02C, 0x7C641C, 0x4A3C10),
	PACK15_4(0xF8C838, 0xC6A02C, 0x7C641C, 0x4A3C10)
};

static const unsigned short twb64_090_super_saiyan_blue_evolved[] = {
	PACK15_4(0x1B97D1, 0x1578A7, 0x0D4B68, 0x082D3E),
	PACK15_4(0x1B97D1, 0x1578A7, 0x0D4B68, 0x082D3E),
	PACK15_4(0x1B97D1, 0x1578A7, 0x0D4B68, 0x082D3E)
};

static const unsigned short twb64_091_pocket_tales_ver[] = {
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000),
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000),
	PACK15_4(0xD0D860, 0x88A000, 0x385000, 0x000000)
};

static const unsigned short twb64_092_investigation_yellow[] = {
	PACK15_4(0xFFF919, 0xCCC714, 0x7F7C0C, 0x4C4A07),
	PACK15_4(0xFFF919, 0xCCC714, 0x7F7C0C, 0x4C4A07),
	PACK15_4(0xFFF919, 0xCCC714, 0x7F7C0C, 0x4C4A07)
};

static const unsigned short twb64_093_sees_blue[] = {
	PACK15_4(0x19D1FF, 0x14A7CC, 0x0C687F, 0x073E4C),
	PACK15_4(0x19D1FF, 0x14A7CC, 0x0C687F, 0x073E4C),
	PACK15_4(0x19D1FF, 0x14A7CC, 0x0C687F, 0x073E4C)
};

static const unsigned short twb64_094_ultra_instinct_sign[] = {
	PACK15_4(0x5A686F, 0x485358, 0x2D3437, 0x1B1F21),
	PACK15_4(0x5A686F, 0x485358, 0x2D3437, 0x1B1F21),
	PACK15_4(0x5A686F, 0x485358, 0x2D3437, 0x1B1F21)
};

static const unsigned short twb64_095_hokage_orange[] = {
	PACK15_4(0xEA8352, 0xBB6841, 0x754129, 0x462718),
	PACK15_4(0xEA8352, 0xBB6841, 0x754129, 0x462718),
	PACK15_4(0xEA8352, 0xBB6841, 0x754129, 0x462718)
};

static const unsigned short twb64_096_straw_hat_red[] = {
	PACK15_4(0xF8523C, 0xC64130, 0x7C291E, 0x4A1812),
	PACK15_4(0xF8523C, 0xC64130, 0x7C291E, 0x4A1812),
	PACK15_4(0xF8523C, 0xC64130, 0x7C291E, 0x4A1812)
};

static const unsigned short twb64_097_sword_art_cyan[] = {
	PACK15_4(0x59C3E2, 0x479CB4, 0x2C6171, 0x1A3A43),
	PACK15_4(0x59C3E2, 0x479CB4, 0x2C6171, 0x1A3A43),
	PACK15_4(0x59C3E2, 0x479CB4, 0x2C6171, 0x1A3A43)
};

static const unsigned short twb64_098_deku_alpha_emerald[] = {
	PACK15_4(0x39AD9E, 0x2D8A7E, 0x1C564F, 0x11332F),
	PACK15_4(0x39AD9E, 0x2D8A7E, 0x1C564F, 0x11332F),
	PACK15_4(0x39AD9E, 0x2D8A7E, 0x1C564F, 0x11332F)
};

static const unsigned short twb64_099_blue_stripes_ver[] = {
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5B5D5D, 0x2D2E2E),
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5B5D5D, 0x2D2E2E),
	PACK15_4(0x8BD3E1, 0x999B9C, 0x5B5D5D, 0x2D2E2E)
};

static const unsigned short twb64_100_precure_marble_raspberry[] = {
	PACK15_4(0xD6225C, 0xAB1B49, 0x6B112E, 0x400A1B),
	PACK15_4(0xD6225C, 0xAB1B49, 0x6B112E, 0x400A1B),
	PACK15_4(0xD6225C, 0xAB1B49, 0x6B112E, 0x400A1B)
};

static const unsigned short twb64_101_765pro_pink[] = {
	PACK15_4(0xF34F6D, 0xC23F57, 0x792736, 0x481720),
	PACK15_4(0xF34F6D, 0xC23F57, 0x792736, 0x481720),
	PACK15_4(0xF34F6D, 0xC23F57, 0x792736, 0x481720)
};

static const unsigned short twb64_102_cinderella_blue[] = {
	PACK15_4(0x2681C8, 0x1E67A0, 0x134064, 0x0B263C),
	PACK15_4(0x2681C8, 0x1E67A0, 0x134064, 0x0B263C),
	PACK15_4(0x2681C8, 0x1E67A0, 0x134064, 0x0B263C)
};

static const unsigned short twb64_103_million_yellow[] = {
	PACK15_4(0xFFC30B, 0xCC9C08, 0x7F6105, 0x4C3A03),
	PACK15_4(0xFFC30B, 0xCC9C08, 0x7F6105, 0x4C3A03),
	PACK15_4(0xFFC30B, 0xCC9C08, 0x7F6105, 0x4C3A03)
};

static const unsigned short twb64_104_sidem_green[] = {
	PACK15_4(0x0FBE94, 0x0C9876, 0x075F4A, 0x04392C),
	PACK15_4(0x0FBE94, 0x0C9876, 0x075F4A, 0x04392C),
	PACK15_4(0x0FBE94, 0x0C9876, 0x075F4A, 0x04392C)
};

static const unsigned short twb64_105_shiny_sky_blue[] = {
	PACK15_4(0x8DBBFF, 0x7095CC, 0x465D7F, 0x2A384C),
	PACK15_4(0x8DBBFF, 0x7095CC, 0x465D7F, 0x2A384C),
	PACK15_4(0x8DBBFF, 0x7095CC, 0x465D7F, 0x2A384C)
};

static const unsigned short twb64_106_angry_volcano_ver[] = {
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000),
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000),
	PACK15_4(0xF8B800, 0xF83800, 0xA81000, 0x1C0000)
};

static const unsigned short twb64_107_nba_vision[] = {
	PACK15_4(0xFFFFFF, 0xC8102E, 0x253B73, 0x000000),
	PACK15_4(0xFFFFFF, 0xC8102E, 0x253B73, 0x000000),
	PACK15_4(0xFFFFFF, 0xC8102E, 0x253B73, 0x000000)
};

static const unsigned short twb64_108_nfl_vision[] = {
	PACK15_4(0xFFFFFF, 0xD50A0A, 0x013369, 0x000000),
	PACK15_4(0xFFFFFF, 0xD50A0A, 0x013369, 0x000000),
	PACK15_4(0xFFFFFF, 0xD50A0A, 0x013369, 0x000000)
};

static const unsigned short twb64_109_mlb_vision[] = {
	PACK15_4(0xFFFFFF, 0x057AFF, 0xBF0D3E, 0x041E42),
	PACK15_4(0xFFFFFF, 0x057AFF, 0xBF0D3E, 0x041E42),
	PACK15_4(0xFFFFFF, 0x057AFF, 0xBF0D3E, 0x041E42)
};

static const unsigned short twb64_110_anime_digivice_ver[] = {
	PACK15_4(0x5B7B63, 0x48624F, 0x2D3D31, 0x1B241D),
	PACK15_4(0x5B7B63, 0x48624F, 0x2D3D31, 0x1B241D),
	PACK15_4(0x5B7B63, 0x48624F, 0x2D3D31, 0x1B241D)
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
	PACK15_4(0xF676A6, 0xC45E84, 0x7B3B53, 0x492331),
	PACK15_4(0xF676A6, 0xC45E84, 0x7B3B53, 0x492331),
	PACK15_4(0xF676A6, 0xC45E84, 0x7B3B53, 0x492331)
};

static const unsigned short twb64_120_miku_blue[] = {
	PACK15_4(0x11ADD5, 0x0D8AAA, 0x08566A, 0x05333F),
	PACK15_4(0x11ADD5, 0x0D8AAA, 0x08566A, 0x05333F),
	PACK15_4(0x11ADD5, 0x0D8AAA, 0x08566A, 0x05333F)
};

static const unsigned short twb64_121_tri_digivice_ver[] = {
	PACK15_4(0x848F79, 0x697260, 0x42473C, 0x272A24),
	PACK15_4(0x848F79, 0x697260, 0x42473C, 0x272A24),
	PACK15_4(0x848F79, 0x697260, 0x42473C, 0x272A24)
};

static const unsigned short twb64_122_survey_corps_uniform[] = {
	PACK15_4(0xACABA9, 0xAC7C59, 0x593D34, 0x321D1A),
	PACK15_4(0xACABA9, 0xAC7C59, 0x593D34, 0x321D1A),
	PACK15_4(0xACABA9, 0xAC7C59, 0x593D34, 0x321D1A)
};

static const unsigned short twb64_123_island_green[] = {
	PACK15_4(0x009B7E, 0x007C64, 0x004D3F, 0x002E25),
	PACK15_4(0x009B7E, 0x007C64, 0x004D3F, 0x002E25),
	PACK15_4(0x009B7E, 0x007C64, 0x004D3F, 0x002E25)
};

static const unsigned short twb64_124_nogizaka46_purple[] = {
	PACK15_4(0x812990, 0x672073, 0x401448, 0x260C2B),
	PACK15_4(0x812990, 0x672073, 0x401448, 0x260C2B),
	PACK15_4(0x812990, 0x672073, 0x401448, 0x260C2B)
};

static const unsigned short twb64_125_ninja_turtle_green[] = {
	PACK15_4(0x86BC25, 0x6B961D, 0x435E12, 0x28380B),
	PACK15_4(0x86BC25, 0x6B961D, 0x435E12, 0x28380B),
	PACK15_4(0x86BC25, 0x6B961D, 0x435E12, 0x28380B)
};

static const unsigned short twb64_126_slime_blue[] = {
	PACK15_4(0x2F8CCC, 0x2570A3, 0x174666, 0x0E2A3D),
	PACK15_4(0x2F8CCC, 0x2570A3, 0x174666, 0x0E2A3D),
	PACK15_4(0x2F8CCC, 0x2570A3, 0x174666, 0x0E2A3D)
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
	PACK15_4(0x4655F5, 0x3844C4, 0x232A7A, 0x151949),
	PACK15_4(0x4655F5, 0x3844C4, 0x232A7A, 0x151949),
	PACK15_4(0x4655F5, 0x3540BB, 0x202773, 0x0C0E2B)
};

static const unsigned short twb64_131_neon_purple[] = {
	PACK15_4(0xB400E6, 0x9000B8, 0x5A0073, 0x360045),
	PACK15_4(0xB400E6, 0x9000B8, 0x5A0073, 0x360045),
	PACK15_4(0xB400E6, 0x9000B8, 0x5A0073, 0x360045)
};

static const unsigned short twb64_132_neon_orange[] = {
	PACK15_4(0xFAA005, 0xC88004, 0x7D5002, 0x4B3001),
	PACK15_4(0xFAA005, 0xC88004, 0x7D5002, 0x4B3001),
	PACK15_4(0xFAA005, 0xC88004, 0x7D5002, 0x4B3001)
};

static const unsigned short twb64_133_moonlight_vision[] = {
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010),
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010),
	PACK15_4(0xF8D868, 0x3890E8, 0x305078, 0x101010)
};

static const unsigned short twb64_134_rising_sun_red[] = {
	PACK15_4(0xBC002D, 0x960024, 0x5D0016, 0x38000D),
	PACK15_4(0xBC002D, 0x960024, 0x5D0016, 0x38000D),
	PACK15_4(0xBC002D, 0x960024, 0x5D0016, 0x38000D)
};

static const unsigned short twb64_135_burger_king_color_combo[] = {
	PACK15_4(0xF5EBDC, 0xFF8732, 0xD62300, 0x502314),
	PACK15_4(0xF5EBDC, 0xFF8732, 0xD62300, 0x502314),
	PACK15_4(0xF5EBDC, 0xFF8732, 0xD62300, 0x502314)
};

static const unsigned short twb64_136_grand_zeno_coat[] = {
	PACK15_4(0xFBFBFA, 0xFCE72D, 0xCE26A9, 0x3D0B32),
	PACK15_4(0xFBFBFA, 0xFCE72D, 0xCE26A9, 0x3D0B32),
	PACK15_4(0xFBFBFA, 0xFCE72D, 0xCE26A9, 0x3D0B32)
};

static const unsigned short twb64_137_pac_man_yellow[] = {
	PACK15_4(0xFFE300, 0xCCB500, 0x7F7100, 0x4C4400),
	PACK15_4(0xFFE300, 0xCCB500, 0x7F7100, 0x4C4400),
	PACK15_4(0xFFE300, 0xCCB500, 0x7F7100, 0x4C4400)
};

static const unsigned short twb64_138_irish_green[] = {
	PACK15_4(0x45BE76, 0x37985E, 0x225F3B, 0x143923),
	PACK15_4(0x45BE76, 0x37985E, 0x225F3B, 0x143923),
	PACK15_4(0x45BE76, 0x37985E, 0x225F3B, 0x143923)
};

static const unsigned short twb64_139_goku_gi[] = {
	PACK15_4(0xF9F0E1, 0xE7612C, 0x173F72, 0x061222),
	PACK15_4(0xF9F0E1, 0xE7612C, 0x173F72, 0x061222),
	PACK15_4(0xF9F0E1, 0xE7612C, 0x173F72, 0x061222)
};

static const unsigned short twb64_140_dragon_ball_orange[] = {
	PACK15_4(0xF0831D, 0xC06817, 0x78410E, 0x482708),
	PACK15_4(0xF0831D, 0xC06817, 0x78410E, 0x482708),
	PACK15_4(0xF0831D, 0xC06817, 0x78410E, 0x482708)
};

static const unsigned short twb64_141_christmas_gold[] = {
	PACK15_4(0xC0A94B, 0x99873C, 0x605425, 0x393216),
	PACK15_4(0xC0A94B, 0x99873C, 0x605425, 0x393216),
	PACK15_4(0xC0A94B, 0x99873C, 0x605425, 0x393216)
};

static const unsigned short twb64_142_pepsi_vision[] = {
	PACK15_4(0xFFFFFF, 0xFF1400, 0x1414C8, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF1400, 0x1414C8, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF1400, 0x1414C8, 0x000000)
};

static const unsigned short twb64_143_bubblun_green[] = {
	PACK15_4(0x6ADC31, 0x54B027, 0x356E18, 0x1F420E),
	PACK15_4(0x6ADC31, 0x54B027, 0x356E18, 0x1F420E),
	PACK15_4(0x6ADC31, 0x54B027, 0x356E18, 0x1F420E)
};

static const unsigned short twb64_144_bobblun_blue[] = {
	PACK15_4(0x1FD1FD, 0x18A7CA, 0x0F687E, 0x093E4B),
	PACK15_4(0x1FD1FD, 0x18A7CA, 0x0F687E, 0x093E4B),
	PACK15_4(0x1FD1FD, 0x18A7CA, 0x0F687E, 0x093E4B)
};

static const unsigned short twb64_145_baja_blast_storm[] = {
	PACK15_4(0x68C2A4, 0x539B83, 0x346152, 0x1F3A31),
	PACK15_4(0x68C2A4, 0x539B83, 0x346152, 0x1F3A31),
	PACK15_4(0x68C2A4, 0x539B83, 0x346152, 0x1F3A31)
};

static const unsigned short twb64_146_olympic_gold[] = {
	PACK15_4(0xD5B624, 0xAA911C, 0x6A5B12, 0x3F360A),
	PACK15_4(0xD5B624, 0xAA911C, 0x6A5B12, 0x3F360A),
	PACK15_4(0xD5B624, 0xAA911C, 0x6A5B12, 0x3F360A)
};

static const unsigned short twb64_147_lisani_orange[] = {
	PACK15_4(0xEB5E01, 0xBC4B00, 0x752F00, 0x461C00),
	PACK15_4(0xEB5E01, 0xBC4B00, 0x752F00, 0x461C00),
	PACK15_4(0xEB5E01, 0xBC4B00, 0x752F00, 0x461C00)
};

static const unsigned short twb64_148_liella_purple[] = {
	PACK15_4(0xA5469B, 0x84387C, 0x52234D, 0x31152E),
	PACK15_4(0xA5469B, 0x84387C, 0x52234D, 0x31152E),
	PACK15_4(0xA5469B, 0x84387C, 0x52234D, 0x31152E)
};

static const unsigned short twb64_149_olympic_silver[] = {
	PACK15_4(0x9EA59C, 0x7E847C, 0x4F524E, 0x2F312E),
	PACK15_4(0x9EA59C, 0x7E847C, 0x4F524E, 0x2F312E),
	PACK15_4(0x9EA59C, 0x7E847C, 0x4F524E, 0x2F312E)
};

static const unsigned short twb64_150_olympic_bronze[] = {
	PACK15_4(0xCD8152, 0xA46741, 0x664029, 0x3D2618),
	PACK15_4(0xCD8152, 0xA46741, 0x664029, 0x3D2618),
	PACK15_4(0xCD8152, 0xA46741, 0x664029, 0x3D2618)
};

static const unsigned short twb64_151_ana_flight_blue[] = {
	PACK15_4(0x00B3F0, 0x3B8BC0, 0x223F9A, 0x00146E),
	PACK15_4(0x00B3F0, 0x3B8BC0, 0x223F9A, 0x00146E),
	PACK15_4(0x00B3F0, 0x3B8BC0, 0x223F9A, 0x00146E)
};

static const unsigned short twb64_152_nijigasaki_orange[] = {
	PACK15_4(0xF39800, 0xC27900, 0x794C00, 0x482D00),
	PACK15_4(0xF39800, 0xC27900, 0x794C00, 0x482D00),
	PACK15_4(0xF39800, 0xC27900, 0x794C00, 0x482D00)
};

static const unsigned short twb64_153_holoblue[] = {
	PACK15_4(0xB0EDFA, 0x49C4F2, 0x3368D3, 0x063F5C),
	PACK15_4(0xB0EDFA, 0x49C4F2, 0x3368D3, 0x063F5C),
	PACK15_4(0xB0EDFA, 0x49C4F2, 0x3368D3, 0x063F5C)
};

static const unsigned short twb64_154_wwe_white_and_red[] = {
	PACK15_4(0xFFFFFF, 0xD7182A, 0x810E19, 0x40070C),
	PACK15_4(0xFFFFFF, 0xD7182A, 0x810E19, 0x40070C),
	PACK15_4(0xFFFFFF, 0xD7182A, 0x810E19, 0x40070C)
};

static const unsigned short twb64_155_yoshi_egg_green[] = {
	PACK15_4(0x66C430, 0x519C26, 0x336218, 0x1E3A0E),
	PACK15_4(0x66C430, 0x519C26, 0x336218, 0x1E3A0E),
	PACK15_4(0x66C430, 0x519C26, 0x336218, 0x1E3A0E)
};

static const unsigned short twb64_156_pokedex_red[] = {
	PACK15_4(0xEA5450, 0xBB4340, 0x752A28, 0x461918),
	PACK15_4(0xEA5450, 0xBB4340, 0x752A28, 0x461918),
	PACK15_4(0xEA5450, 0xBB4340, 0x752A28, 0x461918)
};

static const unsigned short twb64_157_familymart_vision[] = {
	PACK15_4(0x008CD6, 0x00A040, 0x006026, 0x003013),
	PACK15_4(0x008CD6, 0x00A040, 0x006026, 0x003013),
	PACK15_4(0x008CD6, 0x00A040, 0x006026, 0x003013)
};

static const unsigned short twb64_158_xbox_green[] = {
	PACK15_4(0x92C83E, 0x74A031, 0x49641F, 0x2B3C12),
	PACK15_4(0x92C83E, 0x74A031, 0x49641F, 0x2B3C12),
	PACK15_4(0x92C83E, 0x74A031, 0x49641F, 0x2B3C12)
};

static const unsigned short twb64_159_sonic_mega_blue[] = {
	PACK15_4(0x4084D9, 0x3369AD, 0x20426C, 0x132741),
	PACK15_4(0x4084D9, 0x3369AD, 0x20426C, 0x132741),
	PACK15_4(0x4084D9, 0x3369AD, 0x20426C, 0x132741)
};

static const unsigned short twb64_160_sprite_green[] = {
	PACK15_4(0x009B4E, 0x007C3E, 0x004D27, 0x002E17),
	PACK15_4(0x009B4E, 0x007C3E, 0x004D27, 0x002E17),
	PACK15_4(0x009B4E, 0x007C3E, 0x004D27, 0x002E17)
};

static const unsigned short twb64_161_scarlett_green[] = {
	PACK15_4(0x9BF00B, 0x7CC008, 0x4D7805, 0x2E4803),
	PACK15_4(0x9BF00B, 0x7CC008, 0x4D7805, 0x2E4803),
	PACK15_4(0x9BF00B, 0x7CC008, 0x4D7805, 0x2E4803)
};

static const unsigned short twb64_162_glitchy_blue[] = {
	PACK15_4(0x337EFB, 0x2864C8, 0x193F7D, 0x0F254B),
	PACK15_4(0x337EFB, 0x2864C8, 0x193F7D, 0x0F254B),
	PACK15_4(0x337EFB, 0x2864C8, 0x193F7D, 0x0F254B)
};

static const unsigned short twb64_163_classic_lcd[] = {
	PACK15_4(0xC6CBAD, 0x9EA28A, 0x636556, 0x3B3C33),
	PACK15_4(0xC6CBAD, 0x9EA28A, 0x636556, 0x3B3C33),
	PACK15_4(0xC6CBAD, 0x9EA28A, 0x636556, 0x3B3C33)
};

static const unsigned short twb64_164_3ds_virtual_console_ver[] = {
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918),
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918),
	PACK15_4(0xCECEAD, 0xA5A58C, 0x6B6B52, 0x292918)
};

static const unsigned short twb64_165_pocketstation_ver[] = {
	PACK15_4(0x969687, 0x78786C, 0x4B4B43, 0x2D2D28),
	PACK15_4(0x969687, 0x78786C, 0x4B4B43, 0x2D2D28),
	PACK15_4(0x969687, 0x78786C, 0x4B4B43, 0x2D2D28)
};

static const unsigned short twb64_166_timeless_gold_and_red[] = {
	PACK15_4(0xC8AA50, 0xB91E23, 0x6F1215, 0x37090A),
	PACK15_4(0xC8AA50, 0xB91E23, 0x6F1215, 0x37090A),
	PACK15_4(0xC8AA50, 0xB91E23, 0x6F1215, 0x37090A)
};

static const unsigned short twb64_167_smurfy_blue[] = {
	PACK15_4(0x2CB9EF, 0x2394BF, 0x165C77, 0x0D3747),
	PACK15_4(0x2CB9EF, 0x2394BF, 0x165C77, 0x0D3747),
	PACK15_4(0x2CB9EF, 0x2394BF, 0x165C77, 0x0D3747)
};

static const unsigned short twb64_168_swampy_ogre_green[] = {
	PACK15_4(0xC1D62E, 0x9AAB24, 0x606B17, 0x39400D),
	PACK15_4(0xC1D62E, 0x9AAB24, 0x606B17, 0x39400D),
	PACK15_4(0xC1D62E, 0x9AAB24, 0x606B17, 0x39400D)
};

static const unsigned short twb64_169_sailor_spinach_green[] = {
	PACK15_4(0x7BB03C, 0x628C30, 0x3D581E, 0x243412),
	PACK15_4(0x7BB03C, 0x628C30, 0x3D581E, 0x243412),
	PACK15_4(0x7BB03C, 0x628C30, 0x3D581E, 0x243412)
};

static const unsigned short twb64_170_shenron_green[] = {
	PACK15_4(0x5AC34A, 0x489C3B, 0x2D6125, 0x1B3A16),
	PACK15_4(0x5AC34A, 0x489C3B, 0x2D6125, 0x1B3A16),
	PACK15_4(0x5AC34A, 0x489C3B, 0x2D6125, 0x1B3A16)
};

static const unsigned short twb64_171_berserk_blood[] = {
	PACK15_4(0xBB1414, 0x951010, 0x5D0A0A, 0x380606),
	PACK15_4(0xBB1414, 0x951010, 0x5D0A0A, 0x380606),
	PACK15_4(0xBB1414, 0x951010, 0x5D0A0A, 0x380606)
};

static const unsigned short twb64_172_super_star_pink[] = {
	PACK15_4(0xF3A5AA, 0xC28488, 0x795255, 0x483133),
	PACK15_4(0xF3A5AA, 0xC28488, 0x795255, 0x483133),
	PACK15_4(0xF3A5AA, 0xC28488, 0x795255, 0x483133)
};

static const unsigned short twb64_173_gamebuino_classic_ver[] = {
	PACK15_4(0x81A17E, 0x678064, 0x40503F, 0x263025),
	PACK15_4(0x81A17E, 0x678064, 0x40503F, 0x263025),
	PACK15_4(0x81A17E, 0x678064, 0x40503F, 0x263025)
};

static const unsigned short twb64_174_barbie_pink[] = {
	PACK15_4(0xF200A1, 0xC10080, 0x790050, 0x480030),
	PACK15_4(0xF200A1, 0xC10080, 0x790050, 0x480030),
	PACK15_4(0xF200A1, 0xC10080, 0x790050, 0x480030)
};

static const unsigned short twb64_175_yoasobi_amaranth[] = {
	PACK15_4(0xF2285A, 0xC12048, 0x79142D, 0x480C1B),
	PACK15_4(0xF2285A, 0xC12048, 0x79142D, 0x480C1B),
	PACK15_4(0xF2285A, 0xC12048, 0x79142D, 0x480C1B)
};

static const unsigned short twb64_176_nokia_3310_ver[] = {
	PACK15_4(0x73A684, 0x5C8469, 0x395342, 0x223127),
	PACK15_4(0x73A684, 0x5C8469, 0x395342, 0x223127),
	PACK15_4(0x73A684, 0x5C8469, 0x395342, 0x223127)
};

static const unsigned short twb64_177_clover_green[] = {
	PACK15_4(0x39B54A, 0x2D903B, 0x1C5A25, 0x113616),
	PACK15_4(0x39B54A, 0x2D903B, 0x1C5A25, 0x113616),
	PACK15_4(0x39B54A, 0x2D903B, 0x1C5A25, 0x113616)
};

static const unsigned short twb64_178_goku_gt_gi[] = {
	PACK15_4(0xDBE3E6, 0xF0AC18, 0x3D6EA5, 0x122131),
	PACK15_4(0xDBE3E6, 0xF0AC18, 0x3D6EA5, 0x122131),
	PACK15_4(0xDBE3E6, 0xF0AC18, 0x3D6EA5, 0x122131)
};

static const unsigned short twb64_179_famicom_disk_yellow[] = {
	PACK15_4(0xF3C200, 0xC29B00, 0x796100, 0x483A00),
	PACK15_4(0xF3C200, 0xC29B00, 0x796100, 0x483A00),
	PACK15_4(0xF3C200, 0xC29B00, 0x796100, 0x483A00)
};

static const unsigned short twb64_180_team_rocket_uniform[] = {
	PACK15_4(0xEEEFEB, 0xE94E60, 0x755E88, 0x474F4D),
	PACK15_4(0xEEEFEB, 0xE94E60, 0x755E88, 0x474F4D),
	PACK15_4(0xEEEFEB, 0xE94E60, 0x755E88, 0x474F4D)
};

static const unsigned short twb64_181_seiko_timely_vision[] = {
	PACK15_4(0xFFBF00, 0x4393E6, 0x0050A5, 0x202121),
	PACK15_4(0xFFBF00, 0x4393E6, 0x0050A5, 0x202121),
	PACK15_4(0xFFBF00, 0x4393E6, 0x0050A5, 0x202121)
};

static const unsigned short twb64_182_pastel109[] = {
	PACK15_4(0xF2D53F, 0xFD87B2, 0x3DACB8, 0x5503A6),
	PACK15_4(0xF2D53F, 0xFD87B2, 0x3DACB8, 0x5503A6),
	PACK15_4(0xF2D53F, 0xFD87B2, 0x3DACB8, 0x5503A6)
};

static const unsigned short twb64_183_doraemon_tricolor[] = {
	PACK15_4(0xFFE800, 0x00A8F4, 0xE60000, 0x450000),
	PACK15_4(0xFFE800, 0x00A8F4, 0xE60000, 0x450000),
	PACK15_4(0xFFE800, 0x00A8F4, 0xE60000, 0x450000)
};

static const unsigned short twb64_184_fury_blue[] = {
	PACK15_4(0x2B5F98, 0x224C79, 0x152F4C, 0x0C1C2D),
	PACK15_4(0x2B5F98, 0x224C79, 0x152F4C, 0x0C1C2D),
	PACK15_4(0x2B5F98, 0x224C79, 0x152F4C, 0x0C1C2D)
};

static const unsigned short twb64_185_good_smile_vision[] = {
	PACK15_4(0x9FA0A0, 0xEE7700, 0x8E4700, 0x472300),
	PACK15_4(0x9FA0A0, 0xEE7700, 0x8E4700, 0x472300),
	PACK15_4(0x9FA0A0, 0xEE7700, 0x8E4700, 0x472300)
};

static const unsigned short twb64_186_puyo_puyo_green[] = {
	PACK15_4(0x48E236, 0x39B42B, 0x24771B, 0x154310),
	PACK15_4(0x48E236, 0x39B42B, 0x24771B, 0x154310),
	PACK15_4(0x48E236, 0x39B42B, 0x24771B, 0x154310)
};

static const unsigned short twb64_187_circle_k_color_combo[] = {
	PACK15_4(0xF99B2A, 0xEC2E24, 0x8D1B15, 0x460D0A),
	PACK15_4(0xF99B2A, 0xEC2E24, 0x8D1B15, 0x460D0A),
	PACK15_4(0xF99B2A, 0xEC2E24, 0x8D1B15, 0x460D0A)
};

static const unsigned short twb64_188_pizza_hut_red[] = {
	PACK15_4(0xE3383E, 0xB52C31, 0x711C1F, 0x441012),
	PACK15_4(0xE3383E, 0xB52C31, 0x711C1F, 0x441012),
	PACK15_4(0xE3383E, 0xB52C31, 0x711C1F, 0x441012)
};

static const unsigned short twb64_189_emerald_green[] = {
	PACK15_4(0x50C878, 0x40A060, 0x28643C, 0x183C24),
	PACK15_4(0x50C878, 0x40A060, 0x28643C, 0x183C24),
	PACK15_4(0x50C878, 0x40A060, 0x28643C, 0x183C24)
};

static const unsigned short twb64_190_grand_ivory[] = {
	PACK15_4(0xD9D6BE, 0xADAB98, 0x6C6B5F, 0x414039),
	PACK15_4(0xD9D6BE, 0xADAB98, 0x6C6B5F, 0x414039),
	PACK15_4(0xD9D6BE, 0xADAB98, 0x6C6B5F, 0x414039)
};

static const unsigned short twb64_191_demons_gold[] = {
	PACK15_4(0xBAAF56, 0x948C44, 0x5D572B, 0x373419),
	PACK15_4(0xBAAF56, 0x948C44, 0x5D572B, 0x373419),
	PACK15_4(0xBAAF56, 0x948C44, 0x5D572B, 0x373419)
};

static const unsigned short twb64_192_sega_tokyo_blue[] = {
	PACK15_4(0x0082D4, 0x0068A9, 0x00416A, 0x00273F),
	PACK15_4(0x0082D4, 0x0068A9, 0x00416A, 0x00273F),
	PACK15_4(0x0082D4, 0x0068A9, 0x00416A, 0x00273F)
};

static const unsigned short twb64_193_champions_tunic[] = {
	PACK15_4(0xE2DCB1, 0x009EDD, 0x875B40, 0x281B13),
	PACK15_4(0xE2DCB1, 0x009EDD, 0x875B40, 0x281B13),
	PACK15_4(0xE2DCB1, 0x009EDD, 0x875B40, 0x281B13)
};

static const unsigned short twb64_194_dk_barrel_brown[] = {
	PACK15_4(0xC3742F, 0x9C5C25, 0x613A17, 0x3A220E),
	PACK15_4(0xC3742F, 0x9C5C25, 0x613A17, 0x3A220E),
	PACK15_4(0xC3742F, 0x9C5C25, 0x613A17, 0x3A220E)
};

static const unsigned short twb64_195_eva_01[] = {
	PACK15_4(0x54CF54, 0xF99B22, 0x765898, 0x303345),
	PACK15_4(0x54CF54, 0xF99B22, 0x765898, 0x303345),
	PACK15_4(0x54CF54, 0xF99B22, 0x765898, 0x303345)
};

static const unsigned short twb64_196_wild_west_vision[] = {
	PACK15_4(0xD9D7C7, 0xC3976A, 0x924A36, 0x3A160E),
	PACK15_4(0xD9D7C7, 0xC3976A, 0x924A36, 0x3A160E),
	PACK15_4(0xD9D7C7, 0xC3976A, 0x924A36, 0x3A160E)
};

static const unsigned short twb64_197_optimus_prime_palette[] = {
	PACK15_4(0xD3D3D3, 0xD92121, 0x0047AB, 0x001533),
	PACK15_4(0xD3D3D3, 0xD92121, 0x0047AB, 0x001533),
	PACK15_4(0xD3D3D3, 0xD92121, 0x0047AB, 0x001533)
};

static const unsigned short twb64_198_niconico_sea_green[] = {
	PACK15_4(0x19C3A4, 0x149C83, 0x0C6152, 0x073A31),
	PACK15_4(0x19C3A4, 0x149C83, 0x0C6152, 0x073A31),
	PACK15_4(0x19C3A4, 0x149C83, 0x0C6152, 0x073A31)
};

static const unsigned short twb64_199_duracell_copper[] = {
	PACK15_4(0xC8895D, 0xA06D4A, 0x64442E, 0x3C291B),
	PACK15_4(0xC8895D, 0xA06D4A, 0x64442E, 0x3C291B),
	PACK15_4(0xC8895D, 0xA06D4A, 0x64442E, 0x3C291B)
};

static const unsigned short twb64_200_tokyo_skytree_cloudy_blue[] = {
	PACK15_4(0x82B5C7, 0x68909F, 0x415A63, 0x27363B),
	PACK15_4(0x82B5C7, 0x68909F, 0x415A63, 0x27363B),
	PACK15_4(0x82B5C7, 0x68909F, 0x415A63, 0x27363B)
};

static const unsigned short twb64_201_dmg_gold[] = {
PACK15_4(0xA1B560, 0x80904C, 0x505A30, 0x30361C),
PACK15_4(0xA1B560, 0x80904C, 0x505A30, 0x30361C),
PACK15_4(0xA1B560, 0x80904C, 0x505A30, 0x30361C)
};

static const unsigned short twb64_202_lcd_clock_green[] = {
PACK15_4(0x50B580, 0x409066, 0x285A40, 0x183626),
PACK15_4(0x50B580, 0x409066, 0x285A40, 0x183626),
PACK15_4(0x50B580, 0x409066, 0x285A40, 0x183626)
};

static const unsigned short twb64_203_famicom_frenzy[] = {
PACK15_4(0xEFECDA, 0xD9BE72, 0xA32135, 0x231916),
PACK15_4(0xEFECDA, 0xD9BE72, 0xA32135, 0x231916),
PACK15_4(0xEFECDA, 0xD9BE72, 0xA32135, 0x231916)
};

static const unsigned short twb64_204_dk_arcade_blue[] = {
PACK15_4(0x47A2DE, 0x3881B1, 0x23516F, 0x153042),
PACK15_4(0x47A2DE, 0x3881B1, 0x23516F, 0x153042),
PACK15_4(0x47A2DE, 0x3881B1, 0x23516F, 0x153042)
};

static const unsigned short twb64_205_advanced_indigo[] = {
PACK15_4(0x796ABA, 0x605494, 0x3C355D, 0x241F37),
PACK15_4(0x796ABA, 0x605494, 0x3C355D, 0x241F37),
PACK15_4(0x796ABA, 0x605494, 0x3C355D, 0x241F37)
};

static const unsigned short twb64_206_ultra_black[] = {
PACK15_4(0x4D5263, 0x3D414F, 0x262931, 0x17181D),
PACK15_4(0x4D5263, 0x3D414F, 0x262931, 0x17181D),
PACK15_4(0x4D5263, 0x3D414F, 0x262931, 0x17181D)
};

static const unsigned short twb64_207_chaos_emerald_green[] = {
PACK15_4(0xA0E000, 0x80C800, 0x409800, 0x208000),
PACK15_4(0xA0E000, 0x80C800, 0x409800, 0x208000),
PACK15_4(0xA0E000, 0x80C800, 0x409800, 0x208000)
};

static const unsigned short twb64_208_blue_bomber_vision[] = {
PACK15_4(0xE2CDA7, 0x639AFC, 0x0D4DC4, 0x000000),
PACK15_4(0xE2CDA7, 0x639AFC, 0x0D4DC4, 0x000000),
PACK15_4(0xE2CDA7, 0x639AFC, 0x0D4DC4, 0x000000)
};

static const unsigned short twb64_209_krispy_kreme_vision[] = {
PACK15_4(0xFFFFFF, 0xCF152D, 0x166938, 0x000000),
PACK15_4(0xFFFFFF, 0xCF152D, 0x166938, 0x000000),
PACK15_4(0xFFFFFF, 0xCF152D, 0x166938, 0x000000)
};

static const unsigned short twb64_210_steam_gray[] = {
PACK15_4(0xC5C3C0, 0x9D9C99, 0x626160, 0x3B3A39),
PACK15_4(0xC5C3C0, 0x9D9C99, 0x626160, 0x3B3A39),
PACK15_4(0xC5C3C0, 0x9D9C99, 0x626160, 0x3B3A39)
};

static const unsigned short twb64_211_dream_land_gb_ver[] = {
PACK15_4(0xF6FF70, 0xB9D03A, 0x788B1D, 0x48530E),
PACK15_4(0xF6FF70, 0xB9D03A, 0x788B1D, 0x48530E),
PACK15_4(0xF6FF70, 0xB9D03A, 0x788B1D, 0x48530E)
};

static const unsigned short twb64_212_pokemon_pinball_ver[] = {
PACK15_4(0xE8F8B8, 0xA0B050, 0x786030, 0x181820),
PACK15_4(0xE8F8B8, 0xA0B050, 0x786030, 0x181820),
PACK15_4(0xE8F8B8, 0xA0B050, 0x786030, 0x181820)
};

static const unsigned short twb64_213_poketch_ver[] = {
PACK15_4(0x70B070, 0x508050, 0x385030, 0x102818),
PACK15_4(0x70B070, 0x508050, 0x385030, 0x102818),
PACK15_4(0x70B070, 0x508050, 0x385030, 0x102818)
};

static const unsigned short twb64_214_collection_of_saga_ver[] = {
PACK15_4(0xB2C0A8, 0x769A67, 0x345D51, 0x041820),
PACK15_4(0xB2C0A8, 0x769A67, 0x345D51, 0x041820),
PACK15_4(0xB2C0A8, 0x769A67, 0x345D51, 0x041820)
};

static const unsigned short twb64_215_rocky_valley_holiday[] = {
PACK15_4(0xC0F0F8, 0xD89078, 0x805850, 0x204008),
PACK15_4(0xC0F0F8, 0xD89078, 0x805850, 0x204008),
PACK15_4(0xC0F0F8, 0xD89078, 0x805850, 0x204008)
};

static const unsigned short twb64_216_giga_kiwi_dmg[] = {
PACK15_4(0xD0E040, 0xA0A830, 0x607028, 0x384828),
PACK15_4(0xD0E040, 0xA0A830, 0x607028, 0x384828),
PACK15_4(0xD0E040, 0xA0A830, 0x607028, 0x384828)
};

static const unsigned short twb64_217_dmg_pea_green[] = {
PACK15_4(0xD7E894, 0xAEC440, 0x527F39, 0x204631),
PACK15_4(0xD7E894, 0xAEC440, 0x527F39, 0x204631),
PACK15_4(0xD7E894, 0xAEC440, 0x527F39, 0x204631)
};

static const unsigned short twb64_218_timing_hero_ver[] = {
PACK15_4(0xCCCC99, 0x8C994C, 0x4C6718, 0x202E00),
PACK15_4(0xCCCC99, 0x8C994C, 0x4C6718, 0x202E00),
PACK15_4(0xCCCC99, 0x8C994C, 0x4C6718, 0x202E00)
};

static const unsigned short twb64_219_invincible_yellow_and_blue[] = {
PACK15_4(0xFEE566, 0x39C9EB, 0x22788D, 0x113C46),
PACK15_4(0xFEE566, 0x39C9EB, 0x22788D, 0x113C46),
PACK15_4(0xFEE566, 0x39C9EB, 0x22788D, 0x113C46)
};

static const unsigned short twb64_220_grinchy_green[] = {
PACK15_4(0xB7BE1C, 0x929816, 0x5B5F0E, 0x363908),
PACK15_4(0xB7BE1C, 0x929816, 0x5B5F0E, 0x363908),
PACK15_4(0xB7BE1C, 0x929816, 0x5B5F0E, 0x363908)
};

static const unsigned short twb64_221_animate_vision[] = {
PACK15_4(0xFFFFFF, 0xF9BE00, 0x385EAA, 0x231815),
PACK15_4(0xFFFFFF, 0xF9BE00, 0x385EAA, 0x231815),
PACK15_4(0xFFFFFF, 0xF9BE00, 0x385EAA, 0x231815)
};

static const unsigned short twb64_222_school_idol_mix[] = {
PACK15_4(0xF39800, 0x00A0E9, 0xA5469B, 0x31152E),
PACK15_4(0xF39800, 0x00A0E9, 0xA5469B, 0x31152E),
PACK15_4(0xF39800, 0x00A0E9, 0xA5469B, 0x31152E)
};

static const unsigned short twb64_223_green_awakening[] = {
PACK15_4(0xF1FFDD, 0x98DB75, 0x367050, 0x000B16),
PACK15_4(0xF1FFDD, 0x98DB75, 0x367050, 0x000B16),
PACK15_4(0xF1FFDD, 0x98DB75, 0x367050, 0x000B16)
};

static const unsigned short twb64_224_goomba_brown[] = {
PACK15_4(0xAA593B, 0x88472F, 0x552C1D, 0x331A11),
PACK15_4(0xAA593B, 0x88472F, 0x552C1D, 0x331A11),
PACK15_4(0xAA593B, 0x88472F, 0x552C1D, 0x331A11)
};

static const unsigned short twb64_225_warioware_microblue[] = {
PACK15_4(0x1189CA, 0x0D6DA1, 0x084465, 0x05293C),
PACK15_4(0x1189CA, 0x0D6DA1, 0x084465, 0x05293C),
PACK15_4(0x1189CA, 0x0D6DA1, 0x084465, 0x05293C)
};

static const unsigned short twb64_226_konosuba_sherbet[] = {
PACK15_4(0xF08200, 0xE5006E, 0x890042, 0x440021),
PACK15_4(0xF08200, 0xE5006E, 0x890042, 0x440021),
PACK15_4(0xF08200, 0xE5006E, 0x890042, 0x440021)
};

static const unsigned short twb64_227_spooky_purple[] = {
PACK15_4(0x9E7CD2, 0x7E63A8, 0x4F3E69, 0x2F253F),
PACK15_4(0x9E7CD2, 0x7E63A8, 0x4F3E69, 0x2F253F),
PACK15_4(0x9E7CD2, 0x7E63A8, 0x4F3E69, 0x2F253F)
};

static const unsigned short twb64_228_treasure_gold[] = {
PACK15_4(0xCBB524, 0xA2901C, 0x655A12, 0x3C360A),
PACK15_4(0xCBB524, 0xA2901C, 0x655A12, 0x3C360A),
PACK15_4(0xCBB524, 0xA2901C, 0x655A12, 0x3C360A)
};

static const unsigned short twb64_229_cherry_blossom_pink[] = {
PACK15_4(0xF07EB0, 0xC0648C, 0x783F58, 0x482534),
PACK15_4(0xF07EB0, 0xC0648C, 0x783F58, 0x482534),
PACK15_4(0xF07EB0, 0xC0648C, 0x783F58, 0x482534)
};

static const unsigned short twb64_230_golden_trophy[] = {
PACK15_4(0xE8D018, 0xB9A613, 0x74680C, 0x453E07),
PACK15_4(0xE8D018, 0xB9A613, 0x74680C, 0x453E07),
PACK15_4(0xE8D018, 0xB9A613, 0x74680C, 0x453E07)
};

static const unsigned short twb64_231_glacial_winter_blue[] = {
PACK15_4(0x87C1E2, 0x6C9AB4, 0x436071, 0x283943),
PACK15_4(0x87C1E2, 0x6C9AB4, 0x436071, 0x283943),
PACK15_4(0x87C1E2, 0x6C9AB4, 0x436071, 0x283943)
};

static const unsigned short twb64_232_leprechaun_green[] = {
PACK15_4(0x378861, 0x2C6C4D, 0x1B4430, 0x10281D),
PACK15_4(0x378861, 0x2C6C4D, 0x1B4430, 0x10281D),
PACK15_4(0x378861, 0x2C6C4D, 0x1B4430, 0x10281D)
};

static const unsigned short twb64_233_saitama_super_blue[] = {
PACK15_4(0x277ABC, 0x1F6196, 0x133D5E, 0x0B2438),
PACK15_4(0x277ABC, 0x1F6196, 0x133D5E, 0x0B2438),
PACK15_4(0x277ABC, 0x1F6196, 0x133D5E, 0x0B2438)
};

static const unsigned short twb64_234_saitama_super_green[] = {
PACK15_4(0x16AE85, 0x118B6A, 0x0B5742, 0x063427),
PACK15_4(0x16AE85, 0x118B6A, 0x0B5742, 0x063427),
PACK15_4(0x16AE85, 0x118B6A, 0x0B5742, 0x063427)
};

static const unsigned short twb64_235_duolingo_green[] = {
PACK15_4(0x58CC02, 0x46A301, 0x2C6601, 0x1A3D00),
PACK15_4(0x58CC02, 0x46A301, 0x2C6601, 0x1A3D00),
PACK15_4(0x58CC02, 0x46A301, 0x2C6601, 0x1A3D00)
};

static const unsigned short twb64_236_super_mushroom_vision[] = {
PACK15_4(0xF7CEC3, 0xCC9E22, 0x923404, 0x000000),
PACK15_4(0xF7CEC3, 0xCC9E22, 0x923404, 0x000000),
PACK15_4(0xF7CEC3, 0xCC9E22, 0x923404, 0x000000)
};

static const unsigned short twb64_237_ancient_husuian_brown[] = {
PACK15_4(0xB39F90, 0x8F7F73, 0x594F48, 0x352F2B),
PACK15_4(0xB39F90, 0x8F7F73, 0x594F48, 0x352F2B),
PACK15_4(0xB39F90, 0x8F7F73, 0x594F48, 0x352F2B)
};

static const unsigned short twb64_238_sky_pop_ivory[] = {
PACK15_4(0xE5E0B8, 0xBEBB95, 0x86825A, 0x525025),
PACK15_4(0xE5E0B8, 0xBEBB95, 0x86825A, 0x525025),
PACK15_4(0xE5E0B8, 0xBEBB95, 0x86825A, 0x525025)
};

static const unsigned short twb64_239_lawson_blue[] = {
PACK15_4(0x0068B7, 0x005392, 0x00345B, 0x001F36),
PACK15_4(0x0068B7, 0x005392, 0x00345B, 0x001F36),
PACK15_4(0x0068B7, 0x005392, 0x00345B, 0x001F36)
};

static const unsigned short twb64_240_anime_expo_red[] = {
PACK15_4(0xEE3B33, 0xBE2F28, 0x771D19, 0x47110F),
PACK15_4(0xEE3B33, 0xBE2F28, 0x771D19, 0x47110F),
PACK15_4(0xEE3B33, 0xBE2F28, 0x771D19, 0x47110F)
};

static const unsigned short twb64_241_brilliant_diamond_blue[] = {
PACK15_4(0x7FBBE1, 0x6595B4, 0x3F5D70, 0x263843),
PACK15_4(0x7FBBE1, 0x6595B4, 0x3F5D70, 0x263843),
PACK15_4(0x7FBBE1, 0x6595B4, 0x3F5D70, 0x263843)
};

static const unsigned short twb64_242_shining_pearl_pink[] = {
PACK15_4(0xD28EA0, 0xA87180, 0x694750, 0x3F2A30),
PACK15_4(0xD28EA0, 0xA87180, 0x694750, 0x3F2A30),
PACK15_4(0xD28EA0, 0xA87180, 0x694750, 0x3F2A30)
};

static const unsigned short twb64_243_funimation_melon[] = {
PACK15_4(0x96FF00, 0xFF149F, 0x5B0BB5, 0x000000),
PACK15_4(0x96FF00, 0xFF149F, 0x5B0BB5, 0x000000),
PACK15_4(0x96FF00, 0xFF149F, 0x5B0BB5, 0x000000)
};

static const unsigned short twb64_244_teyvat_brown[] = {
PACK15_4(0xB89469, 0x937654, 0x5C4A34, 0x372C1F),
PACK15_4(0xB89469, 0x937654, 0x5C4A34, 0x372C1F),
PACK15_4(0xB89469, 0x937654, 0x5C4A34, 0x372C1F)
};

static const unsigned short twb64_245_chozo_blue[] = {
PACK15_4(0x4EB3E1, 0x3E8FB4, 0x275970, 0x173543),
PACK15_4(0x4EB3E1, 0x3E8FB4, 0x275970, 0x173543),
PACK15_4(0x4EB3E1, 0x3E8FB4, 0x275970, 0x173543)
};

static const unsigned short twb64_246_spotify_green[] = {
PACK15_4(0x1ED760, 0x18AC4C, 0x0F6B30, 0x09401C),
PACK15_4(0x1ED760, 0x18AC4C, 0x0F6B30, 0x09401C),
PACK15_4(0x1ED760, 0x18AC4C, 0x0F6B30, 0x09401C)
};

static const unsigned short twb64_247_dr_pepper_red[] = {
PACK15_4(0x8A2231, 0x6E1B27, 0x451118, 0x290A0E),
PACK15_4(0x8A2231, 0x6E1B27, 0x451118, 0x290A0E),
PACK15_4(0x8A2231, 0x6E1B27, 0x451118, 0x290A0E)
};

static const unsigned short twb64_248_nhk_silver_gray[] = {
PACK15_4(0x808080, 0x666666, 0x404040, 0x262626),
PACK15_4(0x808080, 0x666666, 0x404040, 0x262626),
PACK15_4(0x808080, 0x666666, 0x404040, 0x262626)
};

static const unsigned short twb64_249_dunkin_vision[] = {
PACK15_4(0xFF6E0C, 0xF20C90, 0x910756, 0x48032B),
PACK15_4(0xFF6E0C, 0xF20C90, 0x910756, 0x48032B),
PACK15_4(0xFF6E0C, 0xF20C90, 0x910756, 0x48032B)
};

static const unsigned short twb64_250_deku_gamma_palette[] = {
PACK15_4(0xF0E8D6, 0xB6BEC8, 0x166668, 0x24262B),
PACK15_4(0xF0E8D6, 0xB6BEC8, 0x166668, 0x24262B),
PACK15_4(0xF0E8D6, 0xB6BEC8, 0x166668, 0x24262B)
};

static const unsigned short twb64_251_universal_studios_blue[] = {
PACK15_4(0x036CE2, 0x0256B4, 0x013671, 0x002043),
PACK15_4(0x036CE2, 0x0256B4, 0x013671, 0x002043),
PACK15_4(0x036CE2, 0x0256B4, 0x013671, 0x002043)
};

static const unsigned short twb64_252_hogwarts_goldius[] = {
PACK15_4(0xB6A571, 0x91845A, 0x5B5238, 0x363121),
PACK15_4(0xB6A571, 0x91845A, 0x5B5238, 0x363121),
PACK15_4(0xB6A571, 0x91845A, 0x5B5238, 0x363121)
};

static const unsigned short twb64_253_kentucky_fried_red[] = {
PACK15_4(0xAB182F, 0x881325, 0x550C17, 0x33070E),
PACK15_4(0xAB182F, 0x881325, 0x550C17, 0x33070E),
PACK15_4(0xAB182F, 0x881325, 0x550C17, 0x33070E)
};

static const unsigned short twb64_254_cheeto_orange[] = {
PACK15_4(0xE57600, 0xB75E00, 0x723B00, 0x442300),
PACK15_4(0xE57600, 0xB75E00, 0x723B00, 0x442300),
PACK15_4(0xE57600, 0xB75E00, 0x723B00, 0x442300)
};

static const unsigned short twb64_255_namco_idol_pink[] = {
PACK15_4(0xFF74B8, 0xCC5C93, 0x7F3A5C, 0x4C2237),
PACK15_4(0xFF74B8, 0xCC5C93, 0x7F3A5C, 0x4C2237),
PACK15_4(0xFF74B8, 0xCC5C93, 0x7F3A5C, 0x4C2237)
};

static const unsigned short twb64_256_dominos_pizza_vision[] = {
PACK15_4(0xFFFFFF, 0xE31837, 0x006491, 0x000000),
PACK15_4(0xFFFFFF, 0xE31837, 0x006491, 0x000000),
PACK15_4(0xFFFFFF, 0xE31837, 0x006491, 0x000000)
};

static const unsigned short twb64_257_pac_man_vision[] = {
PACK15_4(0xFFFF00, 0xFFB897, 0x3732FF, 0x000000),
PACK15_4(0xFFFF00, 0xFFB897, 0x3732FF, 0x000000),
PACK15_4(0xFFFF00, 0xFFB897, 0x3732FF, 0x000000)
};

static const unsigned short twb64_258_bills_pc_screen[] = {
PACK15_4(0xF87800, 0xB86000, 0x783800, 0x000000),
PACK15_4(0xF87800, 0xB86000, 0x783800, 0x000000),
PACK15_4(0xF87800, 0xB86000, 0x783800, 0x000000)
};

static const unsigned short twb64_259_ebott_prolouge[] = {
PACK15_4(0xC08226, 0x854A1D, 0x4A290B, 0x2C1708),
PACK15_4(0xC08226, 0x854A1D, 0x4A290B, 0x2C1708),
PACK15_4(0xC08226, 0x854A1D, 0x4A290B, 0x2C1708)
};

static const unsigned short twb64_260_fools_gold_and_silver[] = {
PACK15_4(0xC5C66D, 0x97A1B0, 0x5A6069, 0x2D3034),
PACK15_4(0xC5C66D, 0x97A1B0, 0x5A6069, 0x2D3034),
PACK15_4(0xC5C66D, 0x97A1B0, 0x5A6069, 0x2D3034)
};

static const unsigned short twb64_261_uta_vision[] = {
PACK15_4(0xF4F4F4, 0xF1A7B5, 0xE24465, 0x262C48),
PACK15_4(0xF4F4F4, 0xF1A7B5, 0xE24465, 0x262C48),
PACK15_4(0xF4F4F4, 0xF1A7B5, 0xE24465, 0x262C48)
};

static const unsigned short twb64_262_metallic_paldea_brass[] = {
PACK15_4(0xA29834, 0x817929, 0x514C1A, 0x302D0F),
PACK15_4(0xA29834, 0x817929, 0x514C1A, 0x302D0F),
PACK15_4(0xA29834, 0x817929, 0x514C1A, 0x302D0F)
};

static const unsigned short twb64_263_classy_christmas[] = {
PACK15_4(0xE8E7DF, 0x8BAB95, 0x9E5C5E, 0x534D57),
PACK15_4(0xE8E7DF, 0x8BAB95, 0x9E5C5E, 0x534D57),
PACK15_4(0xE8E7DF, 0x8BAB95, 0x9E5C5E, 0x534D57)
};

static const unsigned short twb64_264_winter_christmas[] = {
PACK15_4(0xDDDDDD, 0x65B08F, 0xAE3B40, 0x341113),
PACK15_4(0xDDDDDD, 0x65B08F, 0xAE3B40, 0x341113),
PACK15_4(0xDDDDDD, 0x65B08F, 0xAE3B40, 0x341113)
};

static const unsigned short twb64_265_idol_world_tricolor[] = {
PACK15_4(0xFFC30B, 0xF34F6D, 0x2681C8, 0x0B263C),
PACK15_4(0xFFC30B, 0xF34F6D, 0x2681C8, 0x0B263C),
PACK15_4(0xFFC30B, 0xF34F6D, 0x2681C8, 0x0B263C)
};

static const unsigned short twb64_266_inkling_tricolor[] = {
PACK15_4(0xEAFF3D, 0xFF505E, 0x603BFF, 0x1C114C),
PACK15_4(0xEAFF3D, 0xFF505E, 0x603BFF, 0x1C114C),
PACK15_4(0xEAFF3D, 0xFF505E, 0x603BFF, 0x1C114C)
};

static const unsigned short twb64_267_7_eleven_color_combo[] = {
PACK15_4(0xFF6C00, 0xEB0F2A, 0x147350, 0x062218),
PACK15_4(0xFF6C00, 0xEB0F2A, 0x147350, 0x062218),
PACK15_4(0xFF6C00, 0xEB0F2A, 0x147350, 0x062218)
};

static const unsigned short twb64_268_pac_palette[] = {
PACK15_4(0xFFD800, 0xFF8C00, 0xDC0000, 0x420000),
PACK15_4(0xFFD800, 0xFF8C00, 0xDC0000, 0x420000),
PACK15_4(0xFFD800, 0xFF8C00, 0xDC0000, 0x420000)
};

static const unsigned short twb64_269_vulnerable_blue[] = {
PACK15_4(0x3732FF, 0x7F5C4B, 0xCC9378, 0xFFB897),
PACK15_4(0x3732FF, 0x7F5C4B, 0xCC9378, 0xFFB897),
PACK15_4(0x3732FF, 0x7F5C4B, 0xCC9378, 0xFFB897)
};

static const unsigned short twb64_270_nightvision_green[] = {
PACK15_4(0x1E390E, 0x335F17, 0x519825, 0x66BF2F),
PACK15_4(0x1E390E, 0x335F17, 0x519825, 0x66BF2F),
PACK15_4(0x1E390E, 0x335F17, 0x519825, 0x66BF2F)
};

static const unsigned short twb64_271_bandai_namco_tricolor[] = {
PACK15_4(0xF6B700, 0xDF4F61, 0x0069B1, 0x001F35),
PACK15_4(0xF6B700, 0xDF4F61, 0x0069B1, 0x001F35),
PACK15_4(0xF6B700, 0xDF4F61, 0x0069B1, 0x001F35)
};

static const unsigned short twb64_272_gold_silver_and_bronze[] = {
PACK15_4(0xBEB049, 0x86949A, 0x996843, 0x2D1F14),
PACK15_4(0xBEB049, 0x86949A, 0x996843, 0x2D1F14),
PACK15_4(0xBEB049, 0x86949A, 0x996843, 0x2D1F14)
};

static const unsigned short twb64_273_deku_vigilante_palette[] = {
PACK15_4(0xADA89A, 0x878B92, 0x38534E, 0x131315),
PACK15_4(0xADA89A, 0x878B92, 0x38534E, 0x131315),
PACK15_4(0xADA89A, 0x878B92, 0x38534E, 0x131315)
};

static const unsigned short twb64_274_super_famicom_supreme[] = {
PACK15_4(0xFEDA5A, 0x44AC71, 0xD94040, 0x0846BA),
PACK15_4(0xFEDA5A, 0x44AC71, 0xD94040, 0x0846BA),
PACK15_4(0xFEDA5A, 0x44AC71, 0xD94040, 0x0846BA)
};

static const unsigned short twb64_275_absorbent_and_yellow[] = {
PACK15_4(0xFFF752, 0xAEC600, 0x687600, 0x343B00),
PACK15_4(0xFFF752, 0xAEC600, 0x687600, 0x343B00),
PACK15_4(0xFFF752, 0xAEC600, 0x687600, 0x343B00)
};

static const unsigned short twb64_276_765pro_tricolor[] = {
PACK15_4(0xB4E04B, 0xE22B30, 0x2743D2, 0x0B143F),
PACK15_4(0xB4E04B, 0xE22B30, 0x2743D2, 0x0B143F),
PACK15_4(0xB4E04B, 0xE22B30, 0x2743D2, 0x0B143F)
};

static const unsigned short twb64_277_gamecube_glimmer[] = {
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318),
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318),
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318)
};

static const unsigned short twb64_278_1st_vision_pastel[] = {
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318),
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318),
PACK15_4(0xB6BED3, 0x11A396, 0xCF4151, 0x3E1318)
};

static const unsigned short twb64_279_perfect_majin_emperor[] = {
PACK15_4(0xFDC1BF, 0xA3B453, 0x8550A9, 0x271832),
PACK15_4(0xFDC1BF, 0xA3B453, 0x8550A9, 0x271832),
PACK15_4(0xFDC1BF, 0xA3B453, 0x8550A9, 0x271832)
};

static const unsigned short twb64_280_j_pop_idol_sherbet[] = {
PACK15_4(0xF19DB5, 0x5BBEE5, 0x812990, 0x260C2B),
PACK15_4(0xF19DB5, 0x5BBEE5, 0x812990, 0x260C2B),
PACK15_4(0xF19DB5, 0x5BBEE5, 0x812990, 0x260C2B)
};

static const unsigned short twb64_281_ryuuguu_sunset[] = {
PACK15_4(0xFFE43F, 0xFD99E1, 0x9238BE, 0x2B1039),
PACK15_4(0xFFE43F, 0xFD99E1, 0x9238BE, 0x2B1039),
PACK15_4(0xFFE43F, 0xFD99E1, 0x9238BE, 0x2B1039)
};

static const unsigned short twb64_282_tropical_starfall[] = {
PACK15_4(0x63FDFB, 0xEF58F7, 0x4344C1, 0x141439),
PACK15_4(0x63FDFB, 0xEF58F7, 0x4344C1, 0x141439),
PACK15_4(0x63FDFB, 0xEF58F7, 0x4344C1, 0x141439)
};

static const unsigned short twb64_283_colorful_horizons[] = {
PACK15_4(0xF6CF72, 0x60BDC7, 0xD15252, 0x373939),
PACK15_4(0xF6CF72, 0x60BDC7, 0xD15252, 0x373939),
PACK15_4(0xF6CF72, 0x60BDC7, 0xD15252, 0x373939)
};

static const unsigned short twb64_284_blackpink_blink_pink[] = {
PACK15_4(0xF4A7BA, 0xC38594, 0x7A535D, 0x493237),
PACK15_4(0xF4A7BA, 0xC38594, 0x7A535D, 0x493237),
PACK15_4(0xF4A7BA, 0xC38594, 0x7A535D, 0x493237)
};

static const unsigned short twb64_285_dmg_switch[] = {
PACK15_4(0x8CAD28, 0x6C9421, 0x426B29, 0x214231),
PACK15_4(0x8CAD28, 0x6C9421, 0x426B29, 0x214231),
PACK15_4(0x8CAD28, 0x6C9421, 0x426B29, 0x214231)
};

static const unsigned short twb64_286_pocket_switch[] = {
PACK15_4(0xB5C69C, 0x8D9C7B, 0x637251, 0x303820),
PACK15_4(0xB5C69C, 0x8D9C7B, 0x637251, 0x303820),
PACK15_4(0xB5C69C, 0x8D9C7B, 0x637251, 0x303820)
};

static const unsigned short twb64_287_sunny_passion_paradise[] = {
PACK15_4(0xF0BA40, 0xE06846, 0x1E6CAE, 0x092034),
PACK15_4(0xF0BA40, 0xE06846, 0x1E6CAE, 0x092034),
PACK15_4(0xF0BA40, 0xE06846, 0x1E6CAE, 0x092034)
};

static const unsigned short twb64_288_saiyan_beast_silver[] = {
PACK15_4(0x969BAF, 0x787C8C, 0x4B4D57, 0x2D2E34),
PACK15_4(0x969BAF, 0x787C8C, 0x4B4D57, 0x2D2E34),
PACK15_4(0x969BAF, 0x787C8C, 0x4B4D57, 0x2D2E34)
};

static const unsigned short twb64_289_radiant_smile_ramp[] = {
PACK15_4(0xFFF89B, 0x01B3C4, 0xE6016B, 0x1B1C81),
PACK15_4(0xFFF89B, 0x01B3C4, 0xE6016B, 0x1B1C81),
PACK15_4(0xFFF89B, 0x01B3C4, 0xE6016B, 0x1B1C81)
};

static const unsigned short twb64_290_a_rise_blue[] = {
PACK15_4(0x30559C, 0x26447C, 0x182A4E, 0x0E192E),
PACK15_4(0x30559C, 0x26447C, 0x182A4E, 0x0E192E),
PACK15_4(0x30559C, 0x26447C, 0x182A4E, 0x0E192E)
};

static const unsigned short twb64_291_tropical_twice_apricot[] = {
PACK15_4(0xFCC89B, 0xFF5FA2, 0x993961, 0x4C1C30),
PACK15_4(0xFCC89B, 0xFF5FA2, 0x993961, 0x4C1C30),
PACK15_4(0xFCC89B, 0xFF5FA2, 0x993961, 0x4C1C30)
};

static const unsigned short twb64_292_odyssey_boy[] = {
PACK15_4(0xACBE8C, 0x7E8E67, 0x505445, 0x222421),
PACK15_4(0xACBE8C, 0x7E8E67, 0x505445, 0x222421),
PACK15_4(0xACBE8C, 0x7E8E67, 0x505445, 0x222421)
};

static const unsigned short twb64_293_frog_coin_green[] = {
PACK15_4(0xFFF7DE, 0x00EF00, 0x398400, 0x003900),
PACK15_4(0xFFF7DE, 0x00EF00, 0x398400, 0x003900),
PACK15_4(0xFFF7DE, 0x00EF00, 0x398400, 0x003900)
};

static const unsigned short twb64_294_garfield_vision[] = {
PACK15_4(0xF5EA8B, 0xE59436, 0x964220, 0x2D1309),
PACK15_4(0xF5EA8B, 0xE59436, 0x964220, 0x2D1309),
PACK15_4(0xF5EA8B, 0xE59436, 0x964220, 0x2D1309)
};

static const unsigned short twb64_295_bedrock_caveman_vision[] = {
PACK15_4(0xFF7F00, 0x009EB8, 0x005E6E, 0x002F37),
PACK15_4(0xFF7F00, 0x009EB8, 0x005E6E, 0x002F37),
PACK15_4(0xFF7F00, 0x009EB8, 0x005E6E, 0x002F37)
};

static const unsigned short twb64_296_bangtan_army_purple[] = {
PACK15_4(0x8048D8, 0x6639AC, 0x40246C, 0x261540),
PACK15_4(0x8048D8, 0x6639AC, 0x40246C, 0x261540),
PACK15_4(0x8048D8, 0x6639AC, 0x40246C, 0x261540)
};

static const unsigned short twb64_297_le_sserafim_fearless_blue[] = {
PACK15_4(0x81A5F9, 0x6784C7, 0x40527C, 0x26314A),
PACK15_4(0x81A5F9, 0x6784C7, 0x40527C, 0x26314A),
PACK15_4(0x81A5F9, 0x6784C7, 0x40527C, 0x26314A)
};

static const unsigned short twb64_298_baja_blast_beach[] = {
PACK15_4(0xDBE441, 0x83CCC5, 0x4E7A76, 0x273D3B),
PACK15_4(0xDBE441, 0x83CCC5, 0x4E7A76, 0x273D3B),
PACK15_4(0xDBE441, 0x83CCC5, 0x4E7A76, 0x273D3B)
};

static const unsigned short twb64_299_3ds_virtual_console_green[] = {
PACK15_4(0xBDFF21, 0x9CEF29, 0x5A8C42, 0x4A4A4A),
PACK15_4(0xBDFF21, 0x9CEF29, 0x5A8C42, 0x4A4A4A),
PACK15_4(0xBDFF21, 0x9CEF29, 0x5A8C42, 0x4A4A4A)
};

static const unsigned short twb64_300_wonder_purple[] = {
PACK15_4(0xE658DF, 0xB846B2, 0x732C6F, 0x451A42),
PACK15_4(0xE658DF, 0xB846B2, 0x732C6F, 0x451A42),
PACK15_4(0xE658DF, 0xB846B2, 0x732C6F, 0x451A42)
};

//
// Palettes by PixelShift
// https://github.com/libretro/gambatte-libretro/issues/219
//
static const unsigned short pixelshift_01_arctic_green[] = {
	PACK15_4(0xC4F0C2, 0x5AB9A8, 0x1E606E, 0x2D1B00),
	PACK15_4(0xC4F0C2, 0x5AB9A8, 0x1E606E, 0x2D1B00),
	PACK15_4(0xC4F0C2, 0x5AB9A8, 0x1E606E, 0x2D1B00)
};

static const unsigned short pixelshift_02_arduboy[] = {
	PACK15_4(0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF),
	PACK15_4(0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF),
	PACK15_4(0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF)
};

static const unsigned short pixelshift_03_bgb_0_3_emulator[] = {
	PACK15_4(0xE8FCCC, 0xACD490, 0x548C70, 0x142C38),
	PACK15_4(0xE8FCCC, 0xACD490, 0x548C70, 0x142C38),
	PACK15_4(0xE8FCCC, 0xACD490, 0x548C70, 0x142C38)
};

static const unsigned short pixelshift_04_camouflage[] = {
	PACK15_4(0x968969, 0x48623F, 0x60513A, 0x272727),
	PACK15_4(0x968969, 0x48623F, 0x60513A, 0x272727),
	PACK15_4(0x968969, 0x48623F, 0x60513A, 0x272727)
};

static const unsigned short pixelshift_05_chocolate_bar[] = {
	PACK15_4(0xF7D9BA, 0xC2925C, 0x975447, 0x310300),
	PACK15_4(0xF7D9BA, 0xC2925C, 0x975447, 0x310300),
	PACK15_4(0xF7D9BA, 0xC2925C, 0x975447, 0x310300)
};

static const unsigned short pixelshift_06_cmyk[] = {
	PACK15_4(0x8CDFFD, 0xFD80C6, 0xFFF68C, 0x3E3E3E),
	PACK15_4(0x8CDFFD, 0xFD80C6, 0xFFF68C, 0x3E3E3E),
	PACK15_4(0x8CDFFD, 0xFD80C6, 0xFFF68C, 0x3E3E3E)
};

static const unsigned short pixelshift_07_cotton_candy[] = {
	PACK15_4(0xF8FFFF, 0x90D9FC, 0xFF7A99, 0x000000),
	PACK15_4(0xF8FFFF, 0x90D9FC, 0xFF7A99, 0x000000),
	PACK15_4(0xF8FFFF, 0x90D9FC, 0xFF7A99, 0x000000)
};

static const unsigned short pixelshift_08_easy_greens[] = {
	PACK15_4(0xEBDD77, 0xA1BC00, 0x0D8833, 0x004333),
	PACK15_4(0xEBDD77, 0xA1BC00, 0x0D8833, 0x004333),
	PACK15_4(0xEBDD77, 0xA1BC00, 0x0D8833, 0x004333)
};

static const unsigned short pixelshift_09_gamate[] = {
	PACK15_4(0x33CC33, 0x20983E, 0x006666, 0x003958),
	PACK15_4(0x33CC33, 0x20983E, 0x006666, 0x003958),
	PACK15_4(0x33CC33, 0x20983E, 0x006666, 0x003958)
};

static const unsigned short pixelshift_10_game_boy_light[] = {
	PACK15_4(0x02EA75, 0x01B158, 0x00793C, 0x004120),
	PACK15_4(0x02EA75, 0x01B158, 0x00793C, 0x004120),
	PACK15_4(0x02EA75, 0x01B158, 0x00793C, 0x004120)
};

static const unsigned short pixelshift_11_game_boy_pocket[] = {
	PACK15_4(0x929775, 0x656A55, 0x535849, 0x282C26),
	PACK15_4(0x929775, 0x656A55, 0x535849, 0x282C26),
	PACK15_4(0x929775, 0x656A55, 0x535849, 0x282C26)
};

static const unsigned short pixelshift_12_game_boy_pocket_alt[] = {
	PACK15_4(0x89A18E, 0x758A78, 0x627262, 0x30372F),
	PACK15_4(0x89A18E, 0x758A78, 0x627262, 0x30372F),
	PACK15_4(0x89A18E, 0x758A78, 0x627262, 0x30372F)
};

static const unsigned short pixelshift_13_game_pocket_computer[] = {
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000)
};

static const unsigned short pixelshift_14_game_and_watch_ball[] = {
	PACK15_4(0x8E9F8D, 0x8E9F8D, 0x343837, 0x343837),
	PACK15_4(0x8E9F8D, 0x8E9F8D, 0x343837, 0x343837),
	PACK15_4(0x8E9F8D, 0x8E9F8D, 0x343837, 0x343837)
};

static const unsigned short pixelshift_15_gb_backlight_blue[] = {
	PACK15_4(0x3FC1FF, 0x068EFF, 0x0058FF, 0x00006D),
	PACK15_4(0x3FC1FF, 0x068EFF, 0x0058FF, 0x00006D),
	PACK15_4(0x3FC1FF, 0x068EFF, 0x0058FF, 0x00006D)
};

static const unsigned short pixelshift_16_gb_backlight_faded[] = {
	PACK15_4(0xFFFFFF, 0xE2E2E2, 0x9E9E9E, 0x808080),
	PACK15_4(0xFFFFFF, 0xE2E2E2, 0x9E9E9E, 0x808080),
	PACK15_4(0xFFFFFF, 0xE2E2E2, 0x9E9E9E, 0x808080)
};

static const unsigned short pixelshift_17_gb_backlight_orange[] = {
	PACK15_4(0xFDA910, 0xF88806, 0xE96B08, 0x833403),
	PACK15_4(0xFDA910, 0xF88806, 0xE96B08, 0x833403),
	PACK15_4(0xFDA910, 0xF88806, 0xE96B08, 0x833403)
};

static const unsigned short pixelshift_18_gb_backlight_white_[] = {
	PACK15_4(0xBAC9E8, 0x9091C9, 0x6731FF, 0x0C0FD0),
	PACK15_4(0xBAC9E8, 0x9091C9, 0x6731FF, 0x0C0FD0),
	PACK15_4(0xBAC9E8, 0x9091C9, 0x6731FF, 0x0C0FD0)
};

static const unsigned short pixelshift_19_gb_backlight_yellow_dark[] = {
	PACK15_4(0x99931A, 0x8B7E0F, 0x63530F, 0x4F3B08),
	PACK15_4(0x99931A, 0x8B7E0F, 0x63530F, 0x4F3B08),
	PACK15_4(0x99931A, 0x8B7E0F, 0x63530F, 0x4F3B08)
};

static const unsigned short pixelshift_20_gb_bootleg[] = {
	PACK15_4(0x89AABB, 0x9EB481, 0x4A714A, 0x4A2D2C),
	PACK15_4(0xF7C6B5, 0xDE7B52, 0x9C3900, 0x4A3100),
	PACK15_4(0xF7C6B5, 0xDE7B52, 0x9C3900, 0x4A3100)
};

static const unsigned short pixelshift_21_gb_hunter[] = {
	PACK15_4(0xB4B0AD, 0x7D7A78, 0x464544, 0x101010),
	PACK15_4(0xB4B0AD, 0x7D7A78, 0x464544, 0x101010),
	PACK15_4(0xB4B0AD, 0x7D7A78, 0x464544, 0x101010)
};

static const unsigned short pixelshift_22_gb_kiosk[] = {
	PACK15_4(0x01FC00, 0x00D100, 0x00BF00, 0x005C09),
	PACK15_4(0x01FC00, 0x00D100, 0x00BF00, 0x005C09),
	PACK15_4(0x01FC00, 0x00D100, 0x00BF00, 0x005C09)
};

static const unsigned short pixelshift_23_gb_kiosk_2[] = {
	PACK15_4(0xFFFF93, 0xC2DE00, 0x536500, 0x003500),
	PACK15_4(0xFFFF93, 0xC2DE00, 0x536500, 0x003500),
	PACK15_4(0xFFFF93, 0xC2DE00, 0x536500, 0x003500)
};

static const unsigned short pixelshift_24_gb_new[] = {
	PACK15_4(0x88CF45, 0x58B865, 0x23854B, 0x1D684B),
	PACK15_4(0x88CF45, 0x58B865, 0x23854B, 0x1D684B),
	PACK15_4(0x88CF45, 0x58B865, 0x23854B, 0x1D684B)
};

static const unsigned short pixelshift_25_gb_nuked[] = {
	PACK15_4(0x70AC80, 0xA88D08, 0x85211F, 0x00004F),
	PACK15_4(0x70AC80, 0xA88D08, 0x85211F, 0x00004F),
	PACK15_4(0x70AC80, 0xA88D08, 0x85211F, 0x00004F)
};

static const unsigned short pixelshift_26_gb_old[] = {
	PACK15_4(0x7E8416, 0x577B46, 0x385D49, 0x2E463D),
	PACK15_4(0x7E8416, 0x577B46, 0x385D49, 0x2E463D),
	PACK15_4(0x7E8416, 0x577B46, 0x385D49, 0x2E463D)
};

static const unsigned short pixelshift_27_gbp_bivert[] = {
	PACK15_4(0xADD2FE, 0x73AAE6, 0x3982CF, 0x005BB8),
	PACK15_4(0xADD2FE, 0x73AAE6, 0x3982CF, 0x005BB8),
	PACK15_4(0xADD2FE, 0x73AAE6, 0x3982CF, 0x005BB8)
};

static const unsigned short pixelshift_28_gb_washed_yellow_backlight[] = {
	PACK15_4(0xC4F796, 0x8DC472, 0x6488AA, 0x1C4599),
	PACK15_4(0xC4F796, 0x8DC472, 0x6488AA, 0x1C4599),
	PACK15_4(0xC4F796, 0x8DC472, 0x6488AA, 0x1C4599)
};

static const unsigned short pixelshift_29_ghost[] = {
	PACK15_4(0xFFF1EB, 0xE096A8, 0x70579C, 0x0A0912),
	PACK15_4(0xFFF1EB, 0xE096A8, 0x70579C, 0x0A0912),
	PACK15_4(0xFFF1EB, 0xE096A8, 0x70579C, 0x0A0912)
};

static const unsigned short pixelshift_30_glow_in_the_dark[] = {
	PACK15_4(0x4DEEAA, 0x339E71, 0x194F38, 0x000000),
	PACK15_4(0x4DEEAA, 0x339E71, 0x194F38, 0x000000),
	PACK15_4(0x4DEEAA, 0x339E71, 0x194F38, 0x000000)
};

static const unsigned short pixelshift_31_gold_bar[] = {
	PACK15_4(0xFCFFD6, 0xFFDB57, 0xD27D2C, 0x854C30),
	PACK15_4(0xFCFFD6, 0xFFDB57, 0xD27D2C, 0x854C30),
	PACK15_4(0xFCFFD6, 0xFFDB57, 0xD27D2C, 0x854C30)
};

static const unsigned short pixelshift_32_grapefruit[] = {
	PACK15_4(0xFFF5DD, 0xF4B26B, 0xB76591, 0x65296C),
	PACK15_4(0xFFF5DD, 0xF4B26B, 0xB76591, 0x65296C),
	PACK15_4(0xFFF5DD, 0xF4B26B, 0xB76591, 0x65296C)
};

static const unsigned short pixelshift_33_gray_green_mix[] = {
	PACK15_4(0xE8E8E8, 0xA0A0A0, 0x585858, 0x101010),
	PACK15_4(0xE0F8D0, 0x88C070, 0x346856, 0x081820),
	PACK15_4(0xE0F8D0, 0x88C070, 0x346856, 0x081820)
};

static const unsigned short pixelshift_34_missingno[] = {
	PACK15_4(0xFFFFFF, 0xF0B088, 0x807098, 0x181010),
	PACK15_4(0xFFFFFF, 0xF0B088, 0x807098, 0x181010),
	PACK15_4(0xFFFFFF, 0xF0B088, 0x807098, 0x181010)
};

static const unsigned short pixelshift_35_ms_dos[] = {
	PACK15_4(0x000000, 0x000000, 0x17F117, 0x17F117),
	PACK15_4(0x000000, 0x000000, 0x17F117, 0x17F117),
	PACK15_4(0x000000, 0x000000, 0x17F117, 0x17F117)
};

static const unsigned short pixelshift_36_newspaper[] = {
	PACK15_4(0x646464, 0x505050, 0x323232, 0x000000),
	PACK15_4(0x646464, 0x505050, 0x323232, 0x000000),
	PACK15_4(0x646464, 0x505050, 0x323232, 0x000000)
};

static const unsigned short pixelshift_37_pip_boy[] = {
	PACK15_4(0x05160C, 0x062D1A, 0x0DB254, 0x1BC364),
	PACK15_4(0x05160C, 0x062D1A, 0x0DB254, 0x1BC364),
	PACK15_4(0x05160C, 0x062D1A, 0x0DB254, 0x1BC364)
};

static const unsigned short pixelshift_38_pocket_girl[] = {
	PACK15_4(0xF7FFAE, 0xFFB3CB, 0x96FBC7, 0x74569B),
	PACK15_4(0xF7FFAE, 0xFFB3CB, 0x96FBC7, 0x74569B),
	PACK15_4(0xF7FFAE, 0xFFB3CB, 0x96FBC7, 0x74569B)
};

static const unsigned short pixelshift_39_silhouette[] = {
	PACK15_4(0xE8E8E8, 0xA0A0A0, 0x585858, 0x000000),
	PACK15_4(0x000000, 0x000000, 0x000000, 0x000000),
	PACK15_4(0x000000, 0x000000, 0x000000, 0x000000)
};

static const unsigned short pixelshift_40_sunburst[] = {
	PACK15_4(0xE1EEC3, 0xECE774, 0xF6903D, 0xF05053),
	PACK15_4(0xE1EEC3, 0xECE774, 0xF6903D, 0xF05053),
	PACK15_4(0xE1EEC3, 0xECE774, 0xF6903D, 0xF05053)
};

static const unsigned short pixelshift_41_technicolor[] = {
	PACK15_4(0xFFF5F7, 0xE6B8C1, 0x456B73, 0x15191A),
	PACK15_4(0xFFF5F7, 0xE6B8C1, 0x456B73, 0x15191A),
	PACK15_4(0xFFF5F7, 0xE6B8C1, 0x456B73, 0x15191A)
};

static const unsigned short pixelshift_42_tron[] = {
	PACK15_4(0x000000, 0x2A0055, 0x5500AA, 0x8000FF),
	PACK15_4(0xAAA9B2, 0x757576, 0x0099EC, 0x0065A8),
	PACK15_4(0xAAA9B2, 0x757576, 0x0099EC, 0x0065A8)
};

static const unsigned short pixelshift_43_vaporwave[] = {
	PACK15_4(0x4ADBE5, 0xE54CB0, 0x574797, 0x1A2D40),
	PACK15_4(0x4ADBE5, 0xE54CB0, 0x574797, 0x1A2D40),
	PACK15_4(0x4ADBE5, 0xE54CB0, 0x574797, 0x1A2D40)
};

static const unsigned short pixelshift_44_virtual_boy[] = {
	PACK15_4(0xD90708, 0x960708, 0x530607, 0x070303),
	PACK15_4(0xD90708, 0x960708, 0x530607, 0x070303),
	PACK15_4(0xD90708, 0x960708, 0x530607, 0x070303)
};

static const unsigned short pixelshift_45_wish[] = {
	PACK15_4(0x8BE5FF, 0x608FCF, 0x7550E8, 0x622E4C),
	PACK15_4(0x8BE5FF, 0x608FCF, 0x7550E8, 0x622E4C),
	PACK15_4(0x8BE5FF, 0x608FCF, 0x7550E8, 0x622E4C)
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
	{ "TWB64 012 - Virtual Vision", twb64_012_virtual_vision },
	{ "TWB64 013 - Golden Wild", twb64_013_golden_wild },
	{ "TWB64 014 - DMG-099", twb64_014_dmg_099 },
	{ "TWB64 015 - Classic Blurple", twb64_015_classic_blurple },
	{ "TWB64 016 - 765 Production Ver.", twb64_016_765_production_ver },
	{ "TWB64 017 - Superball Ivory", twb64_017_superball_ivory },
	{ "TWB64 018 - Crunchyroll Orange", twb64_018_crunchyroll_orange },
	{ "TWB64 019 - Muse Pink", twb64_019_muse_pink },
	{ "TWB64 020 - School Idol Blue", twb64_020_school_idol_blue },
	{ "TWB64 021 - Gamate Ver.", twb64_021_gamate_ver },
	{ "TWB64 022 - Greenscale Ver.", twb64_022_greenscale_ver },
	{ "TWB64 023 - Odyssey Gold", twb64_023_odyssey_gold },
	{ "TWB64 024 - Super Saiyan God", twb64_024_super_saiyan_god },
	{ "TWB64 025 - Super Saiyan Blue", twb64_025_super_saiyan_blue },
	{ "TWB64 026 - ANIMAX BLUE", twb64_026_animax_blue },
	{ "TWB64 027 - BMO Ver.", twb64_027_bmo_ver },
	{ "TWB64 028 - Game.com Ver.", twb64_028_game_com_ver },
	{ "TWB64 029 - Sanrio Pink", twb64_029_sanrio_pink },
	{ "TWB64 030 - Timmy Turner Pink", twb64_030_timmy_turner_pink },
	{ "TWB64 031 - Fairly OddPalette", twb64_031_fairly_oddpalette },
	{ "TWB64 032 - Danny Phantom Silver", twb64_032_danny_phantom_silver },
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
	{ "TWB64 043 - All Might Hero Palette", twb64_043_all_might_hero_palette },
	{ "TWB64 044 - U.A. High School Uniform", twb64_044_ua_high_school_uniform },
	{ "TWB64 045 - Pikachu Yellow", twb64_045_pikachu_yellow },
	{ "TWB64 046 - Eevee Brown", twb64_046_eevee_brown },
	{ "TWB64 047 - Microvision Ver.", twb64_047_microvision_ver },
	{ "TWB64 048 - TI-83 Ver.", twb64_048_ti83_ver },
	{ "TWB64 049 - Aegis Cherry", twb64_049_aegis_cherry },
	{ "TWB64 050 - Labo Fawn", twb64_050_labo_fawn },
	{ "TWB64 051 - MILLION LIVE GOLD!", twb64_051_million_live_gold },
	{ "TWB64 052 - Squidward Sea Foam Green", twb64_052_squidward_sea_foam_green },
	{ "TWB64 053 - VMU Ver.", twb64_053_vmu_ver },
	{ "TWB64 054 - Game Master Ver.", twb64_054_game_master_ver },
	{ "TWB64 055 - Android Green", twb64_055_android_green },
	{ "TWB64 056 - Amazon Vision", twb64_056_amazon_vision },
	{ "TWB64 057 - Google Red", twb64_057_google_red },
	{ "TWB64 058 - Google Blue", twb64_058_google_blue },
	{ "TWB64 059 - Google Yellow", twb64_059_google_yellow },
	{ "TWB64 060 - Google Green", twb64_060_google_green },
	{ "TWB64 061 - WonderSwan Ver.", twb64_061_wonderswan_ver },
	{ "TWB64 062 - Neo Geo Pocket Ver.", twb64_062_neo_geo_pocket_ver },
	{ "TWB64 063 - Dew Green", twb64_063_dew_green },
	{ "TWB64 064 - Coca-Cola Vision", twb64_064_coca_cola_vision },
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
	{ "TWB64 085 - Perfected Ultra Instinct", twb64_085_perfected_ultra_instinct },
	{ "TWB64 086 - Saint Snow Red", twb64_086_saint_snow_red },
	{ "TWB64 087 - Yellow Banana", twb64_087_yellow_banana },
	{ "TWB64 088 - Green Banana", twb64_088_green_banana },
	{ "TWB64 089 - Super Saiyan 3", twb64_089_super_saiyan_3 },
	{ "TWB64 090 - Super Saiyan Blue Evolved", twb64_090_super_saiyan_blue_evolved },
	{ "TWB64 091 - Pocket Tales Ver.", twb64_091_pocket_tales_ver },
	{ "TWB64 092 - Investigation Yellow", twb64_092_investigation_yellow },
	{ "TWB64 093 - S.E.E.S. Blue", twb64_093_sees_blue },
	{ "TWB64 094 - Ultra Instinct Sign", twb64_094_ultra_instinct_sign },
	{ "TWB64 095 - Hokage Orange", twb64_095_hokage_orange },
	{ "TWB64 096 - Straw Hat Red", twb64_096_straw_hat_red },
	{ "TWB64 097 - Sword Art Cyan", twb64_097_sword_art_cyan },
	{ "TWB64 098 - Deku Alpha Emerald", twb64_098_deku_alpha_emerald },
	{ "TWB64 099 - Blue Stripes Ver.", twb64_099_blue_stripes_ver },
	{ "TWB64 100 - Precure Marble Raspberry", twb64_100_precure_marble_raspberry },
	{ "TWB64 101 - 765PRO Pink", twb64_101_765pro_pink },
	{ "TWB64 102 - CINDERELLA Blue", twb64_102_cinderella_blue },
	{ "TWB64 103 - MILLION Yellow!", twb64_103_million_yellow },
	{ "TWB64 104 - SideM Green", twb64_104_sidem_green },
	{ "TWB64 105 - SHINY Sky Blue", twb64_105_shiny_sky_blue },
	{ "TWB64 106 - Angry Volcano Ver.", twb64_106_angry_volcano_ver },
	{ "TWB64 107 - NBA Vision", twb64_107_nba_vision },
	{ "TWB64 108 - NFL Vision", twb64_108_nfl_vision },
	{ "TWB64 109 - MLB Vision", twb64_109_mlb_vision },
	{ "TWB64 110 - Anime Digivice Ver.", twb64_110_anime_digivice_ver },
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
	{ "TWB64 121 - Tri Digivice Ver.", twb64_121_tri_digivice_ver },
	{ "TWB64 122 - Survey Corps Uniform", twb64_122_survey_corps_uniform },
	{ "TWB64 123 - Island Green", twb64_123_island_green },
	{ "TWB64 124 - Nogizaka46 Purple", twb64_124_nogizaka46_purple },
	{ "TWB64 125 - Ninja Turtle Green", twb64_125_ninja_turtle_green },
	{ "TWB64 126 - Slime Blue", twb64_126_slime_blue },
	{ "TWB64 127 - Lime Midori", twb64_127_lime_midori },
	{ "TWB64 128 - Ghostly Aoi", twb64_128_ghostly_aoi },
	{ "TWB64 129 - Retro Bogeda", twb64_129_retro_bogeda },
	{ "TWB64 130 - Royal Blue", twb64_130_royal_blue },
	{ "TWB64 131 - Neon Purple", twb64_131_neon_purple },
	{ "TWB64 132 - Neon Orange", twb64_132_neon_orange },
	{ "TWB64 133 - Moonlight Vision", twb64_133_moonlight_vision },
	{ "TWB64 134 - Rising Sun Red", twb64_134_rising_sun_red },
	{ "TWB64 135 - Burger King Color Combo", twb64_135_burger_king_color_combo },
	{ "TWB64 136 - Grand Zeno Coat", twb64_136_grand_zeno_coat },
	{ "TWB64 137 - Pac-Man Yellow", twb64_137_pac_man_yellow },
	{ "TWB64 138 - Irish Green", twb64_138_irish_green },
	{ "TWB64 139 - Goku Gi", twb64_139_goku_gi },
	{ "TWB64 140 - Dragon Ball Orange", twb64_140_dragon_ball_orange },
	{ "TWB64 141 - Christmas Gold", twb64_141_christmas_gold },
	{ "TWB64 142 - Pepsi Vision", twb64_142_pepsi_vision },
	{ "TWB64 143 - Bubblun Green", twb64_143_bubblun_green },
	{ "TWB64 144 - Bobblun Blue", twb64_144_bobblun_blue },
	{ "TWB64 145 - Baja Blast Storm", twb64_145_baja_blast_storm },
	{ "TWB64 146 - Olympic Gold", twb64_146_olympic_gold },
	{ "TWB64 147 - LisAni Orange!", twb64_147_lisani_orange },
	{ "TWB64 148 - Liella Purple!", twb64_148_liella_purple },
	{ "TWB64 149 - Olympic Silver", twb64_149_olympic_silver },
	{ "TWB64 150 - Olympic Bronze", twb64_150_olympic_bronze },
	{ "TWB64 151 - ANA Flight Blue", twb64_151_ana_flight_blue },
	{ "TWB64 152 - Nijigasaki Orange", twb64_152_nijigasaki_orange },
	{ "TWB64 153 - holoblue", twb64_153_holoblue },
	{ "TWB64 154 - WWE White and Red", twb64_154_wwe_white_and_red },
	{ "TWB64 155 - Yoshi Egg Green", twb64_155_yoshi_egg_green },
	{ "TWB64 156 - Pokedex Red", twb64_156_pokedex_red },
	{ "TWB64 157 - FamilyMart Vision", twb64_157_familymart_vision },
	{ "TWB64 158 - Xbox Green", twb64_158_xbox_green },
	{ "TWB64 159 - Sonic Mega Blue", twb64_159_sonic_mega_blue },
	{ "TWB64 160 - Sprite Green", twb64_160_sprite_green },
	{ "TWB64 161 - Scarlett Green", twb64_161_scarlett_green },
	{ "TWB64 162 - Glitchy Blue", twb64_162_glitchy_blue },
	{ "TWB64 163 - Classic LCD", twb64_163_classic_lcd },
	{ "TWB64 164 - 3DS Virtual Console Ver.", twb64_164_3ds_virtual_console_ver },
	{ "TWB64 165 - PocketStation Ver.", twb64_165_pocketstation_ver },
	{ "TWB64 166 - Timeless Gold and Red", twb64_166_timeless_gold_and_red },
	{ "TWB64 167 - Smurfy Blue", twb64_167_smurfy_blue },
	{ "TWB64 168 - Swampy Ogre Green", twb64_168_swampy_ogre_green },
	{ "TWB64 169 - Sailor Spinach Green", twb64_169_sailor_spinach_green },
	{ "TWB64 170 - Shenron Green", twb64_170_shenron_green },
	{ "TWB64 171 - Berserk Blood", twb64_171_berserk_blood },
	{ "TWB64 172 - Super Star Pink", twb64_172_super_star_pink },
	{ "TWB64 173 - Gamebuino Classic Ver.", twb64_173_gamebuino_classic_ver },
	{ "TWB64 174 - Barbie Pink", twb64_174_barbie_pink },
	{ "TWB64 175 - YOASOBI AMARANTH", twb64_175_yoasobi_amaranth },
	{ "TWB64 176 - Nokia 3310 Ver.", twb64_176_nokia_3310_ver },
	{ "TWB64 177 - Clover Green", twb64_177_clover_green },
	{ "TWB64 178 - Goku GT Gi", twb64_178_goku_gt_gi },
	{ "TWB64 179 - Famicom Disk Yellow", twb64_179_famicom_disk_yellow },
	{ "TWB64 180 - Team Rocket Uniform", twb64_180_team_rocket_uniform },
	{ "TWB64 181 - SEIKO Timely Vision", twb64_181_seiko_timely_vision },
	{ "TWB64 182 - PASTEL109", twb64_182_pastel109 },
	{ "TWB64 183 - Doraemon Tricolor", twb64_183_doraemon_tricolor},
	{ "TWB64 184 - Fury Blue", twb64_184_fury_blue },
	{ "TWB64 185 - GOOD SMILE VISION", twb64_185_good_smile_vision },
	{ "TWB64 186 - Puyo Puyo Green", twb64_186_puyo_puyo_green },
	{ "TWB64 187 - Circle K Color Combo", twb64_187_circle_k_color_combo },
	{ "TWB64 188 - Pizza Hut Red", twb64_188_pizza_hut_red },
	{ "TWB64 189 - Emerald Green", twb64_189_emerald_green },
	{ "TWB64 190 - Grand Ivory", twb64_190_grand_ivory },
	{ "TWB64 191 - Demon's Gold", twb64_191_demons_gold },
	{ "TWB64 192 - SEGA Tokyo Blue", twb64_192_sega_tokyo_blue },
	{ "TWB64 193 - Champion's Tunic", twb64_193_champions_tunic },
	{ "TWB64 194 - DK Barrel Brown", twb64_194_dk_barrel_brown },
	{ "TWB64 195 - EVA-01", twb64_195_eva_01 },
	{ "TWB64 196 - Wild West Vision", twb64_196_wild_west_vision },
	{ "TWB64 197 - Optimus Prime Palette", twb64_197_optimus_prime_palette },
	{ "TWB64 198 - niconico sea green", twb64_198_niconico_sea_green },
	{ "TWB64 199 - Duracell Copper", twb64_199_duracell_copper },
	{ "TWB64 200 - TOKYO SKYTREE CLOUDY BLUE", twb64_200_tokyo_skytree_cloudy_blue },
	{ "TWB64 201 - DMG-GOLD", twb64_201_dmg_gold },
	{ "TWB64 202 - LCD Clock Green", twb64_202_lcd_clock_green },
	{ "TWB64 203 - Famicom Frenzy", twb64_203_famicom_frenzy },
	{ "TWB64 204 - DK Arcade Blue", twb64_204_dk_arcade_blue },
	{ "TWB64 205 - Advanced Indigo", twb64_205_advanced_indigo },
	{ "TWB64 206 - Ultra Black", twb64_206_ultra_black },
	{ "TWB64 207 - Chaos Emerald Green", twb64_207_chaos_emerald_green },
	{ "TWB64 208 - Blue Bomber Vision", twb64_208_blue_bomber_vision },
	{ "TWB64 209 - Krispy Kreme Vision", twb64_209_krispy_kreme_vision },
	{ "TWB64 210 - Steam Gray", twb64_210_steam_gray },
	{ "TWB64 211 - Dream Land GB Ver.", twb64_211_dream_land_gb_ver },
	{ "TWB64 212 - Pokemon Pinball Ver.", twb64_212_pokemon_pinball_ver },
	{ "TWB64 213 - Poketch Ver.", twb64_213_poketch_ver },
	{ "TWB64 214 - COLLECTION of SaGa Ver.", twb64_214_collection_of_saga_ver },
	{ "TWB64 215 - Rocky-Valley Holiday", twb64_215_rocky_valley_holiday },
	{ "TWB64 216 - Giga Kiwi DMG", twb64_216_giga_kiwi_dmg },
	{ "TWB64 217 - DMG Pea Green", twb64_217_dmg_pea_green },
	{ "TWB64 218 - Timing Hero Ver.", twb64_218_timing_hero_ver },
	{ "TWB64 219 - Invincible Yellow and Blue", twb64_219_invincible_yellow_and_blue },
	{ "TWB64 220 - Grinchy Green", twb64_220_grinchy_green },
	{ "TWB64 221 - animate vision", twb64_221_animate_vision },
	{ "TWB64 222 - School Idol Mix", twb64_222_school_idol_mix },
	{ "TWB64 223 - Green Awakening", twb64_223_green_awakening },
	{ "TWB64 224 - Goomba Brown", twb64_224_goomba_brown },
	{ "TWB64 225 - WarioWare MicroBlue", twb64_225_warioware_microblue },
	{ "TWB64 226 - KonoSuba Sherbet", twb64_226_konosuba_sherbet },
	{ "TWB64 227 - Spooky Purple", twb64_227_spooky_purple },
	{ "TWB64 228 - Treasure Gold", twb64_228_treasure_gold },
	{ "TWB64 229 - Cherry Blossom Pink", twb64_229_cherry_blossom_pink },
	{ "TWB64 230 - Golden Trophy", twb64_230_golden_trophy },
	{ "TWB64 231 - Glacial Winter Blue", twb64_231_glacial_winter_blue },
	{ "TWB64 232 - Leprechaun Green", twb64_232_leprechaun_green },
	{ "TWB64 233 - SAITAMA SUPER BLUE", twb64_233_saitama_super_blue },
	{ "TWB64 234 - SAITAMA SUPER GREEN", twb64_234_saitama_super_green },
	{ "TWB64 235 - Duolingo Green", twb64_235_duolingo_green },
	{ "TWB64 236 - Super Mushroom Vision", twb64_236_super_mushroom_vision },
	{ "TWB64 237 - Ancient Hisuian Brown", twb64_237_ancient_husuian_brown },
	{ "TWB64 238 - Sky Pop Ivory", twb64_238_sky_pop_ivory },
	{ "TWB64 239 - LAWSON BLUE", twb64_239_lawson_blue },
	{ "TWB64 240 - Anime Expo Red", twb64_240_anime_expo_red },
	{ "TWB64 241 - Brilliant Diamond Blue", twb64_241_brilliant_diamond_blue },
	{ "TWB64 242 - Shining Pearl Pink", twb64_242_shining_pearl_pink },
	{ "TWB64 243 - Funimation Melon", twb64_243_funimation_melon },
	{ "TWB64 244 - Teyvat Brown", twb64_244_teyvat_brown },
	{ "TWB64 245 - Chozo Blue", twb64_245_chozo_blue },
	{ "TWB64 246 - Spotify Green", twb64_246_spotify_green },
	{ "TWB64 247 - Dr Pepper Red", twb64_247_dr_pepper_red },
	{ "TWB64 248 - NHK Silver Gray", twb64_248_nhk_silver_gray },
	{ "TWB64 249 - Dunkin' Vision", twb64_249_dunkin_vision },
	{ "TWB64 250 - Deku Gamma Palette", twb64_250_deku_gamma_palette },
	{ "TWB64 251 - Universal Studios Blue", twb64_251_universal_studios_blue },
	{ "TWB64 252 - Hogwarts Goldius", twb64_252_hogwarts_goldius },
	{ "TWB64 253 - Kentucky Fried Red", twb64_253_kentucky_fried_red },
	{ "TWB64 254 - Cheeto Orange", twb64_254_cheeto_orange },
	{ "TWB64 255 - Namco Idol Pink", twb64_255_namco_idol_pink },
	{ "TWB64 256 - Domino's Pizza Vision", twb64_256_dominos_pizza_vision },
	{ "TWB64 257 - Pac-Man Vision", twb64_257_pac_man_vision },
	{ "TWB64 258 - Bill's PC Screen", twb64_258_bills_pc_screen },
	{ "TWB64 259 - Sonic Mega Blue", twb64_259_ebott_prolouge },
	{ "TWB64 260 - Fool's Gold and Silver", twb64_260_fools_gold_and_silver },
	{ "TWB64 261 - UTA VISION", twb64_261_uta_vision },
	{ "TWB64 262 - Metallic Paldea Brass", twb64_262_metallic_paldea_brass },
	{ "TWB64 263 - Classy Christmas", twb64_263_classy_christmas },
	{ "TWB64 264 - Winter Christmas", twb64_264_winter_christmas },
	{ "TWB64 265 - IDOL WORLD TRICOLOR!!!", twb64_265_idol_world_tricolor },
	{ "TWB64 266 - Inkling Tricolor", twb64_266_inkling_tricolor },
	{ "TWB64 267 - 7-Eleven Color Combo", twb64_267_7_eleven_color_combo },
	{ "TWB64 268 - PAC-PALETTE", twb64_268_pac_palette },
	{ "TWB64 269 - Vulnerable Blue", twb64_269_vulnerable_blue },
	{ "TWB64 270 - Nightvision Green", twb64_270_nightvision_green },
	{ "TWB64 271 - Bandai Namco Tricolor", twb64_271_bandai_namco_tricolor },
	{ "TWB64 272 - Gold, Silver, and Bronze", twb64_272_gold_silver_and_bronze },
	{ "TWB64 273 - Deku Vigilante Palette", twb64_273_deku_vigilante_palette },
	{ "TWB64 274 - Super Famicom Supreme", twb64_274_super_famicom_supreme },
	{ "TWB64 275 - Absorbent and Yellow", twb64_275_absorbent_and_yellow },
	{ "TWB64 276 - 765PRO TRICOLOR", twb64_276_765pro_tricolor },
	{ "TWB64 277 - GameCube Glimmer", twb64_277_gamecube_glimmer },
	{ "TWB64 278 - 1st Vision Pastel", twb64_278_1st_vision_pastel },
	{ "TWB64 279 - Perfect Majin Emperor", twb64_279_perfect_majin_emperor },
	{ "TWB64 280 - J-Pop Idol Sherbet", twb64_280_j_pop_idol_sherbet },
	{ "TWB64 281 - Ryuuguu Sunset", twb64_281_ryuuguu_sunset },
	{ "TWB64 282 - Tropical Starfall", twb64_282_tropical_starfall },
	{ "TWB64 283 - Colorful Horizons", twb64_283_colorful_horizons },
	{ "TWB64 284 - BLACKPINK BLINK PINK", twb64_284_blackpink_blink_pink },
	{ "TWB64 285 - DMG-SWITCH", twb64_285_dmg_switch },
	{ "TWB64 286 - POCKET SWITCH", twb64_286_pocket_switch },
	{ "TWB64 287 - Sunny Passion Paradise", twb64_287_sunny_passion_paradise },
	{ "TWB64 288 - Saiyan Beast Silver", twb64_288_saiyan_beast_silver },
	{ "TWB64 289 - RADIANT SMILE RAMP", twb64_289_radiant_smile_ramp },
	{ "TWB64 290 - A-RISE BLUE", twb64_290_a_rise_blue },
	{ "TWB64 291 - TROPICAL TWICE APRICOT", twb64_291_tropical_twice_apricot },
	{ "TWB64 292 - Odyssey Boy", twb64_292_odyssey_boy },
	{ "TWB64 293 - Frog Coin Green", twb64_293_frog_coin_green },
	{ "TWB64 294 - Garfield Vision", twb64_294_garfield_vision },
	{ "TWB64 295 - Bedrock Caveman Vision", twb64_295_bedrock_caveman_vision },
	{ "TWB64 296 - BANGTAN ARMY PURPLE", twb64_296_bangtan_army_purple },
	{ "TWB64 297 - LE SSERAFIM FEARLESS BLUE", twb64_297_le_sserafim_fearless_blue },
	{ "TWB64 298 - Baja Blast Beach", twb64_298_baja_blast_beach },
	{ "TWB64 299 - 3DS Virtual Console Green", twb64_299_3ds_virtual_console_green },
	{ "TWB64 300 - Wonder Purple", twb64_300_wonder_purple },
	{ "PixelShift 01 - Arctic Green", pixelshift_01_arctic_green },
	{ "PixelShift 02 - Arduboy", pixelshift_02_arduboy },
	{ "PixelShift 03 - BGB 0.3 Emulator", pixelshift_03_bgb_0_3_emulator },
	{ "PixelShift 04 - Camouflage", pixelshift_04_camouflage },
	{ "PixelShift 05 - Chocolate Bar", pixelshift_05_chocolate_bar },
	{ "PixelShift 06 - CMYK", pixelshift_06_cmyk },
	{ "PixelShift 07 - Cotton Candy", pixelshift_07_cotton_candy },
	{ "PixelShift 08 - Easy Greens", pixelshift_08_easy_greens },
	{ "PixelShift 09 - Gamate", pixelshift_09_gamate },
	{ "PixelShift 10 - Game Boy Light", pixelshift_10_game_boy_light },
	{ "PixelShift 11 - Game Boy Pocket", pixelshift_11_game_boy_pocket },
	{ "PixelShift 12 - Game Boy Pocket Alt", pixelshift_12_game_boy_pocket_alt },
	{ "PixelShift 13 - Game Pocket Computer", pixelshift_13_game_pocket_computer },
	{ "PixelShift 14 - Game & Watch Ball", pixelshift_14_game_and_watch_ball },
	{ "PixelShift 15 - GB Backlight Blue", pixelshift_15_gb_backlight_blue },
	{ "PixelShift 16 - GB Backlight Faded", pixelshift_16_gb_backlight_faded },
	{ "PixelShift 17 - GB Backlight Orange", pixelshift_17_gb_backlight_orange },
	{ "PixelShift 18 - GB Backlight White ", pixelshift_18_gb_backlight_white_ },
	{ "PixelShift 19 - GB Backlight Yellow Dark", pixelshift_19_gb_backlight_yellow_dark },
	{ "PixelShift 20 - GB Bootleg", pixelshift_20_gb_bootleg },
	{ "PixelShift 21 - GB Hunter", pixelshift_21_gb_hunter },
	{ "PixelShift 22 - GB Kiosk", pixelshift_22_gb_kiosk },
	{ "PixelShift 23 - GB Kiosk 2", pixelshift_23_gb_kiosk_2 },
	{ "PixelShift 24 - GB New", pixelshift_24_gb_new },
	{ "PixelShift 25 - GB Nuked", pixelshift_25_gb_nuked },
	{ "PixelShift 26 - GB Old", pixelshift_26_gb_old },
	{ "PixelShift 27 - GBP Bivert", pixelshift_27_gbp_bivert },
	{ "PixelShift 28 - GB Washed Yellow Backlight", pixelshift_28_gb_washed_yellow_backlight },
	{ "PixelShift 29 - Ghost", pixelshift_29_ghost },
	{ "PixelShift 30 - Glow In The Dark", pixelshift_30_glow_in_the_dark },
	{ "PixelShift 31 - Gold Bar", pixelshift_31_gold_bar },
	{ "PixelShift 32 - Grapefruit", pixelshift_32_grapefruit },
	{ "PixelShift 33 - Gray Green Mix", pixelshift_33_gray_green_mix },
	{ "PixelShift 34 - Missingno", pixelshift_34_missingno },
	{ "PixelShift 35 - MS-Dos", pixelshift_35_ms_dos },
	{ "PixelShift 36 - Newspaper", pixelshift_36_newspaper },
	{ "PixelShift 37 - Pip-Boy", pixelshift_37_pip_boy },
	{ "PixelShift 38 - Pocket Girl", pixelshift_38_pocket_girl },
	{ "PixelShift 39 - Silhouette", pixelshift_39_silhouette },
	{ "PixelShift 40 - Sunburst", pixelshift_40_sunburst },
	{ "PixelShift 41 - Technicolor", pixelshift_41_technicolor },
	{ "PixelShift 42 - Tron", pixelshift_42_tron },
	{ "PixelShift 43 - Vaporwave", pixelshift_43_vaporwave },
	{ "PixelShift 44 - Virtual Boy", pixelshift_44_virtual_boy },
	{ "PixelShift 45 - Wish", pixelshift_45_wish },
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
