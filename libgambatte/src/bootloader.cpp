#include <stdint.h>
#include <string.h>
#include <string>

#include "bootloader.h"

Bootloader::Bootloader(){
   usebootloaders = true;
   get_raw_bootloader_data = NULL;
}

void Bootloader::patch_gbc_to_gba_mode(){
   /*moves one jump over another and puts ld b,0x01 into the original position*/
   uint16_t patchloc = 0xF2;
   uint8_t patch[0x7] = {0xCD,0xD0,0x05/*<-call systemsetup*/,0x06,0x01/*<-ld b,0x1*/,0x00/*<-nop*/,0x00/*<-nop*/};
   memcpy(bootromswapspace + patchloc,patch,0x7);
}

void Bootloader::load(bool isgbc,bool isgba){
   bool bootloaderavail;
   
   if(get_raw_bootloader_data == NULL){
      using_bootloader = false;
      return;
   }
   
   bootloaderavail = get_raw_bootloader_data(isgbc,bootromswapspace);
   
   if(!bootloaderavail){
      using_bootloader = false;
      return;
   }
   
   if(isgbc)bootloadersize = 0x900;
   else bootloadersize = 0x100;
   
   using_bootloader = true;
   gbc_mode = isgbc;
   
   if(isgba){//patch bootloader to fake gba mode
      patch_gbc_to_gba_mode();
   }
   
   //backup rom segment that is shared with bootloader
   memcpy(rombackup,(uint8_t*)addrspace_start,bootloadersize);
   
   //put bootloader in main memory
   memcpy((uint8_t*)addrspace_start,bootromswapspace,bootloadersize);
   
   //put back cartridge data in a 256 byte window of the bios that is not mapped(GBC only)
   if(isgbc)memcpy(((uint8_t*)addrspace_start) + 0x100,rombackup + 0x100,0x100);
}

void Bootloader::reset(){
   bootloadersize = 0;
   has_called_FF50 = false;
   addrspace_start = NULL;
   using_bootloader = false;
   gbc_mode = false;
}

bool Bootloader::booting_with_bootloader(){
   return using_bootloader;
}

void Bootloader::set_bootloader_getter(bool (*getter)(bool,uint8_t*)){
   get_raw_bootloader_data = getter;
}

bool Bootloader::get_bootloader_enabled(){
   return usebootloaders;
}

void Bootloader::set_bootloader_enabled(bool enabled){
   usebootloaders = enabled;
}

void Bootloader::set_address_space_start(void* start){
   addrspace_start = start;
}

void Bootloader::choosebank(bool inbootloader){
   //inbootloader = (state.mem.ioamhram.get()[0x150] != 0xFF);//do not uncomment this is just for reference
   if(using_bootloader){
      
      //switching from game to bootloader with savestate
      if(inbootloader && has_called_FF50)uncall_FF50();
      
      //switching from bootloader to game with savestate
      else if(!inbootloader && !has_called_FF50)call_FF50();
      
      //switching from game to game or bootloader to bootloader needs no changes
      
   }
}

void Bootloader::call_FF50(){
   if(!has_called_FF50 && using_bootloader){
      //put rom back in main memory when bootloader has finished
      memcpy((uint8_t*)addrspace_start,rombackup,bootloadersize);
      has_called_FF50 = true;
   }
}

//this is a developer function only,a real gameboy can never undo calling 0xFF50,this function is for savestate functionality
void Bootloader::uncall_FF50(){
   memcpy((uint8_t*)addrspace_start,bootromswapspace,bootloadersize);
   //put back cartridge data in a 256 byte window of the bios that is not mapped(GBC only)
   if(gbc_mode)memcpy(((uint8_t*)addrspace_start) + 0x100,rombackup + 0x100,0x100);
   has_called_FF50 = false;
}
