#pragma once

#include "socket.h"

class Server_Socket: public Socket
{
public:
    Server_Socket(const std::string&);
    ~Server_Socket();

    void connect();
    void disconnect();

private:
    sockaddr_in _addr;
    timeval _timeout;

    void _connect();
};