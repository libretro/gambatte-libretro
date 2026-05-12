/***************************************************************************
 *   Copyright (C) 2007-2010 by Sindre Aamås                               *
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
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "memptrs.h"
#include "rtc.h"
#include "huc3.h"
#include "savestate.h"
#include <memory>
#include <string>
#include <vector>

namespace gambatte
{
   class Mbc
   {
      public:
         virtual ~Mbc() {}
         virtual void romWrite(unsigned P, unsigned data) {};
         virtual void saveState(SaveState::Mem &ss) const {};
         virtual void loadState(const SaveState::Mem &ss) {};
         virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned address, unsigned rombank) const = 0;
         /* Called once when the boot ROM hands control to the cartridge
          * (write of 1 to 0xFF50). No MBC currently implements this --
          * the Sachen MMC1 now unlocks on read-count, not on FF50 --
          * but the hook is kept for any future MBC that needs to
          * sync with end-of-bootstrap. Default is a no-op so every
          * other MBC is unaffected. */
         virtual void bootloaderFinished() {}
         /* Sachen MMC1 only: pointer to the mapper's lock-progress
          * counter (a single unsigned char shared with Memory) and
          * the unlock entry point. Default null/no-op for every
          * other MBC. */
         virtual unsigned char *lockCounterPtr() { return 0; }
         virtual void unlock() {}
   };

   class Cartridge
   {
      public:
         Cartridge() : isSachen_(false) {}
         void setStatePtrs(SaveState &);
         void saveState(SaveState &) const;
         void loadState(const SaveState &);

         bool loaded() const { return mbc.get(); }

         const unsigned char * rmem(unsigned area) const
         {
            return memptrs_.rmem(area);
         }

         unsigned char * wmem(unsigned area) const
         {
            return memptrs_.wmem(area);
         }

         unsigned char * vramdata() const
         {
            return memptrs_.vramdata();
         }

         unsigned char * romdata(unsigned area) const 
         {
            return memptrs_.romdata(area);
         }

         unsigned char * wramdata(unsigned area) const
         {
            return memptrs_.wramdata(area);
         }

         const unsigned char * rdisabledRam() const
         {
            return memptrs_.rdisabledRam();
         }

         const unsigned char * rsrambankptr() const
         {
            return memptrs_.rsrambankptr();
         }

         unsigned char * wsrambankptr() const
         {
            return memptrs_.wsrambankptr();
         }

         unsigned char * vrambankptr() const
         {
            return memptrs_.vrambankptr();
         }

         OamDmaSrc oamDmaSrc() const
         {
            return memptrs_.oamDmaSrc();
         }

         void setVrambank(unsigned bank)
         {
            memptrs_.setVrambank(bank);
         }

         void setWrambank(unsigned bank)
         {
            memptrs_.setWrambank(bank);
         }
         void setOamDmaSrc(OamDmaSrc oamDmaSrc)
         {
            memptrs_.setOamDmaSrc(oamDmaSrc);
         }

         void mbcWrite(unsigned addr, unsigned data) { mbc->romWrite(addr, data); }

         bool isCgb() const
         {
            return gambatte::isCgb(memptrs_);
         }

         void rtcWrite(unsigned data)
         {
            rtc_.write(data);
         }

         unsigned char rtcRead() const 
         {
            return *rtc_.getActive();
         }
         
         const std::string saveBasePath() const;
         void setSaveDir(const std::string &dir);
         int loadROM(const void *romdata, unsigned int romsize, unsigned int forceModel, bool multicartCompat);
         void setGameGenie(const std::string &codes);
         void clearCheats();

         bool isHuC3() const { return huc3_.isHuC3(); }
         unsigned char HuC3Read(unsigned p, unsigned long const cc) { return huc3_.read(p, cc); }
         void HuC3Write(unsigned p, unsigned data) { huc3_.write(p, data); }

         /* Sachen MMC1 (unlicensed mapper used by Sachen 4-in-1 etc.):
          * the cartridge header is bit-scrambled and a "locked" boot
          * state requires hooks outside the normal MBC write path.
          * isSachen()                returns true for Sachen MMC1 carts.
          * sachenLockSetup(haveBoot) is called once after the bootloader
          *                           has been installed (or skipped) to
          *                           overlay the locked header bytes
          *                           when the bootstrap will run, or to
          *                           inject the Sachen logo into VRAM
          *                           when running without a bootstrap.
          * sachenLockCounterPtr()    returns a pointer to the mapper's
          *                           "cart reads in 0x0100..0x01FF"
          *                           counter, or null for non-Sachen
          *                           carts. Memory::read bumps it and
          *                           calls onSachenUnlock() when the
          *                           bootstrap finishes its logo
          *                           display pass.
          * onSachenUnlock()          drops the locked state mid-boot
          *                           so the bootstrap's verify pass
          *                           sees the unscrambled Nintendo
          *                           logo bytes and succeeds.
          * onBootloaderFinished()    is a no-op for the new Sachen
          *                           implementation (lock now drops
          *                           on read-count, not on 0xFF50)
          *                           but is kept for any future MBC
          *                           that needs the hook.
          * For every non-Sachen cart these functions are no-ops. */
         bool isSachen() const { return isSachen_; }
         void sachenLockSetup(bool bootloaderUsed);
         unsigned char *sachenLockCounterPtr();
         void onSachenUnlock();
         void onBootloaderFinished() { if (mbc.get()) mbc->bootloaderFinished(); }

         void *savedata_ptr();
         unsigned savedata_size();

         // Not endian-safe at all, but hey.
         void *rtcdata_ptr();
         unsigned rtcdata_size();

      private:
         struct AddrData
         {
            unsigned long addr;
            unsigned char data;
            AddrData(unsigned long addr, unsigned data) : addr(addr), data(data)
            {
            }
         };
         MemPtrs memptrs_;
         Rtc rtc_;
         HuC3Chip huc3_;
         /* True only when the loaded ROM was identified as a Sachen
          * MMC1 unlicensed cart via the scrambled-logo checksum check
          * in loadROM. Used to route bootloader/VRAM hooks. */
         bool isSachen_;

#if __cplusplus >= 201103L
         std::unique_ptr<Mbc> mbc;
#else
         std::auto_ptr<Mbc> mbc;
#endif

         std::vector<AddrData> ggUndoList_;

         void applyGameGenie(const std::string &code);
   };

}

#endif
