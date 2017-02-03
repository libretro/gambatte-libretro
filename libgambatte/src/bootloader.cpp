#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "bootloader.h"

using std::string;

inline bool exist(const std::string& name){
   FILE *file = fopen(name.c_str(), "r");
   if(file){
      fclose(file);
      return true;
   }
   return false;
}

extern char systempath[];

static uint8_t bootromswapspace[0x900];
static uint8_t rombackup[0x900];
static void* addrspace_start = NULL;
static unsigned int bootloadersize = 0;
static bool has_called_FF50 = false;
static bool using_bootloader = false;
static bool gbc_mode;



//this is the only retroarch specific function,everything else can just be copied over
static string get_bootloader_path(string bootloadername){
   std::string path;
   path = systempath;
   if(path == "")return "";
   if(path[path.length() - 1] != '/')path += '/';
   path += bootloadername;
   return path;
}

bool have_bootloader(bool isgbc){
   string path;
   if(isgbc)path = get_bootloader_path("gbc_bios.bin");
   else path = get_bootloader_path("gb_bios.bin");
   if(path == "")return false;
   return exist(path);
}

bool loadbootloader(bool isgbc){
   unsigned int size;
   string path;
   int n = 0;
   FILE *fp;
   
   if(isgbc)path = get_bootloader_path("gbc_bios.bin");
   else path = get_bootloader_path("gb_bios.bin");
   if(path == "")return false;
   
   if(isgbc)size = 0x900;
   else size = 0x100;
   
   fp = fopen(path.c_str(), "rb");
   if(fp){
      n = fread(bootromswapspace, size, 1, fp);
      fclose(fp);
   }
   
   if(n != 1)return false;
   bootloadersize = size;
   using_bootloader = true;
   gbc_mode = isgbc;
   
   //backup rom segment that is shared with bootloader
   memcpy(rombackup,(uint8_t*)addrspace_start,size);
   
   //put bootloader in main memory
   memcpy((uint8_t*)addrspace_start,bootromswapspace,size);
   
   //put back cartridge data in a 256 byte window of the bios that is not mapped(GBC only)
   if(isgbc)memcpy(((uint8_t*)addrspace_start) + 0x100,rombackup + 0x100,0x100);
   
   return true;
}

void resetbootloader(){
   bootloadersize = 0;
   has_called_FF50 = false;
   addrspace_start = NULL;
   using_bootloader = false;
}

void bootloader_choosebank(bool inbootloader){
   //inbootloader = (state.mem.ioamhram.get()[0x150] != 0xFF);//do not uncomment this is just for reference
   if(using_bootloader){
      
      //switching from game to bootloader with savestate
      if(inbootloader && has_called_FF50)uncall_FF50();
      
      //switching from bootloader to game with savestate
      else if(!inbootloader && !has_called_FF50)call_FF50();
      
      //switching from game to game or bootloader to bootloader needs no changes
      
   }
}

void set_address_space_start(void* start){
   addrspace_start = start;
}

void call_FF50(){
   if(!has_called_FF50 && using_bootloader){
      //put rom back in main memory when bootloader has finished
      memcpy((uint8_t*)addrspace_start,rombackup,bootloadersize);
      has_called_FF50 = true;
   }
}

//this is a developer function only,a real gameboy can never undo calling 0xFF50,this function is for savestate functionality
void uncall_FF50(){
   memcpy((uint8_t*)addrspace_start,bootromswapspace,bootloadersize);
   //put back cartridge data in a 256 byte window of the bios that is not mapped(GBC only)
   if(gbc_mode)memcpy(((uint8_t*)addrspace_start) + 0x100,rombackup + 0x100,0x100);
   has_called_FF50 = false;
}
