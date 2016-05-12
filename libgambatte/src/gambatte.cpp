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
#include "gambatte.h"
#include "cpu.h"
#include "savestate.h"
#include "statesaver.h"
#include "initstate.h"
#include <sstream>

namespace gambatte {
struct GB::Priv {
	CPU cpu;
	int stateNo;
	bool gbaCgbMode;
	
	Priv() : stateNo(1), gbaCgbMode(false) {}

	void on_load_succeeded(unsigned flags);
};
	
GB::GB() : p_(new Priv) {}

GB::~GB() {
	delete p_;
}

long GB::runFor(gambatte::video_pixel_t *const videoBuf, const int pitch,
			gambatte::uint_least32_t *const soundBuf, unsigned &samples) {
	
	p_->cpu.setVideoBuffer(videoBuf, pitch);
	p_->cpu.setSoundBuffer(soundBuf);
	const long cyclesSinceBlit = p_->cpu.runFor(samples * 2);
	samples = p_->cpu.fillSoundBuffer();
	
	return cyclesSinceBlit < 0 ? cyclesSinceBlit : static_cast<long>(samples) - (cyclesSinceBlit >> 1);
}

void GB::reset() {
   SaveState state;
   p_->cpu.setStatePtrs(state);
   setInitState(state, p_->cpu.isCgb(), p_->gbaCgbMode);
   p_->cpu.loadState(state);
}

void GB::setInputGetter(InputGetter *getInput) {
	p_->cpu.setInputGetter(getInput);
}

void GB::Priv::on_load_succeeded(unsigned flags)
{
	SaveState state;
	cpu.setStatePtrs(state);
	setInitState(state, cpu.isCgb(), gbaCgbMode = flags & GBA_CGB);
	cpu.loadState(state);

	stateNo = 1;
}

void *GB::savedata_ptr() { return p_->cpu.savedata_ptr(); }
unsigned GB::savedata_size() { return p_->cpu.savedata_size(); }
void *GB::rtcdata_ptr() { return p_->cpu.rtcdata_ptr(); }
unsigned GB::rtcdata_size() { return p_->cpu.rtcdata_size(); }

int GB::load(const void *romdata, unsigned romsize, const unsigned flags) {
	const int failed = p_->cpu.load(romdata, romsize, flags & FORCE_DMG, flags & MULTICART_COMPAT);
	
	if (!failed)
		p_->on_load_succeeded(flags);
	
	return failed;
}

bool GB::isCgb() const {
	return p_->cpu.isCgb();
}

bool GB::isLoaded() const {
	return true;
}

void GB::setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) {
	p_->cpu.setDmgPaletteColor(palNum, colorNum, rgb32);
}

void GB::loadState(const void *data) {
   SaveState state;
   p_->cpu.setStatePtrs(state);

   if (StateSaver::loadState(state, data)) {
      p_->cpu.loadState(state);
   }
}

void GB::saveState(void *data) {
   SaveState state;
   p_->cpu.setStatePtrs(state);
   p_->cpu.saveState(state);
   StateSaver::saveState(state, data);
}

size_t GB::stateSize() const {
   SaveState state;
   p_->cpu.setStatePtrs(state);
   p_->cpu.saveState(state);
   return StateSaver::stateSize(state);
}

void GB::setColorCorrection(bool enable) {
   p_->cpu.mem_.display_setColorCorrection(enable);
}

video_pixel_t GB::gbcToRgb32(const unsigned bgr15) {
   return p_->cpu.mem_.display_gbcToRgb32(bgr15);
}


void GB::setGameGenie(const std::string &codes) {
 p_->cpu.setGameGenie(codes);
}

void GB::setGameShark(const std::string &codes) {
 p_->cpu.setGameShark(codes);
}

void GB::clearCheats() {
 p_->cpu.clearCheats();
}

#ifdef __LIBRETRO__
void *GB::vram_ptr() const {
 return p_->cpu.vram_ptr();
}

void *GB::rambank0_ptr() const {
 return p_->cpu.rambank0_ptr();
}

void *GB::rambank1_ptr() const {
 return p_->cpu.rambank1_ptr();
}

void *GB::rombank0_ptr() const {
 return p_->cpu.rombank0_ptr();
}

void *GB::rombank1_ptr() const {
 return p_->cpu.rombank1_ptr();
}
#endif

}

