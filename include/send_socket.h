#pragma once

#include "socket.h"

class Send_Socket: public Socket
{
public:
    Send_Socket(const std::string&);
    ~Send_Socket();

    void connect();
    void disconnect();

private:
    sockaddr_in _peer_addr;

    void _connect();
};