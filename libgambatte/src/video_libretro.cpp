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
