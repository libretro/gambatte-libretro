#include "netplay_serial.h"
#include "gambatte_log.h"
#include <string.h>
#include <math.h>

NetplaySerial::NetplaySerial() : dataAvailable_(0), hasPeer_(false)
{
}

NetplaySerial::~NetplaySerial()
{
}

void NetplaySerial::netplayStart(uint16_t clientId, retro_netpacket_send_t sendCallback, retro_netpacket_poll_receive_t pollCallback)
{
    gambatte_log(RETRO_LOG_DEBUG, "Starting netplay!\n");
    send_cb_ = sendCallback;
    poll_cb_ = pollCallback;
}

void NetplaySerial::netplayStop()
{
    gambatte_log(RETRO_LOG_DEBUG, "Stopping netplay!\n");
    send_cb_ = NULL;
    poll_cb_ = NULL;
    hasPeer_ = false;
    dataAvailable_ = 0;
}

bool NetplaySerial::netplayConnect(uint16_t clientId)
{
    if (hasPeer_)
    {
        gambatte_log(RETRO_LOG_DEBUG, "Client tried to connect but we already have a peer\n");
        return false;
    }
    else
    {
        gambatte_log(RETRO_LOG_DEBUG, "Client connected\n");
        hasPeer_ = true;
        return true;
    }
}

void NetplaySerial::netplayDisconnect(uint16_t clientId)
{
    gambatte_log(RETRO_LOG_DEBUG, "Client disconnected\n");
    hasPeer_ = false;
}

void NetplaySerial::netplayReceive(const void *buf, size_t len, uint16_t client_id)
{
    if (dataAvailable_ + len > NETPLAY_SERIAL_BUFFER_SIZE)
    {
        gambatte_log(RETRO_LOG_WARN, "[receive] buffer full, dropping packets...\n");
        return;
    }
    memcpy(&buffer_[dataAvailable_], buf, len);
    dataAvailable_ += len;
}

bool NetplaySerial::check(unsigned char out, unsigned char &in, bool &fastCgb)
{
    if (!isAvailable_() || dataAvailable_ < 2)
    {
        return false;
    }
    int read;
    unsigned char buffer[2];
    if ((read = getData_(&buffer, 2)) <= 0)
    {
        gambatte_log(RETRO_LOG_ERROR, "[check] Error during receive (read=%d)!\n", read);
        return false;
    }
    in = buffer[0];
    fastCgb = buffer[1];
    buffer[0] = out;
    buffer[1] = 128;
    send_cb_(NETPLAY_SERIAL_PACKET_FLAGS, buffer, 2, RETRO_NETPACKET_BROADCAST);
    return true;
}

unsigned char NetplaySerial::send(unsigned char data, bool fastCgb)
{
    if (!isAvailable_())
    {
        return 0xFF;
    }
    unsigned char buffer[2];
    buffer[0] = data;
    buffer[1] = fastCgb;
    send_cb_(NETPLAY_SERIAL_PACKET_FLAGS, buffer, 2, RETRO_NETPACKET_BROADCAST);
    // Poll data to wait for response
    while (!hasData_() && isAvailable_()) // TODO timeout
    {
        poll_cb_();
    }
    int read;
    if ((read = getData_(&buffer, 2)) < 0)
    {
        gambatte_log(RETRO_LOG_ERROR, "[send] Received invalid data length (%d)\n", read);
        return 0xFF;
    }
    return buffer[0];
}

bool NetplaySerial::isAvailable_()
{
    return send_cb_ != NULL && poll_cb_ != NULL;
}

bool NetplaySerial::hasData_()
{
    return dataAvailable_ > 0;
}

int NetplaySerial::getData_(void *buf, size_t len)
{
    if (len > dataAvailable_ || len <= 0)
    {
        return -1;
    }
    int avail = len > dataAvailable_ ? dataAvailable_ : len;
    memcpy(buf, &buffer_, avail);
    memmove(&buffer_[0], &buffer_[avail], dataAvailable_ - avail);
    dataAvailable_ -= avail;
    return avail;
}
