//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "huc3.h"
#include "../savestate.h"
#include <stdio.h>

namespace gambatte {

HuC3Chip::HuC3Chip()
: baseTime_(0)
, haltTime_(0)
, dataTime_(0)
, writingTime_(0)
, ramValue_(0)
, shift_(0)
, ramflag_(0)
, modeflag_(HUC3_NONE)
, irBaseCycle_(0)
, enabled_(false)
, halted_(false)
, irReceivingPulse_(false)
{
}

void HuC3Chip::doLatch() {
	uint64_t tmp = (halted_ ? haltTime_ : std::time(0)) - baseTime_;
    
    unsigned minute = (tmp / 60) % 1440;
    unsigned day = (tmp / 86400) & 0xFFF;
    dataTime_ = (day << 12) | minute;
}

void HuC3Chip::saveState(SaveState &state) const {
	state.huc3.baseTime = baseTime_;
	state.huc3.haltTime = haltTime_;
    state.huc3.dataTime = dataTime_;
    state.huc3.writingTime = writingTime_;
    state.huc3.ramValue = ramValue_;
    state.huc3.shift = shift_;
    state.huc3.halted = halted_;
    state.huc3.modeflag = modeflag_;
    state.huc3.irBaseCycle = irBaseCycle_;
    state.huc3.irReceivingPulse = irReceivingPulse_;
}

void HuC3Chip::loadState(SaveState const &state) {
	baseTime_ = state.huc3.baseTime;
	haltTime_ = state.huc3.haltTime;
    dataTime_ = state.huc3.dataTime;
    ramValue_ = state.huc3.ramValue;
    shift_ = state.huc3.shift;
    halted_ = state.huc3.halted;
    modeflag_ = state.huc3.modeflag;
    writingTime_ = state.huc3.writingTime;
    irBaseCycle_ = state.huc3.irBaseCycle;
    irReceivingPulse_ = state.huc3.irReceivingPulse;
}

unsigned char HuC3Chip::read(unsigned p, unsigned long const cc) {
    // should only reach here with ramflag = 0B-0E
    if(ramflag_ == 0x0E) {
        // INFRARED
        if(!irReceivingPulse_) {
            irReceivingPulse_ = true;
            irBaseCycle_ = cc;
        }
        unsigned long cyclesSinceStart = cc - irBaseCycle_;
        unsigned char modulation = (cyclesSinceStart/105) & 1; // 4194304 Hz CPU, 40000 Hz remote signal
        unsigned long timeUs = cyclesSinceStart*36/151;  // actually *1000000/4194304
        // sony protocol
        if(timeUs < 10000) {
            // initialization allowance
            return 0;
        }
        else if(timeUs < 10000 + 2400) {
            // initial mark
            return modulation;
        }
        else if(timeUs < 10000 + 2400 + 600) {
            // initial space
            return 0;
        }
        else {
            // send data
            timeUs -= 13000;
            // write 20 bits (any 20 seem to do)
            unsigned int data = 0xFFFFF;
            for(unsigned long mask = 1UL << (20-1); mask; mask >>= 1) {
                unsigned int markTime = (data & mask) ? 1200 : 600;
                if(timeUs < markTime) { return modulation; }
                timeUs -= markTime;
                if(timeUs < 600) { return 0; }
                timeUs -= 600;
            }
            
            return 0;
        }
    }
    if(ramflag_ < 0x0B || ramflag_ > 0x0D) {
        printf("[HuC3] error, hit huc3 read with ramflag=%02X\n", ramflag_);
        return 0xFF;
    }
    if(ramflag_ == 0x0D) return 1;
    else return ramValue_;
}

void HuC3Chip::write(unsigned p, unsigned data) {
    // as above
    if(ramflag_ == 0x0B) {
        // command
        switch(data & 0xF0) {
            case 0x10:
                // read time
                doLatch();
                if(modeflag_ == HUC3_READ) {
                    ramValue_ = (dataTime_ >> shift_) & 0x0F;
                    shift_ += 4;
                    if(shift_ > 24) shift_ = 0;
                }
                break;
            case 0x30:
                // write time
                if(modeflag_ == HUC3_WRITE) {
                    if(shift_ == 0) writingTime_ = 0;
                    if(shift_ < 24) {
                        writingTime_ |= (data & 0x0F) << shift_;
                        shift_ += 4;
                        if(shift_ == 24) {
                            updateTime();
                            modeflag_ = HUC3_READ;
                        }
                    }
                }
                break;
            case 0x40:
                // some kind of mode shift
                switch(data & 0x0F) {
                    case 0x0:
                        // shift reset?
                        shift_ = 0;
                        break;
                    case 0x3:
                        // write time?
                        modeflag_ = HUC3_WRITE;
                        shift_ = 0;
                        break;
                    case 0x7:
                        modeflag_ = HUC3_READ;
                        shift_ = 0;
                        break;
                    // others are unimplemented so far
                }
                break;
            case 0x50:
                // ???
                break;
            case 0x60:
                modeflag_ = HUC3_READ; // ???
                break;
        }
    }
    // do nothing for 0C/0D yet
}

void HuC3Chip::updateTime() {
    unsigned minute = (writingTime_ & 0xFFF) % 1440;
    unsigned day = (writingTime_ & 0xFFF000) >> 12;
    baseTime_ = std::time(0) - minute*60 - day*86400;
    haltTime_ = baseTime_;
    
}

}
