#ifndef _NETPLAY_SERIAL_H
#define _NETPLAY_SERIAL_H

#include <gambatte.h>
#include <libretro.h>
#include <time.h>

#define NETPLAY_SERIAL_PACKET_FLAGS                                            \
  RETRO_NETPACKET_FLUSH_HINT | RETRO_NETPACKET_RELIABLE
#define NETPLAY_SERIAL_BUFFER_SIZE 128

class NetplaySerial : public gambatte::SerialIO {
public:
  NetplaySerial();
  ~NetplaySerial();
  void netplayStart(uint16_t clientId, retro_netpacket_send_t sendCallback,
                    retro_netpacket_poll_receive_t pollCallback);
  void netplayStop();
  bool netplayConnect(uint16_t clientId);
  void netplayDisconnect(uint16_t clientId);
  void netplayReceive(const void *buf, size_t len, uint16_t client_id);
  virtual bool check(unsigned char out, unsigned char &in, bool &fastCgb);
  virtual unsigned char send(unsigned char data, bool fastCgb);

private:
  bool isAvailable_();
  bool hasData_();
  int getData_(void *buf, size_t len);

  retro_netpacket_send_t send_cb_;
  retro_netpacket_poll_receive_t poll_cb_;
  bool hasPeer_;
  char buffer_[NETPLAY_SERIAL_BUFFER_SIZE]; // Buffer to store received bytes
  uint16_t dataAvailable_;
};

#endif