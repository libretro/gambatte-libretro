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
#include "gambatte-memory.h"
#include "video.h"
#include "sound.h"
#include "inputgetter.h"
#include "savestate.h"
#include <cstring>

namespace gambatte
{

   Memory::Memory(const Interrupter &interrupter_in)
      : vrambank(vram),
      getInput_(0),
      divLastUpdate_(0),
      lastOamDmaUpdate_(DISABLED_TIME),
      lcd_(ioamhram_, vram, VideoInterruptRequester(&intreq_)),
      interrupter_(interrupter_in),
      dmaSource_(0),
      dmaDestination_(0),
      oamDmaPos_(0xFE),
      serialCnt_(0),
      blanklcd_(false)
   {
      intreq_.setEventTime<BLIT>(144*456ul);
      intreq_.setEventTime<END>(0);
   }

   void Memory::setStatePtrs(SaveState &state)
   {
      state.mem.vram.set(vram, sizeof vram);
      state.mem.ioamhram.set(ioamhram_, sizeof ioamhram_);

      cart_.setStatePtrs(state);
      lcd_.setStatePtrs(state);
      psg_.setStatePtrs(state);
   }

   unsigned long Memory::saveState(SaveState &state, unsigned long cycleCounter)
   {
      cycleCounter = resetCounters(cycleCounter);
      nontrivial_ff_read(0xFF05, cycleCounter);
      nontrivial_ff_read(0xFF0F, cycleCounter);
      nontrivial_ff_read(0xFF26, cycleCounter);

      state.mem.divLastUpdate = divLastUpdate_;
      state.mem.nextSerialtime = intreq_.eventTime(SERIAL);
      state.mem.unhaltTime = intreq_.eventTime(UNHALT);
      state.mem.lastOamDmaUpdate = lastOamDmaUpdate_;
      state.mem.dmaSource = dmaSource_;
      state.mem.dmaDestination = dmaDestination_;
      state.mem.oamDmaPos = oamDmaPos_;

      intreq_.saveState(state);
      cart_.saveState(state);
      tima_.saveState(state);
      lcd_.saveState(state);
      psg_.saveState(state);

      return cycleCounter;
   }

   static inline int serialCntFrom(const unsigned long cyclesUntilDone, const bool cgbFast) {
      return cgbFast ? (cyclesUntilDone + 0xF) >> 4 : (cyclesUntilDone + 0x1FF) >> 9;
   }

   void Memory::loadState(const SaveState &state)
   {
      psg_.loadState(state);
      lcd_.loadState(state, state.mem.oamDmaPos < 0xA0 ? cart_.rdisabledRam() : ioamhram_);
      tima_.loadState(state, TimaInterruptRequester(intreq_));
      cart_.loadState(state);
      intreq_.loadState(state);

      divLastUpdate_ = state.mem.divLastUpdate;
      intreq_.setEventTime<SERIAL>(state.mem.nextSerialtime > state.cpu.cycleCounter ? state.mem.nextSerialtime : state.cpu.cycleCounter);
      intreq_.setEventTime<UNHALT>(state.mem.unhaltTime);
      lastOamDmaUpdate_ = state.mem.lastOamDmaUpdate;
      dmaSource_ = state.mem.dmaSource;
      dmaDestination_ = state.mem.dmaDestination;
      oamDmaPos_ = state.mem.oamDmaPos;
      serialCnt_ = intreq_.eventTime(SERIAL) != DISABLED_TIME
         ? serialCntFrom(intreq_.eventTime(SERIAL) - state.cpu.cycleCounter, ioamhram_[0x102] & isCgb() * 2)
         : 8;

      vrambank = vram + (ioamhram_[0x14F] & isCgb()) * 0x2000;

      cart_.setOamDmaSrc(OAM_DMA_SRC_OFF);
      cart_.setWrambank(isCgb() && (ioamhram_[0x170] & 0x07) ? ioamhram_[0x170] & 0x07 : 1);

      if (lastOamDmaUpdate_ != DISABLED_TIME)
      {
         oamDmaInitSetup();

         const unsigned oamEventPos = oamDmaPos_ < 0xA0 ? 0xA0 : 0x100;

         intreq_.setEventTime<OAM>(lastOamDmaUpdate_ + (oamEventPos - oamDmaPos_) * 4);
      }

      intreq_.setEventTime<BLIT>((ioamhram_[0x140] & 0x80) ? lcd_.nextMode1IrqTime() : state.cpu.cycleCounter);
      blanklcd_ = false;

      if (!isCgb())
         std::memset(vram + 0x2000, 0, 0x2000);
   }

   void Memory::setEndtime(const unsigned long cycleCounter, const unsigned long inc)
   {
      if (intreq_.eventTime(BLIT) <= cycleCounter)
         intreq_.setEventTime<BLIT>(intreq_.eventTime(BLIT) + (70224 << isDoubleSpeed()));

      intreq_.setEventTime<END>(cycleCounter + (inc << isDoubleSpeed()));
   }

   void Memory::updateSerial(const unsigned long cc)
   {
      if (intreq_.eventTime(SERIAL) == DISABLED_TIME)
         return;

      if (intreq_.eventTime(SERIAL) <= cc)
      {
         ioamhram_[0x101] = (((ioamhram_[0x101] + 1) << serialCnt_) - 1) & 0xFF;
         ioamhram_[0x102] &= 0x7F;
         intreq_.setEventTime<SERIAL>(DISABLED_TIME);
         intreq_.flagIrq(8);
      }
      else
      {
         const int targetCnt = serialCntFrom(intreq_.eventTime(SERIAL) - cc, ioamhram_[0x102] & isCgb() * 2);
         ioamhram_[0x101] = (((ioamhram_[0x101] + 1) << (serialCnt_ - targetCnt)) - 1) & 0xFF;
         serialCnt_ = targetCnt;
      }
   }

   void Memory::updateTimaIrq(const unsigned long cc)
   {
      while (intreq_.eventTime(TIMA) <= cc)
         tima_.doIrqEvent(TimaInterruptRequester(intreq_));
   }

   void Memory::updateIrqs(const unsigned long cc)
   {
      updateSerial(cc);
      updateTimaIrq(cc);
      lcd_.update(cc);
   }

   unsigned long Memory::event(unsigned long cycleCounter)
   {
      if (lastOamDmaUpdate_ != DISABLED_TIME)
         updateOamDma(cycleCounter);

      switch (intreq_.minEventId())
      {
         case UNHALT:
            intreq_.unhalt();
            intreq_.setEventTime<UNHALT>(DISABLED_TIME);
            break;
         case END:
            intreq_.setEventTime<END>(DISABLED_TIME - 1);

            while (cycleCounter >= intreq_.minEventTime() && intreq_.eventTime(END) != DISABLED_TIME)
               cycleCounter = event(cycleCounter);

            intreq_.setEventTime<END>(DISABLED_TIME);

            break;
         case BLIT:
            {
               const bool lcden = ioamhram_[0x140] >> 7 & 1;
               unsigned long blitTime = intreq_.eventTime(BLIT);

               if (lcden | blanklcd_)
               {
                  lcd_.updateScreen(blanklcd_, cycleCounter);
                  intreq_.setEventTime<BLIT>(DISABLED_TIME);
                  intreq_.setEventTime<END>(DISABLED_TIME);

                  while (cycleCounter >= intreq_.minEventTime())
                     cycleCounter = event(cycleCounter);
               }
               else
                  blitTime += 70224 << isDoubleSpeed();

               blanklcd_ = lcden ^ 1;
               intreq_.setEventTime<BLIT>(blitTime);
            }
            break;
         case SERIAL:
            updateSerial(cycleCounter);
            break;
         case OAM:
            intreq_.setEventTime<OAM>(lastOamDmaUpdate_ == DISABLED_TIME ?
                  static_cast<unsigned long>(DISABLED_TIME) : intreq_.eventTime(OAM) + 0xA0 * 4);
            break;
         case DMA:
            {
               const bool doubleSpeed = isDoubleSpeed();
               unsigned dmaSrc = dmaSource_;
               unsigned dmaDest = dmaDestination_;
               unsigned dmaLength = ((ioamhram_[0x155] & 0x7F) + 0x1) * 0x10;
               unsigned length = hdmaReqFlagged(intreq_) ? 0x10 : dmaLength;

               ackDmaReq(&intreq_);

               if ((static_cast<unsigned long>(dmaDest) + length) & 0x10000)
               {
                  length = 0x10000 - dmaDest;
                  ioamhram_[0x155] |= 0x80;
               }

               dmaLength -= length;

               if (!(ioamhram_[0x140] & 0x80))
                  dmaLength = 0;

               {
                  unsigned long lOamDmaUpdate = lastOamDmaUpdate_;
                  lastOamDmaUpdate_ = DISABLED_TIME;

                  while (length--)
                  {
                     const unsigned src = dmaSrc++ & 0xFFFF;
                     const unsigned data = ((src & 0xE000) == 0x8000 || src > 0xFDFF) ? 0xFF : read(src, cycleCounter);

                     cycleCounter += 2 << doubleSpeed;

                     if (cycleCounter - 3 > lOamDmaUpdate)
                     {
                        oamDmaPos_ = (oamDmaPos_ + 1) & 0xFF;
                        lOamDmaUpdate += 4;

                        if (oamDmaPos_ < 0xA0)
                        {
                           if (oamDmaPos_ == 0)
                              startOamDma(lOamDmaUpdate - 1);

                           ioamhram_[src & 0xFF] = data;
                        }
                        else if (oamDmaPos_ == 0xA0)
                        {
                           endOamDma(lOamDmaUpdate - 1);
                           lOamDmaUpdate = DISABLED_TIME;
                        }
                     }

                     nontrivial_write(0x8000 | (dmaDest++ & 0x1FFF), data, cycleCounter);
                  }

                  lastOamDmaUpdate_ = lOamDmaUpdate;
               }

               cycleCounter += 4;

               dmaSource_ = dmaSrc;
               dmaDestination_ = dmaDest;
               ioamhram_[0x155] = ((dmaLength / 0x10 - 0x1) & 0xFF) | (ioamhram_[0x155] & 0x80);

               if ((ioamhram_[0x155] & 0x80) && lcd_.hdmaIsEnabled())
               {
                  if (lastOamDmaUpdate_ != DISABLED_TIME)
                     updateOamDma(cycleCounter);

                  lcd_.disableHdma(cycleCounter);
               }
            }

            break;
         case TIMA:
            tima_.doIrqEvent(TimaInterruptRequester(intreq_));
            break;
         case VIDEO:
            lcd_.update(cycleCounter);
            break;
         case INTERRUPTS:
            if (halted())
            {
               if (isCgb())
                  cycleCounter += 4;

               intreq_.unhalt();
               intreq_.setEventTime<UNHALT>(DISABLED_TIME);
            }

            if (ime())
            {
               unsigned address;
               const unsigned pendingIrqs = intreq_.pendingIrqs();
               const unsigned n = pendingIrqs & -pendingIrqs;

               if (n < 8)
               {
                  static const unsigned char lut[] = { 0x40, 0x48, 0x48, 0x50 };
                  address = lut[n-1];
               }
               else
                  address = 0x50 + n;

               intreq_.ackIrq(n);
               cycleCounter = interrupter_.interrupt(address, cycleCounter, *this);
            }

            break;
      }

      return cycleCounter;
   }

   unsigned long Memory::stop(unsigned long cycleCounter)
   {
      cycleCounter += 4 << isDoubleSpeed();

      if (ioamhram_[0x14D] & isCgb())
      {
         psg_.generate_samples(cycleCounter, isDoubleSpeed());

         lcd_.speedChange(cycleCounter);
         ioamhram_[0x14D] = ~ioamhram_[0x14D] & 0x80;

         intreq_.setEventTime<BLIT>((ioamhram_[0x140] & 0x80) ? lcd_.nextMode1IrqTime() : cycleCounter + (70224 << isDoubleSpeed()));

         if (intreq_.eventTime(END) > cycleCounter)
         {
            intreq_.setEventTime<END>(cycleCounter + (isDoubleSpeed() ?
                     (intreq_.eventTime(END) - cycleCounter) << 1 : (intreq_.eventTime(END) - cycleCounter) >> 1));
         }
      }

      intreq_.halt();
      intreq_.setEventTime<UNHALT>(cycleCounter + 0x20000 + isDoubleSpeed() * 8);

      return cycleCounter;
   }

   static void decCycles(unsigned long &counter, const unsigned long dec)
   {
      if (counter != DISABLED_TIME)
         counter -= dec;
   }

   void Memory::decEventCycles(const MemEventId eventId, const unsigned long dec)
   {
      if (intreq_.eventTime(eventId) != DISABLED_TIME)
         intreq_.setEventTime(eventId, intreq_.eventTime(eventId) - dec);
   }

   unsigned long Memory::resetCounters(unsigned long cycleCounter)
   {
      if (lastOamDmaUpdate_ != DISABLED_TIME)
         updateOamDma(cycleCounter);

      updateIrqs(cycleCounter);

      const unsigned long oldCC = cycleCounter;

      {
         const unsigned long divinc = (cycleCounter - divLastUpdate_) >> 8;
         ioamhram_[0x104] = (ioamhram_[0x104] + divinc) & 0xFF;
         divLastUpdate_ += divinc << 8;
      }

      const unsigned long dec = cycleCounter < 0x10000 ? 0 : (cycleCounter & ~0x7FFFul) - 0x8000;

      decCycles(divLastUpdate_, dec);
      decCycles(lastOamDmaUpdate_, dec);
      decEventCycles(SERIAL, dec);
      decEventCycles(OAM, dec);
      decEventCycles(BLIT, dec);
      decEventCycles(END, dec);
      decEventCycles(UNHALT, dec);

      cycleCounter -= dec;

      intreq_.resetCc(oldCC, cycleCounter);
      tima_.resetCc(oldCC, cycleCounter, TimaInterruptRequester(intreq_));
      lcd_.resetCc(oldCC, cycleCounter);
      psg_.resetCounter(cycleCounter, oldCC, isDoubleSpeed());

      return cycleCounter;
   }

   void Memory::updateInput()
   {
      unsigned button = 0xFF;
      unsigned dpad = 0xFF;

      if (getInput_)
      {
         const unsigned is = (*getInput_)();
         button ^= is      & 0x0F;
         dpad   ^= is >> 4 & 0x0F;
      }

      ioamhram_[0x100] |= 0xF;

      if (!(ioamhram_[0x100] & 0x10))
         ioamhram_[0x100] &= dpad;

      if (!(ioamhram_[0x100] & 0x20))
         ioamhram_[0x100] &= button;
   }

   void Memory::updateOamDma(const unsigned long cycleCounter)
   {
      const unsigned char *const oamDmaSrc = oamDmaSrcPtr();
      unsigned cycles = (cycleCounter - lastOamDmaUpdate_) >> 2;

      while (cycles--)
      {
         oamDmaPos_ = (oamDmaPos_ + 1) & 0xFF;
         lastOamDmaUpdate_ += 4;

         if (oamDmaPos_ < 0xA0)
         {
            if (oamDmaPos_ == 0)
               startOamDma(lastOamDmaUpdate_ - 1);

            ioamhram_[oamDmaPos_] = oamDmaSrc ? oamDmaSrc[oamDmaPos_] : cart_.rtcRead();
         }
         else if (oamDmaPos_ == 0xA0)
         {
            endOamDma(lastOamDmaUpdate_ - 1);
            lastOamDmaUpdate_ = DISABLED_TIME;
            break;
         }
      }
   }

   void Memory::oamDmaInitSetup()
   {
      if (ioamhram_[0x146] < 0xA0)
         cart_.setOamDmaSrc(ioamhram_[0x146] < 0x80 ? OAM_DMA_SRC_ROM : OAM_DMA_SRC_VRAM);
      else if (ioamhram_[0x146] < 0xFE - isCgb() * 0x1E)
         cart_.setOamDmaSrc(ioamhram_[0x146] < 0xC0 ? OAM_DMA_SRC_SRAM : OAM_DMA_SRC_WRAM);
      else
         cart_.setOamDmaSrc(OAM_DMA_SRC_INVALID);
   }

   static const unsigned char * oamDmaSrcZero()
   {
      static unsigned char zeroMem[0xA0];
      return zeroMem;
   }

   const unsigned char * Memory::oamDmaSrcPtr() const
   {
      switch (cart_.oamDmaSrc())
      {
         case OAM_DMA_SRC_ROM:
            return cart_.romdata(ioamhram_[0x146] >> 6) + (ioamhram_[0x146] << 8);
         case OAM_DMA_SRC_SRAM:
            return cart_.rsrambankptr() ? cart_.rsrambankptr() + (ioamhram_[0x146] << 8) : 0;
         case OAM_DMA_SRC_VRAM:
            return vrambank + (ioamhram_[0x146] << 8 & 0x1FFF);
         case OAM_DMA_SRC_WRAM:
            return cart_.wramdata(ioamhram_[0x146] >> 4 & 1) + (ioamhram_[0x146] << 8 & 0xFFF);
         case OAM_DMA_SRC_INVALID:
         case OAM_DMA_SRC_OFF:
            break;
      }

      return ioamhram_[0x146] == 0xFF && !isCgb() ? oamDmaSrcZero() : cart_.rdisabledRam();
   }

   void Memory::startOamDma(const unsigned long cycleCounter)
   {
      lcd_.oamChange(cart_.rdisabledRam(), cycleCounter);
   }

   void Memory::endOamDma(const unsigned long cycleCounter)
   {
      oamDmaPos_ = 0xFE;
      cart_.setOamDmaSrc(OAM_DMA_SRC_OFF);
      lcd_.oamChange(ioamhram_, cycleCounter);
   }

   unsigned Memory::nontrivial_ff_read(const unsigned P, const unsigned long cycleCounter)
   {
      if (lastOamDmaUpdate_ != DISABLED_TIME)
         updateOamDma(cycleCounter);

      switch (P & 0x7F)
      {
         case 0x00:
            updateInput();
            break;
         case 0x01:
         case 0x02:
            updateSerial(cycleCounter);
            break;
         case 0x04:
            {
               const unsigned long divcycles = (cycleCounter - divLastUpdate_) >> 8;
               ioamhram_[0x104] = (ioamhram_[0x104] + divcycles) & 0xFF;
               divLastUpdate_ += divcycles << 8;
            }

            break;
         case 0x05:
            ioamhram_[0x105] = tima_.tima(cycleCounter);
            break;
         case 0x0F:
            updateIrqs(cycleCounter);
            ioamhram_[0x10F] = intreq_.ifreg();
            break;
         case 0x26:
            if (ioamhram_[0x126] & 0x80)
            {
               psg_.generate_samples(cycleCounter, isDoubleSpeed());
               ioamhram_[0x126] = 0xF0 | psg_.getStatus();
            }
            else
               ioamhram_[0x126] = 0x70;

            break;
         case 0x30:
         case 0x31:
         case 0x32:
         case 0x33:
         case 0x34:
         case 0x35:
         case 0x36:
         case 0x37:
         case 0x38:
         case 0x39:
         case 0x3A:
         case 0x3B:
         case 0x3C:
         case 0x3D:
         case 0x3E:
         case 0x3F:
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            return psg_.waveRamRead(P & 0xF);
         case 0x41:
            return ioamhram_[0x141] | lcd_.getStat(ioamhram_[0x145], cycleCounter);
         case 0x44:
            return lcd_.getLyReg(cycleCounter/*+4*/);
         case 0x69:
            return lcd_.cgbBgColorRead(ioamhram_[0x168] & 0x3F, cycleCounter);
         case 0x6B:
            return lcd_.cgbSpColorRead(ioamhram_[0x16A] & 0x3F, cycleCounter);
         default: break;
      }

      return ioamhram_[P - 0xFE00];
   }

   static bool isInOamDmaConflictArea(const OamDmaSrc oamDmaSrc, const unsigned addr, const bool cgb)
   {
      struct Area { unsigned short areaUpper, exceptAreaLower, exceptAreaWidth, pad; };

      static const Area cgbAreas[] = {
         { 0xC000, 0x8000, 0x2000, 0 },
         { 0xC000, 0x8000, 0x2000, 0 },
         { 0xA000, 0x0000, 0x8000, 0 },
         { 0xFE00, 0x0000, 0xC000, 0 },
         { 0xC000, 0x8000, 0x2000, 0 },
         { 0x0000, 0x0000, 0x0000, 0 }
      };

      static const Area dmgAreas[] = {
         { 0xFE00, 0x8000, 0x2000, 0 },
         { 0xFE00, 0x8000, 0x2000, 0 },
         { 0xA000, 0x0000, 0x8000, 0 },
         { 0xFE00, 0x8000, 0x2000, 0 },
         { 0xFE00, 0x8000, 0x2000, 0 },
         { 0x0000, 0x0000, 0x0000, 0 }
      };

      const Area *const a = cgb ? cgbAreas : dmgAreas;

      return addr < a[oamDmaSrc].areaUpper && addr - a[oamDmaSrc].exceptAreaLower >= a[oamDmaSrc].exceptAreaWidth;
   }

   unsigned Memory::nontrivial_read(const unsigned P, const unsigned long cycleCounter)
   {
      if (P < 0xFF80)
      {
         if (lastOamDmaUpdate_ != DISABLED_TIME)
         {
            updateOamDma(cycleCounter);

            if (isInOamDmaConflictArea(cart_.oamDmaSrc(), P, isCgb()) && oamDmaPos_ < 0xA0)
               return ioamhram_[oamDmaPos_];
         }

         if (P < 0xC000)
         {
            if (P < 0x8000)
               return cart_.romdata(P >> 14)[P];

            if (P < 0xA000)
            {
               if (!lcd_.vramAccessible(cycleCounter))
                  return 0xFF;

               return vrambank[P - 0x8000];
            }

            if (cart_.rsrambankptr())
               return cart_.rsrambankptr()[P];

            return cart_.rtcRead();
         }

         if (P < 0xFE00)
            return cart_.wramdata(P >> 12 & 1)[P & 0xFFF];

         if (P >= 0xFF00)
            return nontrivial_ff_read(P, cycleCounter);

         if (!lcd_.oamReadable(cycleCounter) || oamDmaPos_ < 0xA0)
            return 0xFF;
      }

      return ioamhram_[P - 0xFE00];
   }

   void Memory::nontrivial_ff_write(const unsigned P, unsigned data, const unsigned long cycleCounter)
   {
      if (lastOamDmaUpdate_ != DISABLED_TIME)
         updateOamDma(cycleCounter);

      switch (P & 0xFF)
      {
         case 0x00:
            data = (ioamhram_[0x100] & 0xCF) | (data & 0xF0);
            break;
         case 0x01:
            updateSerial(cycleCounter);
            break;
         case 0x02:
            updateSerial(cycleCounter);

            serialCnt_ = 8;
            intreq_.setEventTime<SERIAL>((data & 0x81) == 0x81
                  ? (data & isCgb() * 2 ? (cycleCounter & ~0x7ul) + 0x10 * 8 : (cycleCounter & ~0xFFul) + 0x200 * 8)
                  : static_cast<unsigned long>(DISABLED_TIME));

            data |= 0x7E - isCgb() * 2;
            break;
         case 0x04:
            ioamhram_[0x104] = 0;
            divLastUpdate_ = cycleCounter;
            return;
         case 0x05:
            tima_.setTima(data, cycleCounter, TimaInterruptRequester(intreq_));
            break;
         case 0x06:
            tima_.setTma(data, cycleCounter, TimaInterruptRequester(intreq_));
            break;
         case 0x07:
            data |= 0xF8;
            tima_.setTac(data, cycleCounter, TimaInterruptRequester(intreq_));
            break;
         case 0x0F:
            updateIrqs(cycleCounter);
            intreq_.setIfreg(0xE0 | data);
            return;
         case 0x10:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr10(data);
            data |= 0x80;
            break;
         case 0x11:
            if (!psg_.isEnabled())
            {
               if (isCgb())
                  return;

               data &= 0x3F;
            }

            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr11(data);
            data |= 0x3F;
            break;
         case 0x12:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr12(data);
            break;
         case 0x13:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr13(data);
            return;
         case 0x14:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr14(data);
            data |= 0xBF;
            break;
         case 0x16:
            if (!psg_.isEnabled())
            {
               if (isCgb())
                  return;

               data &= 0x3F;
            }

            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr21(data);
            data |= 0x3F;
            break;
         case 0x17:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr22(data);
            break;
         case 0x18:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr23(data);
            return;
         case 0x19:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr24(data);
            data |= 0xBF;
            break;
         case 0x1A:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr30(data);
            data |= 0x7F;
            break;
         case 0x1B:
            if (!psg_.isEnabled() && isCgb()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr31(data);
            return;
         case 0x1C:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr32(data);
            data |= 0x9F;
            break;
         case 0x1D:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr33(data);
            return;
         case 0x1E:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr34(data);
            data |= 0xBF;
            break;
         case 0x20:
            if (!psg_.isEnabled() && isCgb()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr41(data);
            return;
         case 0x21:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr42(data);
            break;
         case 0x22:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr43(data);
            break;
         case 0x23:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_nr44(data);
            data |= 0xBF;
            break;
         case 0x24:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.set_so_volume(data);
            break;
         case 0x25:
            if (!psg_.isEnabled()) return;
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.map_so(data);
            break;
         case 0x26:
            if ((ioamhram_[0x126] ^ data) & 0x80)
            {
               psg_.generate_samples(cycleCounter, isDoubleSpeed());

               if (!(data & 0x80))
               {
                  for (unsigned i = 0xFF10; i < 0xFF26; ++i)
                     ff_write(i, 0, cycleCounter);

                  psg_.setEnabled(false);
               }
               else
               {
                  psg_.reset();
                  psg_.setEnabled(true);
               }
            }

            data = (data & 0x80) | (ioamhram_[0x126] & 0x7F);
            break;
         case 0x30:
         case 0x31:
         case 0x32:
         case 0x33:
         case 0x34:
         case 0x35:
         case 0x36:
         case 0x37:
         case 0x38:
         case 0x39:
         case 0x3A:
         case 0x3B:
         case 0x3C:
         case 0x3D:
         case 0x3E:
         case 0x3F:
            psg_.generate_samples(cycleCounter, isDoubleSpeed());
            psg_.waveRamWrite(P & 0xF, data);
            break;
         case 0x40:
            if (ioamhram_[0x140] != data)
            {
               if ((ioamhram_[0x140] ^ data) & 0x80)
               {
                  const unsigned lyc = lcd_.getStat(ioamhram_[0x145], cycleCounter) & 4;
                  const bool hdmaEnabled = lcd_.hdmaIsEnabled();

                  lcd_.lcdcChange(data, cycleCounter);
                  ioamhram_[0x144] = 0;
                  ioamhram_[0x141] &= 0xF8;

                  if (data & 0x80)
                     intreq_.setEventTime<BLIT>(lcd_.nextMode1IrqTime() + (blanklcd_ ? 0 : 70224 << isDoubleSpeed()));
                  else
                  {
                     ioamhram_[0x141] |= lyc;
                     intreq_.setEventTime<BLIT>(cycleCounter + (456 * 4 << isDoubleSpeed()));

                     if (hdmaEnabled)
                        flagHdmaReq(&intreq_);
                  }
               } else
                  lcd_.lcdcChange(data, cycleCounter);

               ioamhram_[0x140] = data;
            }

            return;
         case 0x41:
            lcd_.lcdstatChange(data, cycleCounter);
            data = (ioamhram_[0x141] & 0x87) | (data & 0x78);
            break;
         case 0x42:
            lcd_.scyChange(data, cycleCounter);
            break;
         case 0x43:
            lcd_.scxChange(data, cycleCounter);
            break;
         case 0x45:
            lcd_.lycRegChange(data, cycleCounter);
            break;
         case 0x46:
            if (lastOamDmaUpdate_ != DISABLED_TIME)
               endOamDma(cycleCounter);

            lastOamDmaUpdate_ = cycleCounter;
            intreq_.setEventTime<OAM>(cycleCounter + 8);
            ioamhram_[0x146] = data;
            oamDmaInitSetup();
            return;
         case 0x47:
            if (!isCgb())
               lcd_.dmgBgPaletteChange(data, cycleCounter);

            break;
         case 0x48:
            if (!isCgb())
               lcd_.dmgSpPalette1Change(data, cycleCounter);

            break;
         case 0x49:
            if (!isCgb())
               lcd_.dmgSpPalette2Change(data, cycleCounter);

            break;
         case 0x4A:
            lcd_.wyChange(data, cycleCounter);
            break;
         case 0x4B:
            lcd_.wxChange(data, cycleCounter);
            break;

         case 0x4D:
            ioamhram_[0x14D] |= data & 0x01;
            return;
         case 0x4F:
            if (isCgb()) {
               vrambank = vram + (data & 0x01) * 0x2000;
               ioamhram_[0x14F] = 0xFE | data;
            }

            return;
         case 0x51:
            dmaSource_ = data << 8 | (dmaSource_ & 0xFF);
            return;
         case 0x52:
            dmaSource_ = (dmaSource_ & 0xFF00) | (data & 0xF0);
            return;
         case 0x53:
            dmaDestination_ = data << 8 | (dmaDestination_ & 0xFF);
            return;
         case 0x54:
            dmaDestination_ = (dmaDestination_ & 0xFF00) | (data & 0xF0);
            return;
         case 0x55:
            if (isCgb())
            {
               ioamhram_[0x155] = data & 0x7F;

               if (lcd_.hdmaIsEnabled())
               {
                  if (!(data & 0x80))
                  {
                     ioamhram_[0x155] |= 0x80;
                     lcd_.disableHdma(cycleCounter);
                  }
               }
               else
               {
                  if (data & 0x80)
                  {
                     if (ioamhram_[0x140] & 0x80)
                        lcd_.enableHdma(cycleCounter);
                     else
                        flagHdmaReq(&intreq_);
                  }
                  else
                     flagGdmaReq(&intreq_);
               }
            }

            return;
         case 0x56:
            if (isCgb())
               ioamhram_[0x156] = data | 0x3E;

            return;
         case 0x68:
            if (isCgb())
               ioamhram_[0x168] = data | 0x40;

            return;
         case 0x69:
            if (isCgb())
            {
               const unsigned index = ioamhram_[0x168] & 0x3F;

               lcd_.cgbBgColorChange(index, data, cycleCounter);

               ioamhram_[0x168] = (ioamhram_[0x168] & ~0x3F) | ((index + (ioamhram_[0x168] >> 7)) & 0x3F);
            }

            return;
         case 0x6A:
            if (isCgb())
               ioamhram_[0x16A] = data | 0x40;

            return;
         case 0x6B:
            if (isCgb())
            {
               const unsigned index = ioamhram_[0x16A] & 0x3F;

               lcd_.cgbSpColorChange(index, data, cycleCounter);

               ioamhram_[0x16A] = (ioamhram_[0x16A] & ~0x3F) | ((index + (ioamhram_[0x16A] >> 7)) & 0x3F);
            }

            return;
         case 0x6C:
            if (isCgb())
               ioamhram_[0x16C] = data | 0xFE;

            return;
         case 0x70:
            if (isCgb())
            {
               cart_.setWrambank((data & 0x07) ? (data & 0x07) : 1);
               ioamhram_[0x170] = data | 0xF8;
            }
            return;
         case 0x72:
         case 0x73:
         case 0x74:
            if (isCgb())
               break;

            return;
         case 0x75:
            if (isCgb())
               ioamhram_[0x175] = data | 0x8F;

            return;
         case 0xFF:
            intreq_.setIereg(data);
            break;
         default:
            return;
      }

      ioamhram_[P - 0xFE00] = data;
   }

   void Memory::nontrivial_write(const unsigned P, const unsigned data, const unsigned long cycleCounter)
   {
      if (lastOamDmaUpdate_ != DISABLED_TIME)
      {
         updateOamDma(cycleCounter);

         if (isInOamDmaConflictArea(cart_.oamDmaSrc(), P, isCgb()) && oamDmaPos_ < 0xA0)
         {
            ioamhram_[oamDmaPos_] = data;
            return;
         }
      }

      if (P < 0xFE00)
      {
         if (P < 0xA000)
         {
            if (P < 0x8000)
               cart_.mbcWrite(P, data);
            else if (lcd_.vramAccessible(cycleCounter))
            {
               lcd_.vramChange(cycleCounter);
               vrambank[P - 0x8000] = data;
            }
         }
         else if (P < 0xC000)
         {
            if (cart_.wsrambankptr())
               cart_.wsrambankptr()[P] = data;
            else
               cart_.rtcWrite(data);
         }
         else
            cart_.wramdata(P >> 12 & 1)[P & 0xFFF] = data;
      }
      else if (P - 0xFF80u >= 0x7Fu)
      {
         if (P < 0xFF00)
         {
            if (lcd_.oamWritable(cycleCounter) && oamDmaPos_ >= 0xA0 && (P < 0xFEA0 || isCgb()))
            {
               lcd_.oamChange(cycleCounter);
               ioamhram_[P - 0xFE00] = data;
            }
         }
         else
            nontrivial_ff_write(P, data, cycleCounter);
      }
      else
         ioamhram_[P - 0xFE00] = data;
   }

   bool Memory::loadROM(const void *romdata, unsigned romsize, const bool forceDmg, const bool multicartCompat)
   {
      if (cart_.loadROM(romdata, romsize, forceDmg, multicartCompat))
         return true;

      psg_.init(cart_.isCgb());
      lcd_.reset(ioamhram_, cart_.isCgb());
      interrupter_.setGameShark(std::string());

      return false;
   }

   unsigned Memory::fillSoundBuffer(const unsigned long cycleCounter)
   {
      psg_.generate_samples(cycleCounter, isDoubleSpeed());
      return psg_.fillBuffer();
   }

   void Memory::setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32)
   {
      lcd_.setDmgPaletteColor(palNum, colorNum, rgb32);
   }

}
