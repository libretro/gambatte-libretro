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
#include <cmath>

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

   void LCD::setColorCorrection(bool colorCorrection_)
   {
      colorCorrection = colorCorrection_;
      refreshPalettes();
   }
   
   void LCD::setColorCorrectionMode(unsigned colorCorrectionMode_)
   {
      // Note: We only have two modes, so could make 'colorCorrectionMode'
      // a bool... but may want to add other modes in the future
      // (e.g. a special GBA colour correction mode for when the GBA
      // flag is set in Shantae/Zelda: Oracle/etc.)
      colorCorrectionMode = colorCorrectionMode_;
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

      reset(oamram, vram, false);
      setVideoBuffer(0, 160);

      setColorCorrection(true);
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
      const unsigned r = bgr15       & 0x1F;
      const unsigned g = bgr15 >>  5 & 0x1F;
      const unsigned b = bgr15 >> 10 & 0x1F;
      
      if (colorCorrection)
      {
         if (colorCorrectionMode == 1)
         {
            // Use fast (inaccurate) Gambatte default method
#ifdef VIDEO_RGB565
            return (((r * 13 + g * 2 + b + 8) << 7) & 0xF800) | ((g * 3 + b + 1) >> 1) << 5 | ((r * 3 + g * 2 + b * 11 + 8) >> 4);
#else
            return ((r * 13 + g * 2 + b) >> 1) << 16 | (g * 3 + b) << 9 | (r * 3 + g * 2 + b * 11) >> 1;
#endif
         }
         else
         {
            // Use Pokefan531's "gold standard" GBC colour correction
            // (https://forums.libretro.com/t/real-gba-and-ds-phat-colors/1540/159)
            // NB: The results produced by this implementation are ever so slightly
            // different from the output of the gbc-colour shader. This is due to the
            // fact that we have to tolerate rounding errors here that are simply not
            // an issue when tweaking the final image with a post-processing shader.
            // *However:* the difference is so tiny small that 99.9% of users will
            // never notice, and the result is still 100x better than the 'fast'
            // colour correction method.
            //
            // Constants
            // (Don't know whether floating-point rounding modes can be changed
            // dynamically at run time, so can't rely on constant folding of
            // inline [float / float] expressions - just play it safe...)
            const float targetGamma = 2.2;
            const float displayGammaInv = 1.0 / targetGamma;
            const float rgbMax = 31.0;
            const float rgbMaxInv = 1.0 / rgbMax;
            // Perform gamma expansion
            float rCorrect = std::pow(static_cast<float>(r) * rgbMaxInv, targetGamma);
            float gCorrect = std::pow(static_cast<float>(g) * rgbMaxInv, targetGamma);
            float bCorrect = std::pow(static_cast<float>(b) * rgbMaxInv, targetGamma);
            // Perform colour mangling (lots of magic numbers...)
            rCorrect = (0.86629 * rCorrect) + (0.13361 * gCorrect);
            gCorrect = (0.02429 * rCorrect) + (0.70857 * gCorrect) + (0.26714 * bCorrect);
            bCorrect = (0.11337 * rCorrect) + (0.11448 * gCorrect) + (0.77215 * bCorrect);
            // Perform gamma compression
            rCorrect = rgbMax * std::pow(rCorrect, displayGammaInv);
            gCorrect = rgbMax * std::pow(gCorrect, displayGammaInv);
            bCorrect = rgbMax * std::pow(bCorrect, displayGammaInv);
            // Convert back to 5bit unsigned
            unsigned rFinal = static_cast<unsigned>(rCorrect + 0.5) & 0x1F;
            unsigned gFinal = static_cast<unsigned>(gCorrect + 0.5) & 0x1F;
            unsigned bFinal = static_cast<unsigned>(bCorrect + 0.5) & 0x1F;
            
#ifdef VIDEO_RGB565
            return rFinal<<11 | gFinal<<6 | bFinal;
#else
            return rFinal<<16 | gFinal<<8 | bFinal;
#endif
         }
      }
      else
      {
#ifdef VIDEO_RGB565
         return r<<11 | g<<6 | b;
#else
         return r<<16 | g<<8 | b;
#endif
      }
   }

}
