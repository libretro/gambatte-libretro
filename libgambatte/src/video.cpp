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
#include "video.h"
#include "savestate.h"
#include <cstring>
#include <algorithm>

namespace gambatte
{
   void LCD::setDmgPalette(video_pixel_t *const palette, const video_pixel_t *const dmgColors, const unsigned data)
   {
      palette[0] = dmgColors[data      & 3];
      palette[1] = dmgColors[data >> 2 & 3];
      palette[2] = dmgColors[data >> 4 & 3];
      palette[3] = dmgColors[data >> 6 & 3];
   }


   void LCD::setColorCorrection(bool colorCorrection_)
   {
      colorCorrection=colorCorrection_;
      refreshPalettes();
   }

LCD::LCD(const unsigned char *const oamram, const unsigned char *const vram, const VideoInterruptRequester memEventRequester) :
   ppu_(nextM0Time_, oamram, vram),
   eventTimes_(memEventRequester),
   statReg_(0),
   m2IrqStatReg_(0),
   m1IrqStatReg_(0)
{
   std::memset( bgpData_, 0, sizeof  bgpData_);
   std::memset(objpData_, 0, sizeof objpData_);

   for (std::size_t i = 0; i < sizeof(dmgColorsRgb32_) / sizeof(dmgColorsRgb32_[0]); ++i)
   {
#ifdef VIDEO_RGB565
      uint16_t dmgColors[4]={0xFFFF, //11111 111111 11111
         0xAD55, //10101 101010 10101
         0x52AA, //01010 010101 01010
         0x0000};//00000 000000 00000
      setDmgPaletteColor(i, dmgColors[i&3]);
#else
      setDmgPaletteColor(i, (3 - (i & 3)) * 85 * 0x010101);
#endif
   }

   reset(oamram, false);
   setVideoBuffer(0, 160);

   setColorCorrection(true);
}

void LCD::reset(const unsigned char *const oamram, const bool cgb)
{
   ppu_.reset(oamram, cgb);
   lycIrq_.setCgb(cgb);
   refreshPalettes();
}

static unsigned long mode2IrqSchedule(const unsigned statReg, const LyCounter &lyCounter, const unsigned long cycleCounter)
{
   if (!(statReg & 0x20))
      return disabled_time;

   unsigned next = lyCounter.time() - cycleCounter;

   if (lyCounter.ly() >= 143 || (lyCounter.ly() == 142 && next <= 4) || (statReg & 0x08))
      next += (153u - lyCounter.ly()) * lyCounter.lineTime();
   else
   {
      if (next <= 4)
         next += lyCounter.lineTime();

      next -= 4;
   }

   return cycleCounter + next;
}

static inline unsigned long m0IrqTimeFromXpos166Time(
      const unsigned long xpos166Time, const bool cgb, const bool ds)
{
   return xpos166Time + cgb - ds;
}

static inline unsigned long hdmaTimeFromM0Time(
      const unsigned long m0Time, const bool ds)
{
   return m0Time + 1 - ds;
}

static unsigned long nextHdmaTime(const unsigned long lastM0Time,
      const unsigned long nextM0Time, const unsigned long cycleCounter, const bool ds)
{
   return cycleCounter < hdmaTimeFromM0Time(lastM0Time, ds)
      ? hdmaTimeFromM0Time(lastM0Time, ds)
      : hdmaTimeFromM0Time(nextM0Time, ds);
}

void LCD::setStatePtrs(SaveState &state)
{
   state.ppu.bgpData.set(  bgpData_, sizeof  bgpData_);
   state.ppu.objpData.set(objpData_, sizeof objpData_);
   ppu_.setStatePtrs(state);
}

void LCD::saveState(SaveState &state) const
{
   state.mem.hdmaTransfer = hdmaIsEnabled();
   state.ppu.nextM0Irq = eventTimes_(MODE0_IRQ) - ppu_.now();
   state.ppu.pendingLcdstatIrq = eventTimes_(ONESHOT_LCDSTATIRQ) != disabled_time;

   lycIrq_.saveState(state);
   m0Irq_.saveState(state);
   ppu_.saveState(state);
}

void LCD::loadState(const SaveState &state, const unsigned char *const oamram)
{
   statReg_ = state.mem.ioamhram.get()[0x141];
   m2IrqStatReg_ = statReg_;
   m1IrqStatReg_ = statReg_;

   ppu_.loadState(state, oamram);
   lycIrq_.loadState(state);
   m0Irq_.loadState(state);

   if (ppu_.lcdc() & 0x80)
   {
      nextM0Time_.predictNextM0Time(ppu_);
      lycIrq_.reschedule(ppu_.lyCounter(), ppu_.now());

      eventTimes_.setm<ONESHOT_LCDSTATIRQ>(state.ppu.pendingLcdstatIrq
            ? ppu_.now() + 1 : static_cast<unsigned long>(disabled_time));
      eventTimes_.setm<ONESHOT_UPDATEWY2>(state.ppu.oldWy != state.mem.ioamhram.get()[0x14A]
            ? ppu_.now() + 1 : static_cast<unsigned long>(disabled_time));
      eventTimes_.set<LY_COUNT>(ppu_.lyCounter().time());
      eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), ppu_.now()));
      eventTimes_.setm<LYC_IRQ>(lycIrq_.time());
      eventTimes_.setm<MODE1_IRQ>(ppu_.lyCounter().nextFrameCycle(144 * 456, ppu_.now()));
      eventTimes_.setm<MODE2_IRQ>(mode2IrqSchedule(statReg_, ppu_.lyCounter(), ppu_.now()));
      eventTimes_.setm<MODE0_IRQ>((statReg_ & 0x08) ? ppu_.now() + state.ppu.nextM0Irq : static_cast<unsigned long>(disabled_time));
      eventTimes_.setm<HDMA_REQ>(state.mem.hdmaTransfer
            ? nextHdmaTime(ppu_.lastM0Time(), nextM0Time_.predictedNextM0Time(), ppu_.now(), isDoubleSpeed())
            : static_cast<unsigned long>(disabled_time));
   }
   else
   {
      for (int i = 0; i < NUM_MEM_EVENTS; ++i)
         eventTimes_.set(static_cast<MemEvent>(i), disabled_time);
   }

   refreshPalettes();
}

void LCD::refreshPalettes()
{
   if (ppu_.cgb())
   {
      for (unsigned i = 0; i < 8 * 8; i += 2)
      {
         ppu_.bgPalette()[i >> 1] = gbcToRgb32( bgpData_[i] |  bgpData_[i + 1] << 8);
         ppu_.spPalette()[i >> 1] = gbcToRgb32(objpData_[i] | objpData_[i + 1] << 8);
      }
   }
   else
   {
      setDmgPalette(ppu_.bgPalette()    , dmgColorsRgb32_    ,  bgpData_[0]);
      setDmgPalette(ppu_.spPalette()    , dmgColorsRgb32_ + 4, objpData_[0]);
      setDmgPalette(ppu_.spPalette() + 4, dmgColorsRgb32_ + 8, objpData_[1]);
   }
}

namespace {
   template<typename T>
      static void clear(T *buf, const unsigned long color, const int dpitch)
      {
         unsigned lines = 144;

         while (lines--)
         {
            std::fill_n(buf, 160, color);
            buf += dpitch;
         }
      }

}

void LCD::updateScreen(const bool blanklcd, const unsigned long cycleCounter)
{
   update(cycleCounter);

   if (blanklcd && ppu_.frameBuf().fb())
   {
      const video_pixel_t color = ppu_.cgb() ? gbcToRgb32(0xFFFF) : dmgColorsRgb32_[0];
      clear(ppu_.frameBuf().fb(), color, ppu_.frameBuf().pitch());
   }
}

void LCD::resetCc(const unsigned long oldCc, const unsigned long newCc)
{
   update(oldCc);
   ppu_.resetCc(oldCc, newCc);

   if (ppu_.lcdc() & 0x80)
   {
      const unsigned long dec = oldCc - newCc;

      nextM0Time_.invalidatePredictedNextM0Time();
      lycIrq_.reschedule(ppu_.lyCounter(), newCc);

      for (int i = 0; i < NUM_MEM_EVENTS; ++i)
      {
         if (eventTimes_(static_cast<MemEvent>(i)) != disabled_time)
            eventTimes_.set(static_cast<MemEvent>(i), eventTimes_(static_cast<MemEvent>(i)) - dec);
      }

      eventTimes_.set<LY_COUNT>(ppu_.lyCounter().time());
   }
}

void LCD::speedChange(const unsigned long cycleCounter)
{
   update(cycleCounter);
   ppu_.speedChange(cycleCounter);

   if (ppu_.lcdc() & 0x80)
   {
      nextM0Time_.predictNextM0Time(ppu_);
      lycIrq_.reschedule(ppu_.lyCounter(), cycleCounter);

      eventTimes_.set<LY_COUNT>(ppu_.lyCounter().time());
      eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));
      eventTimes_.setm<LYC_IRQ>(lycIrq_.time());
      eventTimes_.setm<MODE1_IRQ>(ppu_.lyCounter().nextFrameCycle(144 * 456, cycleCounter));
      eventTimes_.setm<MODE2_IRQ>(mode2IrqSchedule(statReg_, ppu_.lyCounter(), cycleCounter));

      if (eventTimes_(MODE0_IRQ) != disabled_time && eventTimes_(MODE0_IRQ) - cycleCounter > 1)
         eventTimes_.setm<MODE0_IRQ>(m0IrqTimeFromXpos166Time(ppu_.predictedNextXposTime(166), ppu_.cgb(), isDoubleSpeed()));

      if (hdmaIsEnabled() && eventTimes_(HDMA_REQ) - cycleCounter > 1)
      {
         eventTimes_.setm<HDMA_REQ>(nextHdmaTime(ppu_.lastM0Time(),
                  nextM0Time_.predictedNextM0Time(), cycleCounter, isDoubleSpeed()));
      }
   }
}

static inline unsigned long m0TimeOfCurrentLine(const unsigned long nextLyTime,
      const unsigned long lastM0Time, const unsigned long nextM0Time)
{
   return nextM0Time < nextLyTime ? nextM0Time : lastM0Time;
}

unsigned long LCD::m0TimeOfCurrentLine(const unsigned long cc)
{
   if (cc >= nextM0Time_.predictedNextM0Time())
   {
      update(cc);
      nextM0Time_.predictNextM0Time(ppu_);
   }

   return gambatte::m0TimeOfCurrentLine(ppu_.lyCounter().time(), ppu_.lastM0Time(), nextM0Time_.predictedNextM0Time());
}

static bool isHdmaPeriod(const LyCounter &lyCounter,
      const unsigned long m0TimeOfCurrentLy, const unsigned long cycleCounter)
{
   const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

   return /*(ppu_.lcdc & 0x80) && */lyCounter.ly() < 144 && timeToNextLy > 4
      && cycleCounter >= hdmaTimeFromM0Time(m0TimeOfCurrentLy, lyCounter.isDoubleSpeed());
}

void LCD::enableHdma(const unsigned long cycleCounter)
{
   if (cycleCounter >= nextM0Time_.predictedNextM0Time())
   {
      update(cycleCounter);
      nextM0Time_.predictNextM0Time(ppu_);
   }
   else if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   if (isHdmaPeriod(ppu_.lyCounter(),
            gambatte::m0TimeOfCurrentLine(ppu_.lyCounter().time(),
               ppu_.lastM0Time(), nextM0Time_.predictedNextM0Time()), cycleCounter))
      eventTimes_.flagHdmaReq();

   eventTimes_.setm<HDMA_REQ>(nextHdmaTime(ppu_.lastM0Time(), nextM0Time_.predictedNextM0Time(), cycleCounter, isDoubleSpeed()));
}

void LCD::disableHdma(const unsigned long cycleCounter)
{
   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   eventTimes_.setm<HDMA_REQ>(disabled_time);
}

bool LCD::vramAccessible(const unsigned long cycleCounter)
{
   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   return !(ppu_.lcdc() & 0x80) || ppu_.lyCounter().ly() >= 144
      || ppu_.lyCounter().lineCycles(cycleCounter) < 80U
      || cycleCounter + isDoubleSpeed() - ppu_.cgb() + 2 >= m0TimeOfCurrentLine(cycleCounter);
}

bool LCD::cgbpAccessible(const unsigned long cycleCounter)
{
   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   return !(ppu_.lcdc() & 0x80) || ppu_.lyCounter().ly() >= 144
      || ppu_.lyCounter().lineCycles(cycleCounter) < 80U + isDoubleSpeed()
      || cycleCounter >= m0TimeOfCurrentLine(cycleCounter) + 3 - isDoubleSpeed();
}

void LCD::doCgbColorChange(unsigned char *const pdata,
      video_pixel_t *const palette, unsigned index, const unsigned data)
{
   pdata[index] = data;
   index >>= 1;
   palette[index] = gbcToRgb32(pdata[index << 1] | pdata[(index << 1) + 1] << 8);
}

void LCD::doCgbBgColorChange(unsigned index, const unsigned data, const unsigned long cycleCounter)
{
   if (cgbpAccessible(cycleCounter))
   {
      update(cycleCounter);
      doCgbColorChange(bgpData_, ppu_.bgPalette(), index, data);
   }
}

void LCD::doCgbSpColorChange(unsigned index, const unsigned data, const unsigned long cycleCounter)
{
   if (cgbpAccessible(cycleCounter))
   {
      update(cycleCounter);
      doCgbColorChange(objpData_, ppu_.spPalette(), index, data);
   }
}

bool LCD::oamReadable(const unsigned long cycleCounter)
{
   if (!(ppu_.lcdc() & 0x80) || ppu_.inactivePeriodAfterDisplayEnable(cycleCounter))
      return true;

   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   if (ppu_.lyCounter().lineCycles(cycleCounter) + 4 - ppu_.lyCounter().isDoubleSpeed() * 3u >= 456)
      return ppu_.lyCounter().ly() >= 144-1 && ppu_.lyCounter().ly() != 153;

   return ppu_.lyCounter().ly() >= 144 || cycleCounter + isDoubleSpeed() - ppu_.cgb() + 2 >= m0TimeOfCurrentLine(cycleCounter);
}

bool LCD::oamWritable(const unsigned long cycleCounter)
{
   if (!(ppu_.lcdc() & 0x80) || ppu_.inactivePeriodAfterDisplayEnable(cycleCounter))
      return true;

   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   if (ppu_.lyCounter().lineCycles(cycleCounter) + 3 + ppu_.cgb() - ppu_.lyCounter().isDoubleSpeed() * 2u >= 456)
      return ppu_.lyCounter().ly() >= 144-1 && ppu_.lyCounter().ly() != 153;

   return ppu_.lyCounter().ly() >= 144 || cycleCounter + isDoubleSpeed() - ppu_.cgb() + 2 >= m0TimeOfCurrentLine(cycleCounter);
}

void LCD::mode3CyclesChange()
{
   nextM0Time_.invalidatePredictedNextM0Time();

   if (eventTimes_(MODE0_IRQ) != disabled_time
         && eventTimes_(MODE0_IRQ) > m0IrqTimeFromXpos166Time(ppu_.now(), ppu_.cgb(), isDoubleSpeed())) {
      eventTimes_.setm<MODE0_IRQ>(m0IrqTimeFromXpos166Time(ppu_.predictedNextXposTime(166), ppu_.cgb(), isDoubleSpeed()));
   }

   if (eventTimes_(HDMA_REQ) != disabled_time
         && eventTimes_(HDMA_REQ) > hdmaTimeFromM0Time(ppu_.lastM0Time(), isDoubleSpeed())) {
      nextM0Time_.predictNextM0Time(ppu_);
      eventTimes_.setm<HDMA_REQ>(hdmaTimeFromM0Time(nextM0Time_.predictedNextM0Time(), isDoubleSpeed()));
   }
}

void LCD::wxChange(const unsigned newValue, const unsigned long cycleCounter)
{
   update(cycleCounter + isDoubleSpeed() + 1);
   ppu_.setWx(newValue);
   mode3CyclesChange();
}

void LCD::wyChange(const unsigned newValue, const unsigned long cycleCounter)
{
   update(cycleCounter + 1);
   ppu_.setWy(newValue);
   // 	mode3CyclesChange(); // should be safe to wait until after wy2 delay, because no mode3 events are close to when wy1 is read.

   // wy2 is a delayed version of wy. really just slowness of ly == wy comparison.
   if (ppu_.cgb() && (ppu_.lcdc() & 0x80)) {
      eventTimes_.setm<ONESHOT_UPDATEWY2>(cycleCounter + 5);
   } else {
      update(cycleCounter + 2);
      ppu_.updateWy2();
      mode3CyclesChange();
   }
}

void LCD::scxChange(const unsigned newScx, const unsigned long cycleCounter) {
   update(cycleCounter + ppu_.cgb() + isDoubleSpeed());
   ppu_.setScx(newScx);
   mode3CyclesChange();
}

void LCD::scyChange(const unsigned newValue, const unsigned long cycleCounter) {
   update(cycleCounter + ppu_.cgb() + isDoubleSpeed());
   ppu_.setScy(newValue);
}

void LCD::oamChange(const unsigned long cycleCounter) {
   if (ppu_.lcdc() & 0x80) {
      update(cycleCounter);
      ppu_.oamChange(cycleCounter);
      eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));
   }
}

void LCD::oamChange(const unsigned char *const oamram, const unsigned long cycleCounter) {
   update(cycleCounter);
   ppu_.oamChange(oamram, cycleCounter);

   if (ppu_.lcdc() & 0x80)
      eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));
}

void LCD::lcdcChange(const unsigned data, const unsigned long cycleCounter) {
   const unsigned oldLcdc = ppu_.lcdc();
   update(cycleCounter);

   if ((oldLcdc ^ data) & 0x80)
   {
      ppu_.setLcdc(data, cycleCounter);

      if (data & 0x80) {
         lycIrq_.lcdReset();
         m0Irq_.lcdReset(statReg_, lycIrq_.lycReg());

         if (lycIrq_.lycReg() == 0 && (statReg_ & 0x40))
            eventTimes_.flagIrq(2);

         nextM0Time_.predictNextM0Time(ppu_);
         lycIrq_.reschedule(ppu_.lyCounter(), cycleCounter);

         eventTimes_.set<LY_COUNT>(ppu_.lyCounter().time());
         eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));
         eventTimes_.setm<LYC_IRQ>(lycIrq_.time());
         eventTimes_.setm<MODE1_IRQ>(ppu_.lyCounter().nextFrameCycle(144 * 456, cycleCounter));
         eventTimes_.setm<MODE2_IRQ>(mode2IrqSchedule(statReg_, ppu_.lyCounter(), cycleCounter));

         if (statReg_ & 0x08)
            eventTimes_.setm<MODE0_IRQ>(m0IrqTimeFromXpos166Time(ppu_.predictedNextXposTime(166), ppu_.cgb(), isDoubleSpeed()));

         if (hdmaIsEnabled())
            eventTimes_.setm<HDMA_REQ>(nextHdmaTime(ppu_.lastM0Time(),
                     nextM0Time_.predictedNextM0Time(), cycleCounter, isDoubleSpeed()));
      }
      else
      {
         for (int i = 0; i < NUM_MEM_EVENTS; ++i)
            eventTimes_.set(static_cast<MemEvent>(i), disabled_time);
      }
   }
   else if (data & 0x80)
   {
      if (ppu_.cgb())
      {
         ppu_.setLcdc((oldLcdc & ~0x14) | (data & 0x14), cycleCounter);

         if ((oldLcdc ^ data) & 0x04)
            eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));

         update(cycleCounter + isDoubleSpeed() + 1);
         ppu_.setLcdc(data, cycleCounter + isDoubleSpeed() + 1);

         if ((oldLcdc ^ data) & 0x20)
            mode3CyclesChange();
      }
      else
      {
         ppu_.setLcdc(data, cycleCounter);

         if ((oldLcdc ^ data) & 0x04)
            eventTimes_.setm<SPRITE_MAP>(SpriteMapper::schedule(ppu_.lyCounter(), cycleCounter));

         if ((oldLcdc ^ data) & 0x22)
            mode3CyclesChange();
      }
   }
   else
      ppu_.setLcdc(data, cycleCounter);
}

void LCD::lcdstatChange(const unsigned data, const unsigned long cycleCounter)
{
   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   const unsigned old = statReg_;
   statReg_ = data;
   lycIrq_.statRegChange(data, ppu_.lyCounter(), cycleCounter);

   if (ppu_.lcdc() & 0x80)
   {
      if (!ppu_.cgb())
      {
         const unsigned timeToNextLy = ppu_.lyCounter().time() - cycleCounter;
         const unsigned lycCmpLy = ppu_.lyCounter().ly() == 153 && timeToNextLy <= 444U + 8 ? 0 : ppu_.lyCounter().ly();

         if (lycCmpLy == lycIrq_.lycReg())
         {
            if (!(old & 0x40) && (ppu_.lyCounter().ly() < 144 || !(old & 0x10)))
               eventTimes_.flagIrq(2);
         }
         else if (ppu_.lyCounter().ly() >= 144)
         {
            if (!(old & 0x10))
               eventTimes_.flagIrq(2);
         }
         else if (cycleCounter + 1 >= m0TimeOfCurrentLine(cycleCounter))
         {
            if (!(old & 0x08))
               eventTimes_.flagIrq(2);
         }
      }
      else
      {
         const unsigned timeToNextLy = ppu_.lyCounter().time() - cycleCounter;
         const unsigned lycCmpLy = ppu_.lyCounter().ly() == 153 && timeToNextLy <= (444U << isDoubleSpeed()) + 8
            ? 0 : ppu_.lyCounter().ly();
         const bool lycperiod = lycCmpLy == lycIrq_.lycReg() && timeToNextLy > 4U - isDoubleSpeed() * 4U;
         const bool lycperiodActive = lycperiod && (old & 0x40);

         if (lycperiod && (data & 0x40) - (old & 0x40) == 0x40 && (ppu_.lyCounter().ly() < 144 || !(old & 0x10)))
            eventTimes_.flagIrq(2);
         else if (ppu_.lyCounter().ly() >= 144 && (data & 0x10) - (old & 0x10) == 0x10 &&
               (ppu_.lyCounter().ly() < 153 || timeToNextLy > 4U - isDoubleSpeed() * 4U) && !lycperiodActive)
            eventTimes_.flagIrq(2);
         else if ((data & 0x08) - (old & 0x08) == 0x08 && ppu_.lyCounter().ly() < 144 && !lycperiodActive &&
               timeToNextLy > 4 && cycleCounter + isDoubleSpeed() * 2 >= m0TimeOfCurrentLine(cycleCounter))
            eventTimes_.flagIrq(2);
         else if ((data & 0x28) == 0x20 && !(old & 0x20)
               && ((timeToNextLy <= 4 && ppu_.lyCounter().ly() < 143)
                  || (timeToNextLy == 456*2 && ppu_.lyCounter().ly() < 144)))
            eventTimes_.flagIrq(2);
      }

      if ((data & 0x08) && eventTimes_(MODE0_IRQ) == disabled_time)
      {
         update(cycleCounter);
         eventTimes_.setm<MODE0_IRQ>(m0IrqTimeFromXpos166Time(ppu_.predictedNextXposTime(166), ppu_.cgb(), isDoubleSpeed()));
      }

      eventTimes_.setm<MODE2_IRQ>(mode2IrqSchedule(data, ppu_.lyCounter(), cycleCounter));
      eventTimes_.setm<LYC_IRQ>(lycIrq_.time());
   }

   m2IrqStatReg_ = eventTimes_(MODE2_IRQ) - cycleCounter > (ppu_.cgb() - isDoubleSpeed()) * 4U
      ? data : (m2IrqStatReg_ & 0x10) | (statReg_ & ~0x10);
   m1IrqStatReg_ = eventTimes_(MODE1_IRQ) - cycleCounter > (ppu_.cgb() - isDoubleSpeed()) * 4U
      ? data : (m1IrqStatReg_ & 0x08) | (statReg_ & ~0x08);

   m0Irq_.statRegChange(data, eventTimes_(MODE0_IRQ), cycleCounter, ppu_.cgb());
}

void LCD::lycRegChange(const unsigned data, const unsigned long cycleCounter)
{
   if (data == lycIrq_.lycReg())
      return;

   if (cycleCounter >= eventTimes_.nextEventTime())
      update(cycleCounter);

   m0Irq_.lycRegChange(data, eventTimes_(MODE0_IRQ), cycleCounter, isDoubleSpeed(), ppu_.cgb());	
   lycIrq_.lycRegChange(data, ppu_.lyCounter(), cycleCounter);

   if (!(ppu_.lcdc() & 0x80))
      return;

   eventTimes_.setm<LYC_IRQ>(lycIrq_.time());

   const unsigned timeToNextLy = ppu_.lyCounter().time() - cycleCounter;

   if ((statReg_ & 0x40) && data < 154 &&
         (ppu_.lyCounter().ly() < 144 || !(statReg_ & 0x10) || (ppu_.lyCounter().ly() == 153 && timeToNextLy <= 4U)))
   {
      unsigned lycCmpLy = ppu_.lyCounter().ly();

      if (ppu_.cgb())
      {
         if (timeToNextLy <= 4U >> isDoubleSpeed())
            lycCmpLy = lycCmpLy < 153 ? lycCmpLy + 1 : 0;
         else if (timeToNextLy <= 8)
            lycCmpLy = 0xFF;
         else if (lycCmpLy == 153 && timeToNextLy <= (448U << isDoubleSpeed()) + 8)
            lycCmpLy = 0;
      }
      else
      {
         if (timeToNextLy <= 4)
            lycCmpLy = 0xFF;
         else if (lycCmpLy == 153 && timeToNextLy <= 444U + 8)
            lycCmpLy = 0;
      }

      if (data == lycCmpLy)
      {
         if (ppu_.cgb() && !isDoubleSpeed())
            eventTimes_.setm<ONESHOT_LCDSTATIRQ>(cycleCounter + 5);
         else
            eventTimes_.flagIrq(2);
      }
   }
}

unsigned LCD::getStat(const unsigned lycReg, const unsigned long cycleCounter)
{
   unsigned stat = 0;

   if (ppu_.lcdc() & 0x80)
   {
      if (cycleCounter >= eventTimes_.nextEventTime())
         update(cycleCounter);

      const unsigned timeToNextLy = ppu_.lyCounter().time() - cycleCounter;

      if (ppu_.lyCounter().ly() > 143)
      {
         if (ppu_.lyCounter().ly() < 153 || timeToNextLy > 4 - isDoubleSpeed() * 4U)
            stat = 1;
      }
      else
      {
         const unsigned lineCycles = 456 - (timeToNextLy >> isDoubleSpeed());

         if (lineCycles < 80)
         {
            if (!ppu_.inactivePeriodAfterDisplayEnable(cycleCounter))
               stat = 2;
         }
         else if (cycleCounter + isDoubleSpeed() - ppu_.cgb() + 2 < m0TimeOfCurrentLine(cycleCounter))
            stat = 3;
      }

      if ((ppu_.lyCounter().ly() == lycReg && timeToNextLy > 4 - isDoubleSpeed() * 4U) ||
            (lycReg == 0 && ppu_.lyCounter().ly() == 153 && timeToNextLy >> isDoubleSpeed() <= 456 - 8))
         stat |= 4;
   }

   return stat;
}

inline void LCD::doMode2IrqEvent()
{
   const unsigned ly = eventTimes_(LY_COUNT) - eventTimes_(MODE2_IRQ) < 8
      ? (ppu_.lyCounter().ly() == 153 ? 0 : ppu_.lyCounter().ly() + 1)
      : ppu_.lyCounter().ly();

   if ((ly != 0 || !(m2IrqStatReg_ & 0x10)) &&
         (!(m2IrqStatReg_ & 0x40) || (lycIrq_.lycReg() != 0 ? ly != (lycIrq_.lycReg() + 1U) : ly > 1)))
      eventTimes_.flagIrq(2);

   m2IrqStatReg_ = statReg_;

   if (!(statReg_ & 0x08))
   {
      unsigned long nextTime = eventTimes_(MODE2_IRQ) + ppu_.lyCounter().lineTime();

      if (ly == 0)
         nextTime -= 4;
      else if (ly == 143)
         nextTime += ppu_.lyCounter().lineTime() * 10 + 4;

      eventTimes_.setm<MODE2_IRQ>(nextTime);
   }
   else
      eventTimes_.setm<MODE2_IRQ>(eventTimes_(MODE2_IRQ) + (70224 << isDoubleSpeed()));
}

inline void LCD::event()
{
   switch (eventTimes_.nextEvent())
   {
      case MEM_EVENT:
         switch (eventTimes_.nextMemEvent())
         {
            case MODE1_IRQ:
               eventTimes_.flagIrq((m1IrqStatReg_ & 0x18) == 0x10 ? 3 : 1);
               m1IrqStatReg_ = statReg_;
               eventTimes_.setm<MODE1_IRQ>(eventTimes_(MODE1_IRQ) + (70224 << isDoubleSpeed()));
               break;

            case LYC_IRQ:
               {
                  unsigned char ifreg = 0;
                  lycIrq_.doEvent(&ifreg, ppu_.lyCounter());
                  eventTimes_.flagIrq(ifreg);
                  eventTimes_.setm<LYC_IRQ>(lycIrq_.time());
               }
               break;
            case SPRITE_MAP:
               eventTimes_.setm<SPRITE_MAP>(ppu_.doSpriteMapEvent(eventTimes_(SPRITE_MAP)));
               mode3CyclesChange();
               break;
            case HDMA_REQ:
               eventTimes_.flagHdmaReq();
               nextM0Time_.predictNextM0Time(ppu_);
               eventTimes_.setm<HDMA_REQ>(hdmaTimeFromM0Time(nextM0Time_.predictedNextM0Time(), isDoubleSpeed()));
               break;
            case MODE2_IRQ:
               doMode2IrqEvent();
               break;
            case MODE0_IRQ:
               {
                  unsigned char ifreg = 0;
                  m0Irq_.doEvent(&ifreg, ppu_.lyCounter().ly(), statReg_, lycIrq_.lycReg());
                  eventTimes_.flagIrq(ifreg);
               }

               eventTimes_.setm<MODE0_IRQ>((statReg_ & 0x08)
                     ? m0IrqTimeFromXpos166Time(ppu_.predictedNextXposTime(166), ppu_.cgb(), isDoubleSpeed())
                     : static_cast<unsigned long>(disabled_time));
               break;
            case ONESHOT_LCDSTATIRQ:
               eventTimes_.flagIrq(2);
               eventTimes_.setm<ONESHOT_LCDSTATIRQ>(disabled_time);
               break;
            case ONESHOT_UPDATEWY2:
               ppu_.updateWy2();
               mode3CyclesChange();
               eventTimes_.setm<ONESHOT_UPDATEWY2>(disabled_time);
               break;
         }
         break;
      case LY_COUNT:
         ppu_.doLyCountEvent();
         eventTimes_.set<LY_COUNT>(ppu_.lyCounter().time());
         break;
   }
}

void LCD::update(const unsigned long cycleCounter)
{
   if (!(ppu_.lcdc() & 0x80))
      return;

   while (cycleCounter >= eventTimes_.nextEventTime())
   {
      ppu_.update(eventTimes_.nextEventTime());
      event();
   }

   ppu_.update(cycleCounter);
}

void LCD::setVideoBuffer(video_pixel_t *const videoBuf, const int pitch)
{
   ppu_.setFrameBuf(videoBuf, pitch);
}

void LCD::setDmgPaletteColor(const unsigned index, const video_pixel_t rgb32)
{
   dmgColorsRgb32_[index] = rgb32;
}

void LCD::setDmgPaletteColor(const unsigned palNum, const unsigned colorNum, const video_pixel_t rgb32)
{
   if (palNum > 2 || colorNum > 3)
      return;

   setDmgPaletteColor(palNum * 4 | colorNum, rgb32);
   refreshPalettes();
}

}
