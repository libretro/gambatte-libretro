#ifndef CALENDAR_CLOCK_H
#define CALENDAR_CLOCK_H

#include "../savestate.h"
#include <ctime>
#include <stdint.h>

namespace gambatte
{

   /* The calendar time the RTC chips read. Until a frontend sets it, it is
    * the host clock; once set, it advances by emulated time. */
   class CalendarClock
   {
      public:
         enum { samples_per_sec = 2097152 };

         CalendarClock()
            : sec_(0),
            samples_(0),
            set_(false),
            running_(true)
         {
         }

         uint64_t now() const
         {
            return set_ ? sec_ : std::time(0);
         }

         void set(const uint64_t sec, const unsigned long samples)
         {
            sec_     = sec;
            samples_ = samples;
            set_     = true;
         }

         void clear()
         {
            set_     = false;
            running_ = true;
         }

         void setRunning(const bool running)
         {
            running_ = running;
         }

         void advance(const unsigned long samples)
         {
            if (!set_ || !running_)
               return;

            samples_ += samples;
            sec_     += samples_ / samples_per_sec;
            samples_ %= samples_per_sec;
         }

         void saveState(SaveState &state) const
         {
            state.calendar.sec     = set_ ? sec_ : 0;
            state.calendar.samples = set_ ? samples_ : 0;
         }

         void loadState(const SaveState &state)
         {
            if (state.calendar.sec)
               set(state.calendar.sec, state.calendar.samples);
         }

      private:
         uint64_t sec_;
         unsigned long samples_;
         bool set_;
         bool running_;
   };

}

#endif
