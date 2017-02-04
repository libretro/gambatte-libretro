#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <string>

class Bootloader{
   
private:
   uint8_t bootromswapspace[0x900];
   uint8_t rombackup[0x900];
   void* addrspace_start;
   unsigned int bootloadersize;
   bool has_called_FF50;
   bool using_bootloader;
   bool gbc_mode;
   bool usebootloaders;
   bool (*get_raw_bootloader_data)(bool/*gbcver*/,uint8_t* /*data*/);

   void patch_gbc_to_gba_mode();
   void uncall_FF50();
   
public:
   Bootloader();
   void load(bool isgbc,bool isgba);
   void reset();
   bool booting_with_bootloader();

   void set_bootloader_getter(bool (*getter)(bool,uint8_t*));
   
   bool get_bootloader_enabled();
   void set_bootloader_enabled(bool enabled);
   
   void set_address_space_start(void* start);

   void choosebank(bool inbootloader);

   void call_FF50();
};

#endif

