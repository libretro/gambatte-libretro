/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#ifndef CPU_H
#define CPU_H

#include "gambatte-memory.h"

namespace gambatte {

class CPU {
	Memory memory;
	
	unsigned long cycleCounter_;

	unsigned short PC_;
	unsigned short SP;
	
	unsigned HF1, HF2, ZF, CF;

	unsigned char A_, B, C, D, E, /*F,*/ H, L;

	bool skip;
	
	void process(unsigned long cycles);
	
public:
	
	CPU();
// 	void halt();

// 	unsigned interrupt(unsigned address, unsigned cycleCounter);
	
	long runFor(unsigned long cycles);
	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state);
	void loadState(const SaveState &state);
	
   void *savedata_ptr() { return memory.savedata_ptr(); }
   unsigned savedata_size() { return memory.savedata_size(); }
   void *rtcdata_ptr() { return memory.rtcdata_ptr(); }
   unsigned rtcdata_size() { return memory.rtcdata_size(); }
	
	void setVideoBuffer(video_pixel_t *const videoBuf, const int pitch) {
		memory.setVideoBuffer(videoBuf, pitch);
	}
	
	void setInputGetter(InputGetter *getInput) {
		memory.setInputGetter(getInput);
	}
	
	void setSaveDir(const std::string &sdir) {
		memory.setSaveDir(sdir);
	}
	
   bool load(const std::string &romfile, bool forceDmg, bool multicartCompat) {
      return memory.loadROM(romfile, forceDmg, multicartCompat);
   }

   bool load(const void *romfile, unsigned romsize, bool forceDmg, bool multicartCompat) {
      return memory.loadROM(romfile, romsize, forceDmg, multicartCompat);
   }
	
	void setSoundBuffer(uint_least32_t *const buf) { memory.setSoundBuffer(buf); }
	unsigned fillSoundBuffer() { return memory.fillSoundBuffer(cycleCounter_); }
	
	bool isCgb() const { return memory.isCgb(); }
	
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) {
		memory.setDmgPaletteColor(palNum, colorNum, rgb32);
	}

   void setGameGenie(const std::string &codes) { memory.setGameGenie(codes); }
   void setGameShark(const std::string &codes) { memory.setGameShark(codes); }
   void clearCheats() { memory.clearCheats(); }
	
	void display_setColorCorrection(bool enable) { memory.display_setColorCorrection(enable); }
	video_pixel_t display_gbcToRgb32(const unsigned bgr15) { return memory.display_gbcToRgb32(bgr15); }
};

}

#endif
