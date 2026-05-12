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
#include "cartridge.h"
#include "../savestate.h"
#include <cstring>
#include <string.h>
#include <algorithm>
#include "gambatte_log.h"

extern void cartridge_set_rumble(unsigned active);

namespace gambatte
{

   static unsigned toMulti64Rombank(const unsigned rombank)
   {
      return (rombank >> 1 & 0x30) | (rombank & 0xF);
   }

   static inline unsigned rambanks(MemPtrs const &memptrs)
   {
      return (memptrs.rambankdataend() - memptrs.rambankdata()) / 0x2000;
   }

   static inline unsigned rombanks(MemPtrs const &memptrs)
   {
      return (memptrs.romdataend()     - memptrs.romdata()    ) / 0x4000;
   }

   class DefaultMbc : public Mbc {
      public:
         virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
            return (addr< 0x4000) == (bank == 0);
         }
   };
   class Mbc0 : public DefaultMbc {
      MemPtrs &memptrs;
      bool enableRam;
      public:
      explicit Mbc0(MemPtrs &memptrs)
         : memptrs(memptrs),
         enableRam(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         if (P < 0x2000) {
            enableRam = (data & 0xF) == 0xA;
            memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.enableRam = enableRam;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         enableRam = ss.enableRam;
         memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
      }
   };

   class Mbc1 : public DefaultMbc {
      MemPtrs &memptrs;
      unsigned char rombank;
      unsigned char rambank;
      bool enableRam;
      bool rambankMode;
      static unsigned adjustedRombank(unsigned bank) { return (bank & 0x1F) ? bank : bank | 1; }
      void setRambank() const { memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, rambank & (rambanks(memptrs) - 1)); }
      void setRombank() const { memptrs.setRombank(adjustedRombank(rombank) & (rombanks(memptrs) - 1)); }
      public:
      explicit Mbc1(MemPtrs &memptrs)
         : memptrs(memptrs),
         rombank(1),
         rambank(0),
         enableRam(false),
         rambankMode(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 13 & 3) {
            case 0:
               enableRam = (data & 0xF) == 0xA;
               setRambank();
               break;
            case 1:
               rombank = rambankMode ? data & 0x1F : (rombank & 0x60) | (data & 0x1F);
               setRombank();
               break;
            case 2:
               if (rambankMode) {
                  rambank = data & 3;
                  setRambank();
               } else {
                  rombank = (data << 5 & 0x60) | (rombank & 0x1F);
                  setRombank();
               }
               break;
            case 3:
               // Pretty sure this should take effect immediately, but I have a policy not to change old behavior
               // unless I have something (eg. a verified test or a game) that justifies it.
               rambankMode = data & 1;
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.rambank = rambank;
         ss.enableRam = enableRam;
         ss.rambankMode = rambankMode;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         rambank = ss.rambank;
         enableRam = ss.enableRam;
         rambankMode = ss.rambankMode;
         setRambank();
         setRombank();
      }
   };
   class Mbc1Multi64 : public Mbc {
      MemPtrs &memptrs;
      unsigned char rombank;
      bool enableRam;
      bool rombank0Mode;
      static unsigned adjustedRombank(unsigned bank) { return (bank & 0x1F) ? bank : bank | 1; }
      void setRombank() const {
         if (rombank0Mode) {
            const unsigned rb = toMulti64Rombank(rombank);
            memptrs.setRombank0(rb & 0x30);
            memptrs.setRombank(adjustedRombank(rb));
         } else {
            memptrs.setRombank0(0);
            memptrs.setRombank(adjustedRombank(rombank) & (rombanks(memptrs) - 1));
         }
      }
      public:
      explicit Mbc1Multi64(MemPtrs &memptrs)
         : memptrs(memptrs),
         rombank(1),
         enableRam(false),
         rombank0Mode(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 13 & 3) {
            case 0:
               enableRam = (data & 0xF) == 0xA;
               memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
               break;
            case 1:
               rombank = (rombank & 0x60) | (data & 0x1F);
               memptrs.setRombank(rombank0Mode
                        ? adjustedRombank(toMulti64Rombank(rombank))
                        : adjustedRombank(rombank) & (rombanks(memptrs) - 1));
               break;
            case 2:
               rombank = (data << 5 & 0x60) | (rombank & 0x1F);
               setRombank();
               break;
            case 3:
               rombank0Mode = data & 1;
               setRombank();
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.enableRam = enableRam;
         ss.rambankMode = rombank0Mode;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         enableRam = ss.enableRam;
         rombank0Mode = ss.rambankMode;
         memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
         setRombank();
      }
      virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
         return (addr < 0x4000) == ((bank & 0xF) == 0);
      }
   };
   /* ---------------------------------------------------------------- *
    *  Sachen MMC1                                                     *
    * ---------------------------------------------------------------- *
    *  Unlicensed Game Boy mapper used by Sachen multi-cart releases   *
    *  such as "4 in 1 (Europe) (4B-001)" -- the cartridge that        *
    *  prompted this implementation (libretro/gambatte-libretro #190). *
    *                                                                  *
    *  Reference: https://wiki.tauwasser.eu/view/Sachen_MMC1           *
    *  Original implementation by NewRisingSun in hhugboy 1.3.0        *
    *  (MbcUnlSachenMMC1.cpp / .h, released under CC0 and GPL-2).      *
    *  See also mGBA 0.10 which ports the same logic.                  *
    *                                                                  *
    *  Salient hardware quirks that this implementation reproduces:    *
    *                                                                  *
    *  1. Header scramble. While the CPU bus address satisfies         *
    *        A8 = 1 and A15..A9 = 0  (i.e. 0x0100..0x01FF),            *
    *     the mapper performs the bit permutation                      *
    *        RA0 <- A6 , RA1 <- A4 , RA4 <- A1 , RA6 <- A0             *
    *     when fetching from the attached ROM. The permutation is an   *
    *     involution -- doing it twice yields the original address --  *
    *     and it is used to obscure each game's header in the ROM      *
    *     image. We undo it at load time by *pre-descrambling* the     *
    *     0x100..0x1FF window of every 16 KiB bank in place. This      *
    *     keeps the hot read path (a single pointer indirection in     *
    *     MemPtrs) free of any per-access conditional. The only        *
    *     visible side effect is that addresses 0x4100..0x41FF of any  *
    *     bank that ends up mapped at the 0x4000..0x7FFF window will   *
    *     now read descrambled bytes too, but no real Sachen game ever *
    *     touches its own header through that window.                  *
    *                                                                  *
    *  2. Lock / unlock state. After reset the mapper is "locked":     *
    *     every cartridge read in 0x0000..0x7FFF has its A7 forced     *
    *     high, so the bootstrap ROM that compares logo bytes against  *
    *     0x0104..0x0133 actually sees the bytes stored at             *
    *     0x0184..0x01B3 of the active ROM bank. That second region    *
    *     holds the Sachen logo (descrambled at load), which means     *
    *     the boot ROM passes its Nintendo-logo check while the cart   *
    *     still displays Sachen branding. On real hardware the         *
    *     mapper unlocks after 49 high->low transitions of A15; in     *
    *     gambatte we cannot observe A15 from outside the CPU core,    *
    *     so we approximate by unlocking on the 0xFF50 write that      *
    *     hands control from the bootstrap to the cartridge. This is   *
    *     functionally correct for every known Sachen cart because     *
    *     the boot ROM is the only thing that ever exercises the      *
    *     lock state. When gambatte is configured to run without any   *
    *     bootstrap (the common case for libretro builds) the lock     *
    *     is dropped at load time and the Sachen logo tiles are        *
    *     written directly into VRAM so the game's own copy-protection *
    *     check still finds them.                                      *
    *                                                                  *
    *  3. Banking. Three internal registers control which physical ROM *
    *     banks appear at 0x0000..0x3FFF (the "base" window) and       *
    *     0x4000..0x7FFF (the "switch" window):                        *
    *                                                                  *
    *        base bank      : 0x0000..0x1FFF writes -- gated           *
    *        ROM bank       : 0x2000..0x3FFF writes                    *
    *        ROM bank mask  : 0x4000..0x5FFF writes -- gated           *
    *                                                                  *
    *     The map-enable gate consults bits 4 and 5 of the current     *
    *     ROM-bank value: writes to the base and mask registers only   *
    *     take effect when both bits are set (rom_bank & 0x30 ==       *
    *     0x30). The mapping formula is                                *
    *                                                                  *
    *        base  : (base & mask) | 0                                 *
    *        switch: (base & mask) | (rom_bank & ~mask)                *
    *                                                                  *
    *     and the ROM-bank register is zero-adjusted (a write of 0     *
    *     stores 1) so the base window can never alias the switch.     *
    *                                                                  *
    *  The mapper has no SRAM and no battery; the savestate fields     *
    *  reused below (rombank, rambank, enableRam) carry rom_bank,      *
    *  outerBank, and the lock flag respectively. outerMask and the   *
    *  lock-progress counter get their own dedicated fields           *
    *  (SaveState::Mem::sachenOuterMask, ::sachenLockCount); the      *
    *  latter is needed so a savestate captured mid-bootstrap         *
    *  resumes with the exact read-count progress and the verify     *
    *  pass still unlocks at the right point.                         *
    * ---------------------------------------------------------------- */

   /* Apply the Sachen MMC1 address bit permutation to a 16-bit
    * address. Bits 0,1,4,6 are swapped pairwise (A0<->A6, A1<->A4);
    * all other bits pass through unchanged. The transform is its
    * own inverse. */
   static inline unsigned sachenScramble(unsigned addr) {
      return (addr & ~0x53u)
           | ((addr >> 6) & 0x01u)
           | ((addr >> 3) & 0x02u)
           | ((addr << 3) & 0x10u)
           | ((addr << 6) & 0x40u);
   }

   class Mbc1Sachen : public Mbc {
      MemPtrs &memptrs;
      unsigned char rom_bank;
      unsigned char outerBank;
      unsigned char outerMask;
      bool          locked;
      /* CPU reads of 0x0100..0x01FF observed by Memory::read while
       * the cart is in its locked boot state. The DMG bootstrap's
       * display pass performs exactly 48 such reads (cart 0x0104..
       * 0x0133); when this counter hits 48 the mapper unlocks so
       * the bootstrap's later verify pass sees the unscrambled
       * Nintendo-logo bytes and proceeds to FF50. The counter
       * itself lives here so a single byte address can be shared
       * with Memory at setup time; the public lockCounterPtr()
       * returns it. */
      unsigned char lockCount;
      /* Original (unlocked) bytes at offsets 0x100..0x1FF of physical
       * bank 0, captured after descrambling. The lock overlay writes
       * a permuted view derived from this back into the same ROM
       * region; on unlock we copy these bytes back to restore the
       * descrambled header. */
      unsigned char unlockedBank0Header[0x100];

      unsigned rombankMask() const {
         /* Physical ROM banks present in the loaded image, expressed
          * as a mask suitable for clamping a bank index. The pow2ceil
          * call in loadROM guarantees rombanks() is a power of two. */
         return rombanks(memptrs) - 1;
      }

      void applyBanks() const {
         const unsigned mask = rombankMask();
         memptrs.setRombank0((outerBank & outerMask) & mask);
         memptrs.setRombank(((outerBank & outerMask) | (rom_bank & ~outerMask)) & mask);
      }

   public:
      explicit Mbc1Sachen(MemPtrs &memptrs)
         : memptrs(memptrs),
           rom_bank(1),
           outerBank(0),
           outerMask(0),
           locked(true),
           lockCount(0)
      {
         /* Capture the descrambled header bytes that loadROM has
          * already written to physical bank 0. These are what reads
          * at 0x0100..0x01FF will see once we drop the lock. */
         std::memcpy(unlockedBank0Header,
                     memptrs.romdata() + 0x100,
                     sizeof unlockedBank0Header);
      }

      /* Write a "locked" view of the header to physical bank 0's
       * 0x100..0x1FF window. While locked, every read in
       * 0x0000..0x7FFF has its A7 forced high, so a read of logical
       * 0x104 returns the byte at logical 0x184, etc. We achieve the
       * same effect statically by mirroring the upper 128 bytes of
       * the descrambled header into the lower 128 bytes. The upper
       * 128 bytes (already at offsets 0x180..0x1FF) need no change
       * because A7 is already set there. */
      void installLockOverlay() {
         unsigned char *const dst = memptrs.romdata() + 0x100;
         for (unsigned i = 0; i < 0x80; ++i)
            dst[i] = unlockedBank0Header[i | 0x80];
         /* The upper half is already correct; rewrite it for clarity
          * so a partial load (savestate, soft reset) cannot leave
          * the buffer in an inconsistent state. */
         for (unsigned i = 0x80; i < 0x100; ++i)
            dst[i] = unlockedBank0Header[i];
         locked    = true;
         lockCount = 0;
      }

      void removeLockOverlay() {
         std::memcpy(memptrs.romdata() + 0x100,
                     unlockedBank0Header,
                     sizeof unlockedBank0Header);
         locked = false;
      }

      virtual unsigned char *lockCounterPtr() { return &lockCount; }
      virtual void unlock() {
         if (locked)
            removeLockOverlay();
      }

      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 13) {
            case 0: /* 0x0000..0x1FFF -- base ROM bank, gated */
               if ((rom_bank & 0x30) == 0x30)
                  outerBank = data;
               break;
            case 1: /* 0x2000..0x3FFF -- ROM bank (zero-adjusted) */
               rom_bank = data ? data : 1;
               break;
            case 2: /* 0x4000..0x5FFF -- ROM bank mask, gated */
               if ((rom_bank & 0x30) == 0x30)
                  outerMask = data;
               break;
            default:
               /* 0x6000..0x7FFF and any unmapped range -- ignored. */
               return;
         }
         applyBanks();
      }

      /* No FF50 hook required: the lock state now drops on the
       * 48th cart read in 0x0100..0x01FF, which the DMG bootstrap
       * reaches at the end of its logo display pass, well before
       * the verify pass that needs the unlocked bytes. */

      /* The Sachen MMC1 has no on-cart SRAM; the area
       * 0xA000..0xBFFF reads back open-bus and writes there have no
       * effect. Returning false here keeps Game Genie cheat
       * application limited to the lower 32 KiB. */
      virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
         (void)bank;
         return addr < 0x8000;
      }

      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank          = rom_bank;
         ss.rambank          = outerBank;
         ss.enableRam        = locked;
         ss.sachenOuterMask  = outerMask;
         ss.sachenLockCount  = lockCount;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rom_bank   = ss.rombank ? ss.rombank : 1;
         outerBank  = ss.rambank;
         outerMask  = ss.sachenOuterMask;
         const bool wasLocked = locked;
         const bool wantLocked = ss.enableRam;
         /* Synchronize the in-ROM lock overlay with the restored
          * state. A savestate captured mid-boot (locked=true) will
          * re-install the overlay; one captured post-boot
          * (locked=false) will leave the descrambled bytes in
          * place. The lock-progress counter is preserved across
          * saves so a mid-bootstrap save restores at exactly the
          * same read count and the verify pass unlocks at the
          * right moment. installLockOverlay() resets lockCount to
          * zero, so any preserved value must be restored
          * afterwards. */
         if (wantLocked && !wasLocked) {
            installLockOverlay();
            lockCount = ss.sachenLockCount;
         } else if (!wantLocked && wasLocked) {
            removeLockOverlay();
         } else if (wantLocked) {
            lockCount = ss.sachenLockCount;
         }
         applyBanks();
      }
   };

   class Mbc2 : public DefaultMbc {
      MemPtrs &memptrs;
      unsigned char rombank;
      bool enableRam;
      public:
      explicit Mbc2(MemPtrs &memptrs)
         : memptrs(memptrs),
         rombank(1),
         enableRam(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P & 0x6100) {
            case 0x0000:
               enableRam = (data & 0xF) == 0xA;
               memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
               break;
            case 0x2100:
               rombank = data & 0xF;
               memptrs.setRombank(rombank & (rombanks(memptrs) - 1));
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.enableRam = enableRam;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         enableRam = ss.enableRam;
         memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0, 0);
         memptrs.setRombank(rombank & (rombanks(memptrs) - 1));
      }
   };
   class Mbc3 : public DefaultMbc {
      MemPtrs &memptrs;
      Rtc *const rtc;
      unsigned char rombank;
      unsigned char rambank;
      bool enableRam;
      void setRambank() const {
         unsigned flags = enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0;
         if (rtc) {
            rtc->set(enableRam, rambank);
            if (rtc->getActive())
               flags |= MemPtrs::RTC_EN;
         }
         memptrs.setRambank(flags, rambank & (rambanks(memptrs) - 1));
      }
      public:
      Mbc3(MemPtrs &memptrs, Rtc *const rtc)
         : memptrs(memptrs),
         rtc(rtc),
         rombank(1),
         rambank(0),
         enableRam(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 13 & 3) {
            case 0:
               enableRam = (data & 0xF) == 0xA;
               setRambank();
               break;
            case 1:
               rombank = data & 0x7F;
               setRombank();
               break;
            case 2:
               rambank = data;
               setRambank();
               break;
            case 3:
               if (rtc)
                  rtc->latch(data);
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.rambank = rambank;
         ss.enableRam = enableRam;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         rambank = ss.rambank;
         enableRam = ss.enableRam;
         setRambank();
         setRombank();
      }

      void setRombank() const {
         memptrs.setRombank(std::max(rombank & (rombanks(memptrs) - 1), 1u));
      }
   };
   class HuC1 : public DefaultMbc {
      MemPtrs &memptrs;
      unsigned char rombank;
      unsigned char rambank;
      bool enableRam;
      bool rambankMode;
      void setRambank() const {
         memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : MemPtrs::READ_EN,
               rambankMode ? rambank & (rambanks(memptrs) - 1) : 0);
      }
      void setRombank() const { memptrs.setRombank((rambankMode ? rombank : rambank << 6 | rombank) & (rombanks(memptrs) - 1)); }
      public:
      explicit HuC1(MemPtrs &memptrs)
         : memptrs(memptrs),
         rombank(1),
         rambank(0),
         enableRam(false),
         rambankMode(false)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 13 & 3) {
            case 0:
               enableRam = (data & 0xF) == 0xA;
               setRambank();
               break;
            case 1:
               rombank = data & 0x3F;
               setRombank();
               break;
            case 2:
               rambank = data & 3;
               rambankMode ? setRambank() : setRombank();
               break;
            case 3:
               rambankMode = data & 1;
               setRambank();
               setRombank();
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.rambank = rambank;
         ss.enableRam = enableRam;
         ss.rambankMode = rambankMode;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         rambank = ss.rambank;
         enableRam = ss.enableRam;
         rambankMode = ss.rambankMode;
         setRambank();
         setRombank();
      }
   };
   class HuC3 : public DefaultMbc {
      MemPtrs &memptrs_;
      HuC3Chip *const huc3_;
      unsigned char rombank_;
      unsigned char rambank_;
      unsigned char ramflag_;
      void setRambank() const {
         huc3_->setRamflag(ramflag_);
         unsigned flags;
         if(ramflag_ >= 0x0B && ramflag_ < 0x0F) {
            // System registers mode
            flags = MemPtrs::READ_EN | MemPtrs::WRITE_EN | MemPtrs::RTC_EN;
         }
         else if(ramflag_ == 0x0A || ramflag_ > 0x0D) {
            // Read/write mode
            flags = MemPtrs::READ_EN | MemPtrs::WRITE_EN;
         }
         else {
            // Read-only mode ??
            flags = MemPtrs::READ_EN;
         }
         memptrs_.setRambank(flags, rambank_ & (rambanks(memptrs_) - 1));
      }
      void setRombank() const { memptrs_.setRombank(std::max(rombank_ & (rombanks(memptrs_) - 1), 1u)); }
      public:
      explicit HuC3(MemPtrs &memptrs, HuC3Chip *const huc3)
         : memptrs_(memptrs),
         huc3_(huc3),
         rombank_(1),
         rambank_(0),
         ramflag_(0)
      {
      }
      virtual void romWrite(unsigned const p, unsigned const data) {
         switch (p >> 13 & 3) {
         case 0:
            ramflag_ = data;
            //gambatte_log(RETRO_LOG_DEBUG, "<HuC3> set ramflag to %02X\n", data);
            setRambank();
            break;
         case 1:
            //gambatte_log(RETRO_LOG_DEBUG, "<HuC3> set rombank to %02X\n", data);
            rombank_ = data;
            setRombank();
            break;
         case 2:
            //gambatte_log(RETRO_LOG_DEBUG, "<HuC3> set rambank to %02X\n", data);
            rambank_ = data;
            setRambank();
            break;
         case 3:
            // GEST: "programs will write 1 here"
            break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank_;
         ss.rambank = rambank_;
         ss.HuC3RAMflag = ramflag_;
      }
      virtual void loadState(SaveState::Mem const &ss) {
         rombank_ = ss.rombank;
         rambank_ = ss.rambank;
         ramflag_ = ss.HuC3RAMflag;
         setRambank();
         setRombank();
      }
   };
   class Mbc5 : public DefaultMbc {
      MemPtrs &memptrs;
      unsigned short rombank;
      unsigned char rambank;
      bool enableRam;
      bool hasRumble;
      static unsigned adjustedRombank(const unsigned bank) { return bank; }
      void setRambank() const {
         memptrs.setRambank(enableRam ? MemPtrs::READ_EN | MemPtrs::WRITE_EN : 0,
               rambank & (rambanks(memptrs) - 1));
      }
      void setRombank() const { memptrs.setRombank(adjustedRombank(rombank) & (rombanks(memptrs) - 1));}
      public:
      explicit Mbc5(MemPtrs &memptrs, bool rumble)
         : memptrs(memptrs),
         rombank(1),
         rambank(0),
         enableRam(false),
         hasRumble(rumble)
      {
      }
      virtual void romWrite(const unsigned P, const unsigned data) {
         switch (P >> 12 & 0x7) {
            case 0x0:
            case 0x1:
               enableRam = (data & 0xF) == 0xA;
               setRambank();
               break;
            case 0x2:
            case 0x3:
               rombank = P < 0x3000 ? (rombank & 0x100) | data
                  : (data << 8 & 0x100) | (rombank & 0xFF);
               setRombank();
               break;
            case 0x4:
            case 0x5:
               if(hasRumble && ((P >> 12 & 0x7) == 4))
               {
                  cartridge_set_rumble((data >> 3) & 1);
                  rambank = (data & ~8) & 0x0f;
               }
               else
               {
                  rambank = data & 0x0f;
               }
               setRambank();
               break;
            default:
               break;
         }
      }
      virtual void saveState(SaveState::Mem &ss) const {
         ss.rombank = rombank;
         ss.rambank = rambank;
         ss.enableRam = enableRam;
      }
      virtual void loadState(const SaveState::Mem &ss) {
         rombank = ss.rombank;
         rambank = ss.rambank;
         enableRam = ss.enableRam;
         setRambank();
         setRombank();
      }
   };

   static bool hasRtc(unsigned headerByte0x147)
   {
      switch (headerByte0x147)
      {
         case 0xFE: // huc3
         case 0x0F:
         case 0x10:
            return true;
         default:
            return false;
      }
   }

   void Cartridge::setStatePtrs(SaveState &state)
   {
      state.mem.vram.set(memptrs_.vramdata(), memptrs_.vramdataend() - memptrs_.vramdata());
      state.mem.sram.set(memptrs_.rambankdata(), memptrs_.rambankdataend() - memptrs_.rambankdata());
      state.mem.wram.set(memptrs_.wramdata(0), memptrs_.wramdataend() - memptrs_.wramdata(0));
   }

   void Cartridge::saveState(SaveState &state) const
   {
      mbc->saveState(state.mem);
      rtc_.saveState(state);
      huc3_.saveState(state);
   }

   void Cartridge::loadState(const SaveState &state)
   {
      huc3_.loadState(state);
      rtc_.loadState(state);
      mbc->loadState(state.mem);
   }

   /* Note: a previous helper named enforce8bit() iterated every
    * ROM byte masking with 0xFF, but its enclosing condition
    *   if (static_cast<unsigned char>(0x100))
    * is always false (0x100 truncated to unsigned char is 0), so
    * the body was dead code in every build. Removed. */

   static unsigned pow2ceil(unsigned n)
   {
      --n;
      n |= n >> 1;
      n |= n >> 2;
      n |= n >> 4;
      n |= n >> 8;
      ++n;

      return n;
   }

   /* Sum the bytes that the bootstrap logo check would actually
    * fetch from a locked Sachen MMC1 cartridge across logical
    * addresses 0x0104..0x0133. While locked, the mapper forces A7
    * high, so reads of logical 0x104..0x133 are routed to
    * logical 0x184..0x1B3; the address scrambler then permutes
    * bits 0,1,4,6 of each address before indexing the ROM. The
    * resulting byte stream from a genuine Sachen MMC1 dump always
    * sums to one of two magic constants (SACHEN_LOGO_SUM_A/B),
    * which is what detectSachenMmc1() compares against.
    *
    * Implementation note: the address bit permutation is its own
    * inverse, so applying sachenScramble to 0x184+i yields the
    * physical ROM offset that the mapper would supply when the
    * CPU reads logical 0x104+i in the locked state. */
   static unsigned sachenScrambledLogoSum(const uint8_t *rom) {
      unsigned sum = 0;
      for (unsigned i = 0; i < 0x30; ++i)
         sum += rom[sachenScramble(0x184 + i)];
      return sum;
   }

   /* Magic checksums of the two Sachen logo variants that NewRisingSun
    * identified by scanning known dumps. The 5446 value is the
    * unscrambled Nintendo-logo byte sum, used here only as a negative
    * filter -- if the cart already contains a plain Nintendo logo at
    * 0x0104 it is not a Sachen cart no matter what else looks odd. */
   enum {
      NINTENDO_LOGO_SUM      = 5446,
      SACHEN_LOGO_SUM_A      = 5542,
      SACHEN_LOGO_SUM_B      = 7484
   };

   static bool detectSachenMmc1(const uint8_t *rom, unsigned romsize) {
      if (romsize < 0x200)
         return false;

      unsigned plainSum = 0;
      for (unsigned i = 0; i < 0x30; ++i)
         plainSum += rom[0x104 + i];
      if (plainSum == NINTENDO_LOGO_SUM)
         return false; /* legitimate Nintendo logo present -> not Sachen */

      /* Only the variant whose scrambled logo lives at offset 0x184
       * corresponds to a Sachen *MMC1* dump (the 0x104 variant is
       * MMC2, which this implementation does not handle). */
      if (romsize < 0x1B4)
         return false;
      const unsigned sum184 = sachenScrambledLogoSum(rom);
      return sum184 == SACHEN_LOGO_SUM_A || sum184 == SACHEN_LOGO_SUM_B;
   }

   /* Read one byte from the cartridge image as it would appear at
    * logical CPU address `logicalAddr` once the Sachen MMC1 scramble
    * has been undone. `logicalAddr` must lie in 0x0100..0x01FF. */
   static inline uint8_t sachenReadUnscrambled(const uint8_t *rom,
                                               unsigned logicalAddr) {
      return rom[sachenScramble(logicalAddr)];
   }

   int Cartridge::loadROM(const void *data, unsigned int romsize, unsigned int forceModel, const bool multiCartCompat)
   {
      const uint8_t *romdata = (uint8_t*)data;
      if (romsize < 0x4000 || !romdata)
         return -1;

      unsigned rambanks = 1;
      unsigned rombanks = 2;
      bool cgb = false;
      enum Cartridgetype { PLAIN, MBC1, MBC2, MBC3, MBC5, HUC1, HUC3, SACHEN_MMC1 } type = PLAIN;
      bool rumble = false;

      isSachen_ = detectSachenMmc1(romdata, romsize);

      {
         unsigned char header[0x150];
         /* For a Sachen cart the bytes at offsets 0x0100..0x014F in
          * the raw ROM image are scrambled. Run them through the
          * descrambler before the header switch so the cartridge
          * type, RAM size and CGB-flag bytes read as the publisher
          * intended. The pre-0x0100 region is untouched -- that is
          * the entry vector area and is not subject to scramble. */
         if (isSachen_) {
            std::memcpy(header, romdata, 0x100);
            for (unsigned i = 0x100; i < 0x150; ++i)
               header[i] = sachenReadUnscrambled(romdata, i);
         } else {
            std::memcpy(header, romdata, 0x150);
         }

         if (isSachen_) {
            /* Sachen MMC1 carts have no SRAM, no battery, and are
             * always DMG. Skip the normal header dispatch -- the
             * descrambled byte at 0x0147 carries an unlicensed code
             * that no legitimate switch case would accept anyway. */
            gambatte_log(RETRO_LOG_INFO, "Sachen MMC1 ROM loaded.\n");
            type = SACHEN_MMC1;
         } else
         switch (header[0x0147])
         {
            case 0x00: gambatte_log(RETRO_LOG_INFO, "Plain ROM loaded.\n"); type = PLAIN; break;
            case 0x01: gambatte_log(RETRO_LOG_INFO, "MBC1 ROM loaded.\n"); type = MBC1; break;
            case 0x02: gambatte_log(RETRO_LOG_INFO, "MBC1 ROM+RAM loaded.\n"); type = MBC1; break;
            case 0x03: gambatte_log(RETRO_LOG_INFO, "MBC1 ROM+RAM+BATTERY loaded.\n"); type = MBC1; break;
            case 0x05: gambatte_log(RETRO_LOG_INFO, "MBC2 ROM loaded.\n"); type = MBC2; break;
            case 0x06: gambatte_log(RETRO_LOG_INFO, "MBC2 ROM+BATTERY loaded.\n"); type = MBC2; break;
            case 0x08: gambatte_log(RETRO_LOG_INFO, "Plain ROM with additional RAM loaded.\n"); type = MBC2; break;
            case 0x09: gambatte_log(RETRO_LOG_INFO, "Plain ROM with additional RAM and Battery loaded.\n"); type = MBC2;break;
            case 0x0B: gambatte_log(RETRO_LOG_INFO, "MM01 ROM not supported.\n"); return -1;
            case 0x0C: gambatte_log(RETRO_LOG_INFO, "MM01 ROM not supported.\n"); return -1;
            case 0x0D: gambatte_log(RETRO_LOG_INFO, "MM01 ROM not supported.\n"); return -1;
            case 0x0F: gambatte_log(RETRO_LOG_INFO, "MBC3 ROM+TIMER+BATTERY loaded.\n"); type = MBC3; break;
            case 0x10: gambatte_log(RETRO_LOG_INFO, "MBC3 ROM+TIMER+RAM+BATTERY loaded.\n"); type = MBC3; break;
            case 0x11: gambatte_log(RETRO_LOG_INFO, "MBC3 ROM loaded.\n"); type = MBC3; break;
            case 0x12: gambatte_log(RETRO_LOG_INFO, "MBC3 ROM+RAM loaded.\n"); type = MBC3; break;
            case 0x13: gambatte_log(RETRO_LOG_INFO, "MBC3 ROM+RAM+BATTERY loaded.\n"); type = MBC3; break;
            case 0x15: gambatte_log(RETRO_LOG_INFO, "MBC4 ROM not supported.\n"); return -1;
            case 0x16: gambatte_log(RETRO_LOG_INFO, "MBC4 ROM not supported.\n"); return -1;
            case 0x17: gambatte_log(RETRO_LOG_INFO, "MBC4 ROM not supported.\n"); return -1;
            case 0x19: gambatte_log(RETRO_LOG_INFO, "MBC5 ROM loaded.\n"); type = MBC5; break;
            case 0x1A: gambatte_log(RETRO_LOG_INFO, "MBC5 ROM+RAM loaded.\n"); type = MBC5; break;
            case 0x1B: gambatte_log(RETRO_LOG_INFO, "MBC5 ROM+RAM+BATTERY loaded.\n"); type = MBC5; break;
            case 0x1C: gambatte_log(RETRO_LOG_INFO, "MBC5+RUMBLE ROM loaded.\n"); type = MBC5; rumble = true; break;
            case 0x1D: gambatte_log(RETRO_LOG_INFO, "MBC5+RUMBLE+RAM ROM loaded.\n"); type = MBC5; rumble = true; break;
            case 0x1E: gambatte_log(RETRO_LOG_INFO, "MBC5+RUMBLE+RAM+BATTERY ROM loaded.\n"); type = MBC5; rumble = true; break;
            case 0x20: gambatte_log(RETRO_LOG_INFO, "MBC6 ROM not supported.\n"); return -1;
            case 0x22: gambatte_log(RETRO_LOG_INFO, "MBC7 ROM not supported.\n"); return -1;
	     case 0xEA: type = MBC1; break; // Hack to accept 0xEA as a MBC1 (Sonic 3D Blast 5)
            case 0xFC: gambatte_log(RETRO_LOG_INFO, "Pocket Camera ROM not supported.\n"); return -1;
            case 0xFD: gambatte_log(RETRO_LOG_INFO, "Bandai TAMA5 ROM not supported.\n"); return -1;
            case 0xFE: gambatte_log(RETRO_LOG_INFO, "HuC3 ROM+RAM+BATTERY loaded.\n"); type = HUC3; break;
            case 0xFF: gambatte_log(RETRO_LOG_INFO, "HuC1 ROM+BATTERY loaded.\n"); type = HUC1; break;
            default: gambatte_log(RETRO_LOG_INFO, "Wrong data-format, corrupt or unsupported ROM.\n"); return -1;
         }

         switch (header[0x0149])
         {
               case 0x00: /*std::puts("No RAM");*/ rambanks = type == MBC2; break;
               case 0x01: /*std::puts("2kB RAM");*/ /*rambankrom=1; break;*/
               case 0x02: /*std::puts("8kB RAM");*/
                  rambanks = 1;
                  break;
               case 0x03: /*std::puts("32kB RAM");*/
                  rambanks = 4;
                  break;
               case 0x04: /*std::puts("128kB RAM");*/
                  rambanks = 16;
                  break;
               case 0x05: /*std::puts("undocumented kB RAM");*/
                  rambanks = 16;
                  break;
               default: /*std::puts("Wrong data-format, corrupt or unsupported ROM loaded.");*/
                  rambanks = 16;
                  break;
         }

         /* Sachen MMC1 has no SRAM regardless of what the descrambled
          * header byte at 0x0149 claims; override to avoid allocating
          * useless save memory. */
         if (type == SACHEN_MMC1)
            rambanks = 0;

         switch(forceModel){
            case 1://FORCE_DMG
               cgb = false;
               break;
            case 8://FORCE_CGB
               cgb = true;
               break;
            case 0://dont force anything
               cgb = header[0x0143] >> 7;
         }

         /* Sachen cartridges are mono-only DMG units; if a user
          * forces CGB mode keep the request, but never auto-detect
          * CGB from the (scrambled-then-unscrambled) flag byte. */
         if (type == SACHEN_MMC1 && forceModel == 0)
            cgb = false;
      }

      gambatte_log(RETRO_LOG_INFO, "rambanks: %u\n", rambanks);

      rombanks = pow2ceil(romsize / 0x4000);
      gambatte_log(RETRO_LOG_INFO, "rombanks: %u\n", static_cast<unsigned>(romsize / 0x4000));

      ggUndoList_.clear();
      mbc.reset();
      memptrs_.reset(rombanks, rambanks, cgb ? 8 : 2);
      rtc_.set(false, 0);
      huc3_.set(false);

      memcpy(memptrs_.romdata(), romdata, ((romsize / 0x4000) * 0x4000ul) * sizeof(unsigned char));
      std::memset(memptrs_.romdata() + (romsize / 0x4000) * 0x4000ul, 0xFF, (rombanks - romsize / 0x4000) * 0x4000ul);

      /* Pre-descramble every 16 KiB bank's 0x100..0x1FF window so the
       * scrambled header region reads correctly through the normal
       * MemPtrs fast path. The bit permutation is its own inverse, so
       * a 256-byte in-place pass that swaps each pair of permuted
       * offsets exactly once does the job; we iterate over the
       * lexicographically smaller half and skip the fixed points to
       * avoid double-swapping. */
      if (type == SACHEN_MMC1) {
         const unsigned filledBanks = romsize / 0x4000;
         for (unsigned bank = 0; bank < filledBanks; ++bank) {
            unsigned char *const window = memptrs_.romdata() + bank * 0x4000ul + 0x100;
            for (unsigned i = 0; i < 0x100; ++i) {
               const unsigned j = sachenScramble(0x100 + i) - 0x100;
               if (j > i) {
                  const unsigned char tmp = window[i];
                  window[i] = window[j];
                  window[j] = tmp;
               }
            }
         }
      }

      switch (type)
      {
         case PLAIN: mbc.reset(new Mbc0(memptrs_)); break;
         case MBC1:
                     if (!rambanks && rombanks == 64 && multiCartCompat) {
                        /*std::puts("Multi-ROM \"MBC1\" presumed");*/
                        mbc.reset(new Mbc1Multi64(memptrs_));
                     } else
                        mbc.reset(new Mbc1(memptrs_));
                     break;
         case MBC2: mbc.reset(new Mbc2(memptrs_)); break;
         case MBC3: mbc.reset(new Mbc3(memptrs_, hasRtc(memptrs_.romdata()[0x147]) ? &rtc_ : 0)); break;
         case MBC5: mbc.reset(new Mbc5(memptrs_, rumble)); break;
         case HUC1: mbc.reset(new HuC1(memptrs_)); break;
         case HUC3:
            huc3_.set(true);
            mbc.reset(new HuC3(memptrs_, &huc3_));
            break;
         case SACHEN_MMC1:
            mbc.reset(new Mbc1Sachen(memptrs_));
            break;
      }

      return 0;
   }

   static int asHex(const char c)
   {
      /* Accept both uppercase and lowercase A-F (the previous
       * implementation silently produced wrong values for
       * lowercase letters). */
      if (c >= 'a' && c <= 'f') return c - 'a' + 0xA;
      if (c >= 'A' && c <= 'F') return c - 'A' + 0xA;
      return c - '0';
   }

   void Cartridge::applyGameGenie(const std::string &code)
   {
      if (6 < code.length())
      {
         const unsigned val = (asHex(code[0]) << 4 | asHex(code[1])) & 0xFF;
         const unsigned addr = (asHex(code[2]) << 8 | asHex(code[4]) << 4 | asHex(code[5]) | (asHex(code[6]) ^ 0xF) << 12) & 0x7FFF;
         unsigned cmp = 0xFFFF;

         if (10 < code.length())
         {
            cmp = (asHex(code[8]) << 4 | asHex(code[10])) ^ 0xFF;
            cmp = ((cmp >> 2 | cmp << 6) ^ 0x45) & 0xFF;
         }

         for (unsigned bank = 0; bank < static_cast<std::size_t>(memptrs_.romdataend() - memptrs_.romdata()) / 0x4000; ++bank)
         {
            if (mbc->isAddressWithinAreaRombankCanBeMappedTo(addr, bank)
                  && (cmp > 0xFF || memptrs_.romdata()[bank * 0x4000ul + (addr & 0x3FFF)] == cmp))
            {
               ggUndoList_.push_back(AddrData(bank * 0x4000ul + (addr & 0x3FFF), memptrs_.romdata()[bank * 0x4000ul + (addr & 0x3FFF)]));
               memptrs_.romdata()[bank * 0x4000ul + (addr & 0x3FFF)] = val;
            }
         }
      }
   }

   void Cartridge::setGameGenie(const std::string &codes)
   {
#if 0
      if (loaded())
#endif
      {
         std::string code;
         for (std::size_t pos = 0; pos < codes.length()
               && (code = codes.substr(pos, codes.find(';', pos) - pos), true); pos += code.length() + 1)
            applyGameGenie(code);
      }
   }

   void Cartridge::clearCheats()
   {
       for (std::vector<AddrData>::reverse_iterator it = ggUndoList_.rbegin(), end = ggUndoList_.rend(); it != end; ++it)
          {
             if (memptrs_.romdata() + it->addr < memptrs_.romdataend())
                memptrs_.romdata()[it->addr] = it->data;
          }

       ggUndoList_.clear();
   }

   /* Inject the Sachen logo into VRAM so the cartridge's
    * self-protection check finds it even when running without a
    * Game Boy boot ROM. The expansion below replicates the boot
    * ROM's logic: every source byte represents 8 horizontal pixels,
    * each pixel doubling vertically and horizontally into a 2x2 tile
    * block. The boot ROM writes interleaved planes 0 and 1 to
    * 0x8010..0x81... for the Nintendo logo; we do the same with the
    * Sachen logo bytes that the mapper would have presented through
    * the lock overlay.
    *
    * `srcBank0` must point at the descrambled physical bank 0 (i.e.
    * memptrs_.romdata() right after loadROM has descrambled the
    * 0x100..0x1FF window). The 48 logo bytes live at offsets
    * 0x184..0x1B3 of that bank -- this is the "lock-view" of
    * 0x104..0x133, since A7|=1 maps the latter to the former.
    *
    * Logic ported from NewRisingSun's hhugboy MbcUnlSachenMMC1.cpp,
    * CC0-licensed. */
   static void injectSachenVramLogo(unsigned char *vram,
                                    const unsigned char *srcBank0) {
      /* setInitialVram() in initstate.cpp pre-populates VRAM with the
       * Nintendo logo tile data plus a trailing trademark-R tile so
       * that boot-ROM-less DMG runs display a plausible power-on
       * screen. For a Sachen cart we must replace the logo bytes
       * with the Sachen-logo expansion, and also blank out the R
       * tile that sits just after the logo region (offsets
       * 0x190..0x1A7) so the Sachen logo does not get a stray
       * Nintendo trademark drawn next to it. */
      std::memset(vram + 0x190, 0, 0x1A8 - 0x190);

      for (unsigned i = 0; i < 0x30; ++i) {
         const unsigned char logoByte = srcBank0[0x184 + i];
         const unsigned char b1 =
              ((logoByte >> 0) & 0x80) | ((logoByte >> 1) & 0x40)
            | ((logoByte >> 1) & 0x20) | ((logoByte >> 2) & 0x10)
            | ((logoByte >> 2) & 0x08) | ((logoByte >> 3) & 0x04)
            | ((logoByte >> 3) & 0x02) | ((logoByte >> 4) & 0x01);
         const unsigned char b2 =
              ((logoByte << 4) & 0x80) | ((logoByte << 3) & 0x40)
            | ((logoByte << 3) & 0x20) | ((logoByte << 2) & 0x10)
            | ((logoByte << 2) & 0x08) | ((logoByte << 1) & 0x04)
            | ((logoByte << 1) & 0x02) | ((logoByte << 0) & 0x01);
         /* Boot-ROM logo tile data starts at VRAM offset 0x10 (i.e.
          * logical 0x8010). Each source byte occupies 8 destination
          * bytes: planes 0 and 1 of two adjacent 2-row tile pairs. */
         const unsigned base = 0x10 + i * 8;
         vram[base + 0] = b1;
         vram[base + 2] = b1;
         vram[base + 4] = b2;
         vram[base + 6] = b2;
         vram[base + 1] = 0;
         vram[base + 3] = 0;
         vram[base + 5] = 0;
         vram[base + 7] = 0;
      }
   }

   void Cartridge::sachenLockSetup(bool bootloaderUsed)
   {
      if (!isSachen_ || !mbc.get())
         return;

      Mbc1Sachen *const sachen = static_cast<Mbc1Sachen*>(mbc.get());

      if (bootloaderUsed) {
         /* The bootstrap is going to scan the logo at 0x0104..0x0133
          * during its display pass; present the locked view so VRAM
          * receives the Sachen-logo expansion. The mapper will then
          * unlock after the 48th cart read in 0x0100..0x01FF (the
          * count is bumped by Memory::read and lands precisely at
          * the end of the bootstrap's display pass), so the verify
          * pass that immediately follows sees the unscrambled
          * Nintendo-logo bytes and proceeds to FF50. */
         sachen->installLockOverlay();
      } else {
         /* No bootstrap is going to run. The cartridge itself will
          * verify the logo in VRAM as a self-test, so we inject the
          * Sachen logo there directly and drop straight into the
          * unlocked state. The descrambled header is already in
          * place from loadROM(). */
         injectSachenVramLogo(memptrs_.vramdata(), memptrs_.romdata());
         sachen->unlock();
      }
   }

   /* Memory wires its read-count-and-trigger machinery to these two
    * methods; both indirect through the loaded MBC so non-Sachen
    * carts return null/no-op and Memory's hot read path stays a
    * single null-check branch. */
   unsigned char *Cartridge::sachenLockCounterPtr() {
      return mbc.get() ? mbc->lockCounterPtr() : 0;
   }

   void Cartridge::onSachenUnlock() {
      if (mbc.get())
         mbc->unlock();
   }

}

