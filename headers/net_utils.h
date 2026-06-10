#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

class WinsockInitializer {
public:
    WinsockInitializer();
    ~WinsockInitializer();

    WinsockInitializer(const WinsockInitializer&) = delete;
    WinsockInitializer& operator=(const WinsockInitializer&) = delete;

    bool ok() const { return initialized_; }

private:
    bool initialized_ = false;
};

SOCKET create_listen_socket(int port);
SOCKET accept_client(SOCKET listen_socket);
SOCKET connect_to_host(const std::string& host, int port);

bool send_line(SOCKET socket_handle, const std::string& line);
bool recv_line(SOCKET socket_handle, std::string& line);

void close_socket(SOCKET socket_handle);
