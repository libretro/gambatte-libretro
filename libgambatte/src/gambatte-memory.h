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
#ifndef MEMORY_H
#define MEMORY_H

#include "mem/cartridge.h"
#include "interrupter.h"
#include "sound.h"
#include "tima.h"
#include "video.h"

namespace gambatte
{
   class InputGetter;
   class FilterInfo;

   class Memory
   {
      public:
      explicit Memory(const Interrupter &interrupter);

      void setStatePtrs(SaveState &state);

#ifdef __LIBRETRO__
      void *savedata_ptr() { return cart_.savedata_ptr(); }
      unsigned savedata_size() { return cart_.savedata_size(); }
      void *rtcdata_ptr() { return cart_.rtcdata_ptr(); }
      unsigned rtcdata_size() { return cart_.rtcdata_size(); }
#endif

      void clearCheats() { cart_.clearCheats(); }

      void display_setColorCorrection(bool enable) { lcd_.setColorCorrection(enable); }
      video_pixel_t display_gbcToRgb32(const unsigned bgr15) { return lcd_.gbcToRgb32(bgr15); }

      void loadState(const SaveState &state/*, unsigned long oldCc*/);

      unsigned long saveState(SaveState &state, unsigned long cc);

      unsigned long stop(unsigned long cycleCounter);
      bool isCgb() const { return lcd_.isCgb(); }
      bool ime() const { return intreq_.ime(); }
      bool halted() const { return intreq_.halted(); }

      unsigned long nextEventTime() const { return intreq_.minEventTime(); }

      bool isActive() const { return intreq_.eventTime(END) != DISABLED_TIME; }

      long cyclesSinceBlit(const unsigned long cc) const
      {
         if (cc < intreq_.eventTime(BLIT))
            return -1;
         return (cc - intreq_.eventTime(BLIT)) >> isDoubleSpeed();
      }

      void halt() { intreq_.halt(); }
      void ei(unsigned long cycleCounter) { if (!ime()) { intreq_.ei(cycleCounter); } }
      void di() { intreq_.di(); }

      unsigned ff_read(const unsigned P, const unsigned long cycleCounter)
      {
         return P < 0xFF80 ? nontrivial_ff_read(P, cycleCounter) : ioamhram_[P - 0xFE00];
      }

      unsigned read(const unsigned P, const unsigned long cycleCounter)
      {
         return cart_.rmem(P >> 12) ? cart_.rmem(P >> 12)[P] : nontrivial_read(P, cycleCounter);
      }

      void write(const unsigned P, const unsigned data, const unsigned long cycleCounter)
      {
         if (cart_.wmem(P >> 12))
            cart_.wmem(P >> 12)[P] = data;
         else
            nontrivial_write(P, data, cycleCounter);
      }

      void ff_write(const unsigned P, const unsigned data, const unsigned long cycleCounter)
      {
         if (P - 0xFF80u < 0x7Fu)
            ioamhram_[P - 0xFE00] = data;
         else
            nontrivial_ff_write(P, data, cycleCounter);
      }

      unsigned long event(unsigned long cycleCounter);
      unsigned long resetCounters(unsigned long cycleCounter);

      bool loadROM(const std::string &romfile, bool forceDmg, bool multiCartCompat);
      bool loadROM(const void *romdata, unsigned romsize, bool forceDmg, bool multiCartCompat);

      void setSaveDir(const std::string &dir)
      {
         cart_.setSaveDir(dir);
      }

      void setInputGetter(InputGetter *getInput)
      {
         getInput_ = getInput;
      }

      void setEndtime(unsigned long cc, unsigned long inc);
      void setSoundBuffer(uint_least32_t *const buf) { psg_.setBuffer(buf); }
      unsigned fillSoundBuffer(unsigned long cc);

      void setVideoBuffer(video_pixel_t *const videoBuf, const int pitch)
      {
         lcd_.setVideoBuffer(videoBuf, pitch);
      }

      void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32);

      void setGameGenie(const std::string &codes) { cart_.setGameGenie(codes); }
      void setGameShark(const std::string &codes) { interrupter_.setGameShark(codes); }
      void updateInput();

      private:
      Cartridge cart_;
      unsigned char ioamhram_[0x200];
      InputGetter *getInput_;
      unsigned long divLastUpdate_;
      unsigned long lastOamDmaUpdate_;
      InterruptRequester intreq_;
      Tima tima_;
      LCD lcd_;
      PSG psg_;
      Interrupter interrupter_;
      unsigned short dmaSource_;
      unsigned short dmaDestination_;
      unsigned char oamDmaPos_;
      unsigned char serialCnt_;
      bool blanklcd_;

      unsigned char vram[0x2000 * 2];
      unsigned char *vrambank;

      void decEventCycles(MemEventId eventId, unsigned long dec);
      void oamDmaInitSetup();
      void updateOamDma(unsigned long cycleCounter);
      void startOamDma(unsigned long cycleCounter);
      void endOamDma(unsigned long cycleCounter);
      const unsigned char * oamDmaSrcPtr() const;
      unsigned nontrivial_ff_read(unsigned P, unsigned long cycleCounter);
      unsigned nontrivial_read(unsigned P, unsigned long cycleCounter);
      void nontrivial_ff_write(unsigned P, unsigned data, unsigned long cycleCounter);
      void nontrivial_write(unsigned P, unsigned data, unsigned long cycleCounter);
      void updateSerial(unsigned long cc);
      void updateTimaIrq(unsigned long cc);
      void updateIrqs(unsigned long cc);
      bool isDoubleSpeed() const { return lcd_.isDoubleSpeed(); }

   };

}

#endif
