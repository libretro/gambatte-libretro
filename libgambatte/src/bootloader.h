//load the rom then call the loadbootloader() function before execution starts
bool have_bootloader(bool isgbc);
bool loadbootloader(bool isgbc);
void resetbootloader();

void bootloader_choosebank(bool inbootloader);

void set_address_space_start(void* start);

void call_FF50();
void uncall_FF50();

