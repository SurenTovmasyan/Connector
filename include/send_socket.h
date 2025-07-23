#pragma once

#include "socket.h"

class Send_Socket: public Socket
{
public:
    Send_Socket(const std::string&, int);
    ~Send_Socket();

    void connect();
    void disconnect();

private:
    sockaddr_in _peer_addr;
    std::string _other_side_ip;

    void _connect();
};