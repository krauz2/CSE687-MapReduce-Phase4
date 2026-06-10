#include "net_utils.h"

WinsockInitializer::WinsockInitializer() {
    WSADATA wsa_data{};
    initialized_ = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
}

WinsockInitializer::~WinsockInitializer() {
    if (initialized_) {
        WSACleanup();
    }
}

SOCKET create_listen_socket(int port) {
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    const BOOL reuse = TRUE;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<u_short>(port));

    if (bind(listen_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        close_socket(listen_socket);
        return INVALID_SOCKET;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        close_socket(listen_socket);
        return INVALID_SOCKET;
    }

    return listen_socket;
}

SOCKET accept_client(SOCKET listen_socket) {
    return accept(listen_socket, nullptr, nullptr);
}

SOCKET connect_to_host(const std::string& host, int port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        close_socket(client_socket);
        return INVALID_SOCKET;
    }

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        close_socket(client_socket);
        return INVALID_SOCKET;
    }

    return client_socket;
}

bool send_line(SOCKET socket_handle, const std::string& line) {
    std::string payload = line + "\n";
    const char* data = payload.c_str();
    int total_sent = 0;
    const int total_size = static_cast<int>(payload.size());

    while (total_sent < total_size) {
        const int sent = send(socket_handle, data + total_sent, total_size - total_sent, 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }
        total_sent += sent;
    }

    return true;
}

bool recv_line(SOCKET socket_handle, std::string& line) {
    line.clear();
    char ch = '\0';

    while (true) {
        const int received = recv(socket_handle, &ch, 1, 0);
        if (received == 0) {
            return !line.empty();
        }
        if (received == SOCKET_ERROR) {
            return false;
        }

        if (ch == '\n') {
            return true;
        }

        if (ch != '\r') {
            line.push_back(ch);
        }
    }
}

void close_socket(SOCKET socket_handle) {
    if (socket_handle != INVALID_SOCKET) {
        closesocket(socket_handle);
    }
}
