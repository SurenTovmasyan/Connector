#pragma once

#include <atomic>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>

class Socket
{
public:
    enum Socket_Type{
        SERVER,
        CLIENT
    };

    static const int SOCKET_TIMEOUT_US;

    explicit Socket(Socket_Type, int, const std::string&, int);
    ~Socket();

    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    operator int() const;

    void connect();
    void disconnect();

    bool is_connected() const { return _is_connected; };

protected:
    int _socket_fd;
    Socket_Type _type;

    std::atomic<bool> _is_connected;
    std::atomic<bool> _is_thread_working;

    int _self_port;

    std::string _other_ip;
    int _other_port;

    void _server_connect();
    void _client_connect();
};