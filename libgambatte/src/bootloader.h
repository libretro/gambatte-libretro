#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <string>

extern bool usebootloaders;
extern bool (*get_raw_bootloader_data)(bool/*gbcver*/,uint8_t* /*data*/);

class Bootloader{
private:
   uint8_t bootromswapspace[0x900];
   uint8_t rombackup[0x900];
   void* addrspace_start;
   unsigned int bootloadersize;
   bool has_called_FF50;
   bool using_bootloader;
   bool gbc_mode;

   void patch_gbc_to_gba_mode();
   
public:
   //load the rom then call the loadbootloader() function before execution starts
   void load(bool isgbc,bool isgba);
   void reset();
   bool enabled();

   void set_address_space_start(void* start);

   void choosebank(bool inbootloader);

   void call_FF50();
   void uncall_FF50();
};

#endif

