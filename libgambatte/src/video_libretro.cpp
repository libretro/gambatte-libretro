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
   void LCD::setDmgPaletteColor(const unsigned index, const video_pixel_t rgb32)
   {
      dmgColorsRgb32_[index] = rgb32;
   }

   void LCD::setDmgPalette(video_pixel_t *const palette, const video_pixel_t *const dmgColors, const unsigned data)
   {
      palette[0] = dmgColors[data      & 3];
      palette[1] = dmgColors[data >> 2 & 3];
      palette[2] = dmgColors[data >> 4 & 3];
      palette[3] = dmgColors[data >> 6 & 3];
   }

   void LCD::doCgbColorChange(unsigned char *const pdata,
         video_pixel_t *const palette, unsigned index, const unsigned data)
   {
      pdata[index] = data;
      index >>= 1;
      palette[index] = gbcToRgb32(pdata[index << 1] | pdata[(index << 1) + 1] << 8);
   }

   void LCD::setVideoBuffer(video_pixel_t *const videoBuf, const int pitch)
   {
      ppu_.setFrameBuf(videoBuf, pitch);
   }

   static void clear(video_pixel_t *buf, const unsigned long color, const int dpitch)
   {
      unsigned lines = 144;

      while (lines--)
      {
         std::fill_n(buf, 160, color);
         buf += dpitch;
      }
   }

   void LCD::setDmgPaletteColor(const unsigned palNum, const unsigned colorNum, const video_pixel_t rgb32)
   {
      if (palNum > 2 || colorNum > 3)
         return;

      setDmgPaletteColor(palNum * 4 | colorNum, rgb32);
      refreshPalettes();
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

   video_pixel_t LCD::gbcToRgb32(const unsigned bgr15)
   {
      if (colorCorrection)
      {
#ifdef VIDEO_RGB565
         const unsigned r = bgr15 & 0x1F;
         const unsigned g = bgr15 >> 5 & 0x1F;
         const unsigned b = bgr15 >> 10 & 0x1F;

         return (((r * 13 + g * 2 + b + 8) << 7) & 0xF800) | ((g * 3 + b + 1) >> 1) << 5 | ((r * 3 + g * 2 + b * 11 + 8) >> 4);
#else
         const unsigned r = bgr15       & 0x1F;
         const unsigned g = bgr15 >>  5 & 0x1F;
         const unsigned b = bgr15 >> 10 & 0x1F;

         return ((r * 13 + g * 2 + b) >> 1) << 16 | (g * 3 + b) << 9 | (r * 3 + g * 2 + b * 11) >> 1;
#endif
      }
      else
      {
#ifdef VIDEO_RGB565
         const unsigned r = bgr15       & 0x1F;
         const unsigned g = bgr15 >>  5 & 0x1F;
         const unsigned b = bgr15 >> 10 & 0x1F;

         return r<<11 | g<<6 | b;
#else
         const unsigned r = bgr15       & 0x1F;
         const unsigned g = bgr15 >>  5 & 0x1F;
         const unsigned b = bgr15 >> 10 & 0x1F;

         return r<<16 | g<<8 | b;
#endif
      }
   }

}
