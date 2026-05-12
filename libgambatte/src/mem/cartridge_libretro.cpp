#include "cartridge.h"
#include "../savestate.h"
#include <cstring>
#include <fstream>

#include <libretro.h>

namespace gambatte
{
   static bool hasBattery(unsigned char headerByte0x147)
   {
      switch (headerByte0x147)
      {
         case 0x03:
         case 0x06:
         case 0x09:
         case 0x0F:
         case 0x10:
         case 0x13:
         case 0x1B:
         case 0x1E:
         case 0xFE:
         case 0xFF:
            return true;
         default:
            return false;
      }
   }

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

   void *Cartridge::savedata_ptr()
   {
      /* Sachen MMC1 carts have no SRAM, and the byte at 0x0147 is
       * the descrambled cartridge type which has no Nintendo
       * meaning anyway -- skip the hasBattery() check entirely so
       * we never hand the frontend a bogus save pointer. */
      if (isSachen_)
         return 0;
      // Check ROM header for battery.
      if (hasBattery(memptrs_.romdata()[0x147]))
         return memptrs_.rambankdata();
      return 0;
   }

   unsigned Cartridge::savedata_size()
   {
      if (isSachen_)
         return 0;
      if (hasBattery(memptrs_.romdata()[0x147]))
         return memptrs_.rambankdataend() - memptrs_.rambankdata();
      return 0;
   }

   void *Cartridge::rtcdata_ptr()
   {
      if (isSachen_)
         return 0;
      if (hasRtc(memptrs_.romdata()[0x147])) {
         if (isHuC3()) {
            return &huc3_.getBaseTime();
         } else {
            return &rtc_.getBaseTime();
         }
      }
      return 0;
   }

   unsigned Cartridge::rtcdata_size()
   { 
      if (isSachen_)
         return 0;
      if (hasRtc(memptrs_.romdata()[0x147])) {
         if (isHuC3()) {
            return sizeof(huc3_.getBaseTime());
         } else {
            return sizeof(rtc_.getBaseTime());
         }
      }
      return 0;
   }

}
