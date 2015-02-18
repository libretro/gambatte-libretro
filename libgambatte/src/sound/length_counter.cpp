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
#include "length_counter.h"
#include "master_disabler.h"
#include <algorithm>

namespace gambatte
{

   LengthCounter::LengthCounter(MasterDisabler &disabler, const unsigned mask)
      : disableMaster_(disabler)
      , lengthCounter_(0)
      , lengthMask_(mask)
   {
      nr1Change(0, 0, 0);
   }

   void LengthCounter::event()
   {
      counter_ = COUNTER_DISABLED;
      lengthCounter_ = 0;
      disableMaster_();
   }

   void LengthCounter::nr1Change(const unsigned newNr1, const unsigned nr4, const unsigned long cc)
   {
      lengthCounter_ = (~newNr1 & lengthMask_) + 1;
      counter_ = (nr4 & 0x40) 
         ?( (cc >> 13) + lengthCounter_) << 13 
         : static_cast<unsigned long>(COUNTER_DISABLED);
   }

   void LengthCounter::nr4Change(const unsigned oldNr4, const unsigned newNr4, const unsigned long cc)
   {
      unsigned dec = 0;

      if (counter_ != COUNTER_DISABLED)
         lengthCounter_ = (counter_ >> 13) - (cc >> 13);

      if (newNr4 & 0x40)
      {
         dec = ~cc >> 12 & 1;

         if (!(oldNr4 & 0x40) && lengthCounter_)
         {
            if (!(lengthCounter_ -= dec))
               disableMaster_();
         }
      }

      if ((newNr4 & 0x80) && !lengthCounter_)
         lengthCounter_ = lengthMask_ + 1 - dec;

      if ((newNr4 & 0x40) && lengthCounter_)
         counter_ = ((cc >> 13) + lengthCounter_) << 13;
      else
         counter_ = COUNTER_DISABLED;
   }

   void LengthCounter::saveState(SaveState::SPU::LCounter &lstate) const
   {
      lstate.counter = counter_;
      lstate.lengthCounter = lengthCounter_;
   }

   void LengthCounter::loadState(const SaveState::SPU::LCounter &lstate, const unsigned long cc)
   {
      counter_ = std::max(lstate.counter, cc);
      lengthCounter_ = lstate.lengthCounter;
   }

}
