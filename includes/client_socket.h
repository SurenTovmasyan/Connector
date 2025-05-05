#pragma once

#include "socket.h"

class Client_Socket: public Socket
{
public:
    Client_Socket(const std::string&);
    ~Client_Socket();

    void connect();
    void disconnect();

private:
    sockaddr_in _peer_addr;

    void _connect();
};