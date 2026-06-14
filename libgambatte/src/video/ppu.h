//
//   Copyright (C) 2010 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef PPU_H
#define PPU_H

#include "lcddef.h"
#include "ly_counter.h"
#include "sprite_mapper.h"
#include "gbint.h"
#include "gambatte.h"
#include <cstddef>

namespace gambatte {

class PPUFrameBuf {
public:
	PPUFrameBuf() : buf_(0), fbline_(nullfbline()), pitch_(0) {}
	video_pixel_t * fb() const { return buf_; }
	video_pixel_t * fbline() const { return fbline_; }
	std::ptrdiff_t pitch() const { return pitch_; }
	void setBuf(video_pixel_t *buf, std::ptrdiff_t pitch) { buf_ = buf; pitch_ = pitch; fbline_ = nullfbline(); }
	void setFbline(unsigned ly) { fbline_ = buf_ ? buf_ + std::ptrdiff_t(ly) * pitch_ : nullfbline(); }

private:
	video_pixel_t *buf_;
	video_pixel_t *fbline_;
	std::ptrdiff_t pitch_;

	/* Shared 160-pixel scratch used as a destination when no real
	 * framebuf is attached. With DUAL_MODE compiled in, both PPU
	 * instances would write to this same buffer; the libretro
	 * core is single-threaded so there is no data race, but it is
	 * shared mutable state and nothing in the current code path
	 * actually reads it back. Make it a non-static member of
	 * PPUFrameBuf if DUAL_MODE is ever revived as a real feature. */
	static video_pixel_t * nullfbline() { static video_pixel_t nullfbline_[160]; return nullfbline_; }
};

struct PPUPriv;

struct PPUState {
	void (*f)(PPUPriv &v);
	unsigned (*predictCyclesUntilXpos_f)(PPUPriv const &v, int targetxpos, unsigned cycles);
	unsigned char id;
};

struct PPUPriv {
	video_pixel_t bgPalette[8 * 4];
	video_pixel_t spPalette[8 * 4];
	/* Precomputed BG palette expansion.
	 *
	 * Indexed by [palette_id][byte][lane]: each byte of ntileword
	 * packs four 2-bit palette indices (lanes 0..3 in byte order),
	 * so each bgPaletteExpanded[p][byte] holds the four resolved
	 * colours for that nibble of the tile word.  Both the DMG and
	 * CGB renderers issue two 16-byte loads + two 16-byte stores
	 * per 8-pixel tile instead of eight dependent scalar loads from
	 * bgPalette.
	 *
	 * DMG mode uses only palette 0.  CGB mode picks palette
	 * (nattrib & 7) per tile.  The table is rebuilt only when a BG
	 * palette changes: BGP / refresh / doCgbBgColorChange paths.
	 *
	 * Size: 8 * 256 * 4 * sizeof(video_pixel_t).  For the default
	 * libretro u32 build that's 32 KiB; for the VIDEO_RGB565 /
	 * VIDEO_ABGR1555 u16 builds it's 16 KiB.  Modern x86 L1d is
	 * 32-48 KiB and Cortex-A7/A53/A72 L1d is 32 KiB, so the worst
	 * case fits.  In practice each frame uses only a few palettes
	 * actively, so the hot working set is much smaller than the
	 * full table. */
	video_pixel_t bgPaletteExpanded[8][256][4];
	struct Sprite { unsigned char spx, oampos, line, attrib; } spriteList[11];
	unsigned short spwordList[11];
	unsigned char nextSprite;
	unsigned char currentSprite;

	unsigned char const *vram;
	PPUState const *nextCallPtr;

	unsigned long now;
	unsigned long lastM0Time;
	long cycles;

	unsigned tileword;
	unsigned ntileword;

	SpriteMapper spriteMapper;
	LyCounter lyCounter;
	PPUFrameBuf framebuf;

	unsigned char lcdc;
	unsigned char scy;
	unsigned char scx;
	unsigned char wy;
	unsigned char wy2;
	unsigned char wx;
	unsigned char winDrawState;
	unsigned char wscx;
	unsigned char winYPos;
	unsigned char reg0;
	unsigned char reg1;
	unsigned char attrib;
	unsigned char nattrib;
	unsigned char xpos;
	unsigned char endx;

	bool cgb;
   bool dmgMode;
	bool weMaster;

	PPUPriv(NextM0Time &nextM0Time, unsigned char const *oamram, unsigned char const *vram);
};

class PPU {
public:
	PPU(NextM0Time &nextM0Time, unsigned char const *oamram, unsigned char const *vram)
	: p_(nextM0Time, oamram, vram)
	{
	}

	video_pixel_t * bgPalette() { return p_.bgPalette; }

	/* Recompute bgPaletteExpanded[palette_id] from the current
	 * bgPalette[palette_id*4 .. palette_id*4+3].  DMG mode uses
	 * palette 0 only.  Called by the LCD on every BG palette write
	 * (BGP register, CGB BCPD register, refresh / savestate paths).
	 * Idempotent and side-effect-free. */
	void refreshBgPaletteExpansion(unsigned const palette_id)
	{
		video_pixel_t const *const pal = p_.bgPalette + palette_id * 4;
		video_pixel_t const c0 = pal[0];
		video_pixel_t const c1 = pal[1];
		video_pixel_t const c2 = pal[2];
		video_pixel_t const c3 = pal[3];
		video_pixel_t const cs[4] = { c0, c1, c2, c3 };
		for (int b = 0; b < 256; ++b) {
			p_.bgPaletteExpanded[palette_id][b][0] = cs[ b       & 3];
			p_.bgPaletteExpanded[palette_id][b][1] = cs[(b >> 2) & 3];
			p_.bgPaletteExpanded[palette_id][b][2] = cs[(b >> 4) & 3];
			p_.bgPaletteExpanded[palette_id][b][3] = cs[ b >> 6      ];
		}
	}
	bool cgb() const { return p_.cgb; }
   void setDmgMode(bool mode) { p_.dmgMode = mode; }
   bool inDmgMode() const { return p_.dmgMode; }
	void doLyCountEvent() { p_.lyCounter.doEvent(); }
	unsigned long doSpriteMapEvent(unsigned long time) { return p_.spriteMapper.doEvent(time); }
	PPUFrameBuf const & frameBuf() const { return p_.framebuf; }
   
	bool inactivePeriodAfterDisplayEnable(unsigned long cc) const {
		return p_.spriteMapper.inactivePeriodAfterDisplayEnable(cc);
	}

	unsigned long lastM0Time() const { return p_.lastM0Time; }
	unsigned lcdc() const { return p_.lcdc; }
	void loadState(SaveState const &state, unsigned char const *oamram);
	LyCounter const & lyCounter() const { return p_.lyCounter; }
	unsigned long now() const { return p_.now; }
	void oamChange(unsigned long cc) { p_.spriteMapper.oamChange(cc); }
	void oamChange(unsigned char const *oamram, unsigned long cc) { p_.spriteMapper.oamChange(oamram, cc); }
	unsigned long predictedNextXposTime(unsigned xpos) const;
	void reset(unsigned char const *oamram, unsigned char const *vram, bool cgb);
	void resetCc(unsigned long oldCc, unsigned long newCc);
	void saveState(SaveState &ss) const;
	void setFrameBuf(video_pixel_t *buf, std::ptrdiff_t pitch) { p_.framebuf.setBuf(buf, pitch); }
	void setLcdc(unsigned lcdc, unsigned long cc);
	void setScx(unsigned scx) { p_.scx = scx; }
	void setScy(unsigned scy) { p_.scy = scy; }
	void setStatePtrs(SaveState &ss) { p_.spriteMapper.setStatePtrs(ss); }
	void setWx(unsigned wx) { p_.wx = wx; }
	void setWy(unsigned wy) { p_.wy = wy; }
	void updateWy2() { p_.wy2 = p_.wy; }
	void speedChange(unsigned long cycleCounter);
	video_pixel_t * spPalette() { return p_.spPalette; }
	void update(unsigned long cc);

private:
	PPUPriv p_;
};

}

#endif
