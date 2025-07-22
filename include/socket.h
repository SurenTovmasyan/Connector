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

#include "../config_loader/include/config_loader.h"

// @TODO: create socket manager, which will connect and disconnect to the remote module of mine
// create 2 socket classes - Server_Socket and Client_Socket, each will try connect as it should
// Manager will manage them, stop the threads they have created and take the gotten socket

class Socket
{
public:
    explicit Socket(const std::string&);
    virtual ~Socket();

    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    operator int() const;

    virtual void connect()    = 0;
    virtual void disconnect() = 0;

    bool is_connected() const { return _is_connected; };

protected:
    int _socket_fd;

    std::atomic<bool> _is_connected;
    std::atomic<bool> _is_thread_working;
    
    Config_Loader<Config_Type::JSON> _config;
};