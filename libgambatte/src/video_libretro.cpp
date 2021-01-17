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

/* GBC colour correction factors */
#define GBC_CC_LUM 0.94f
#define GBC_CC_R   0.82f
#define GBC_CC_G   0.665f
#define GBC_CC_B   0.73f
#define GBC_CC_RG  0.125f
#define GBC_CC_RB  0.195f
#define GBC_CC_GR  0.24f
#define GBC_CC_GB  0.075f
#define GBC_CC_BR  -0.06f
#define GBC_CC_BG  0.21f

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
   
   void LCD::setColorCorrectionBrightness(float colorCorrectionBrightness_)
   {
      colorCorrectionBrightness = colorCorrectionBrightness_;
      refreshPalettes();
   }
   
   void LCD::setDarkFilterLevel(unsigned darkFilterLevel_)
   {
      darkFilterLevel = darkFilterLevel_;
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
#elif defined(VIDEO_ABGR1555)
         uint16_t dmgColors[4]={0xEFFF, //11111 111111 11111
            0x56B5, //10101 10101 10101
            0x294A, //01010 01010 01010
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

   // RGB range: [0,1]
   void LCD::darkenRgb(float &r, float &g, float &b)
   {
      // Note: This is *very* approximate...
      // - Should be done in linear colour space. It isn't.
      // - Should alter brightness by performing an RGB->HSL->RGB
      //   conversion. We just do simple linear scaling instead.
      // Basically, this is intended for use on devices that are
      // too weak to run shaders (i.e. why would you want a 'dark filter'
      // if your device supports proper LCD shaders?). We therefore
      // cut corners for the sake of performance...
      //
      // Constants
      // > Luminosity factors: photometric/digital ITU BT.709
      static const float lumaR = 0.2126f;
      static const float lumaG = 0.7152f;
      static const float lumaB = 0.0722f;
      // Calculate luminosity
      float luma = (lumaR * r) + (lumaG * g) + (lumaB * b);
      // Get 'darkness' scaling factor
      // > User set 'dark filter' level scaled by current luminosity
      //   (i.e. lighter colours affected more than darker colours)
      float darkFactor = 1.0f - ((static_cast<float>(darkFilterLevel) * 0.01f) * luma);
      darkFactor = darkFactor < 0.0f ? 0.0f : darkFactor;
      // Perform scaling...
      r = r * darkFactor;
      g = g * darkFactor;
      b = b * darkFactor;
   }

   video_pixel_t LCD::gbcToRgb32(const unsigned bgr15)
   {
      const unsigned r = bgr15       & 0x1F;
      const unsigned g = bgr15 >>  5 & 0x1F;
      const unsigned b = bgr15 >> 10 & 0x1F;
      
      unsigned rFinal = 0;
      unsigned gFinal = 0;
      unsigned bFinal = 0;
      
      // Constants
      // (Don't know whether floating-point rounding modes can be changed
      // dynamically at run time, so can't rely on constant folding of
      // inline [float / float] expressions - just play it safe...)
      static const float rgbMax = 31.0;
      static const float rgbMaxInv = 1.0 / rgbMax;
      
      bool isDark = false;
      
      if (colorCorrection)
      {
         if (colorCorrectionMode == 1)
         {
            // Use fast (inaccurate) Gambatte default method
            rFinal = ((r * 13) + (g * 2) + b) >> 4;
            gFinal = ((g * 3) + b) >> 2;
            bFinal = ((r * 3) + (g * 2) + (b * 11)) >> 4;
         }
         else
         {
            // Use Pokefan531's "gold standard" GBC colour correction
            // (https://forums.libretro.com/t/real-gba-and-ds-phat-colors/1540/190)
            // NB: The results produced by this implementation are ever so slightly
            // different from the output of the gbc-colour shader. This is due to the
            // fact that we have to tolerate rounding errors here that are simply not
            // an issue when tweaking the final image with a post-processing shader.
            // *However:* the difference is so tiny small that 99.9% of users will
            // never notice, and the result is still 100x better than the 'fast'
            // colour correction method.
            //
            // Constants
            static const float targetGamma = 2.2;
            static const float displayGammaInv = 1.0 / targetGamma;
            // Perform gamma expansion
            float adjustedGamma = targetGamma - colorCorrectionBrightness;
            float rFloat = std::pow(static_cast<float>(r) * rgbMaxInv, adjustedGamma);
            float gFloat = std::pow(static_cast<float>(g) * rgbMaxInv, adjustedGamma);
            float bFloat = std::pow(static_cast<float>(b) * rgbMaxInv, adjustedGamma);
            // Perform colour mangling
            float rCorrect = GBC_CC_LUM * ((GBC_CC_R  * rFloat) + (GBC_CC_GR * gFloat) + (GBC_CC_BR * bFloat));
            float gCorrect = GBC_CC_LUM * ((GBC_CC_RG * rFloat) + (GBC_CC_G  * gFloat) + (GBC_CC_BG * bFloat));
            float bCorrect = GBC_CC_LUM * ((GBC_CC_RB * rFloat) + (GBC_CC_GB * gFloat) + (GBC_CC_B  * bFloat));
            // Range check...
            rCorrect = rCorrect > 0.0f ? rCorrect : 0.0f;
            gCorrect = gCorrect > 0.0f ? gCorrect : 0.0f;
            bCorrect = bCorrect > 0.0f ? bCorrect : 0.0f;
            // Perform gamma compression
            rCorrect = std::pow(rCorrect, displayGammaInv);
            gCorrect = std::pow(gCorrect, displayGammaInv);
            bCorrect = std::pow(bCorrect, displayGammaInv);
            // Range check...
            rCorrect = rCorrect > 1.0f ? 1.0f : rCorrect;
            gCorrect = gCorrect > 1.0f ? 1.0f : gCorrect;
            bCorrect = bCorrect > 1.0f ? 1.0f : bCorrect;
            // Perform image darkening, if required
            if (darkFilterLevel > 0)
            {
               darkenRgb(rCorrect, gCorrect, bCorrect);
               isDark = true;
            }
            // Convert back to 5bit unsigned
            rFinal = static_cast<unsigned>((rCorrect * rgbMax) + 0.5) & 0x1F;
            gFinal = static_cast<unsigned>((gCorrect * rgbMax) + 0.5) & 0x1F;
            bFinal = static_cast<unsigned>((bCorrect * rgbMax) + 0.5) & 0x1F;
         }
      }
      else
      {
         rFinal = r;
         gFinal = g;
         bFinal = b;
      }
      
      // Perform image darkening, if required and we haven't
      // already done it during colour correction
      if (darkFilterLevel > 0 && !isDark)
      {
         // Convert colour range from [0,0x1F] to [0,1]
         float rDark = static_cast<float>(rFinal) * rgbMaxInv;
         float gDark = static_cast<float>(gFinal) * rgbMaxInv;
         float bDark = static_cast<float>(bFinal) * rgbMaxInv;
         // Perform image darkening
         darkenRgb(rDark, gDark, bDark);
         // Convert back to 5bit unsigned
         rFinal = static_cast<unsigned>((rDark * rgbMax) + 0.5) & 0x1F;
         gFinal = static_cast<unsigned>((gDark * rgbMax) + 0.5) & 0x1F;
         bFinal = static_cast<unsigned>((bDark * rgbMax) + 0.5) & 0x1F;
      }
      
#ifdef VIDEO_RGB565
      return rFinal << 11 | gFinal << 6 | bFinal;
#elif defined(VIDEO_ABGR1555)
      return bFinal << 10 | gFinal << 5 | rFinal;
#else
      return rFinal << 16 | gFinal << 8 | bFinal;
#endif
   }

}
