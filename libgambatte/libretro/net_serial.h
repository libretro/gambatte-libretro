#ifndef _NET_SERIAL_H
#define _NET_SERIAL_H

#if defined(__HAIKU__)
#include <sys/socket.h>
#include <sys/select.h>
#endif

#if _WIN32
#include <winsock2.h>
#endif

#include <gambatte.h>
#include <time.h>

class NetSerial : public gambatte::SerialIO
{
	public:
		NetSerial();
		~NetSerial();

		bool start(bool is_server, int port, const std::string& hostname);
		void stop();

		virtual bool check(unsigned char out, unsigned char& in, bool& fastCgb);
		virtual unsigned char send(unsigned char data, bool fastCgb);

	private:
		bool startServerSocket();
		bool startClientSocket();
		bool acceptClient();
		bool checkAndRestoreConnection(bool throttle);

		bool is_stopped_;
		bool is_server_;
		int  port_;
		std::string hostname_;

		int server_fd_;
		int sockfd_;

		clock_t lastConnectAttempt_;

#ifdef __WIN32
		WSADATA wsaData;
		int wsaStartupStatus;
#endif
};

#endif
