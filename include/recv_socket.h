#pragma once

#include "socket.h"

class Recv_Socket: public Socket
{
public:
    Recv_Socket(int);
    ~Recv_Socket();

    void connect();
    void disconnect();

private:
    sockaddr_in _addr;
    timeval _timeout;

    void _connect();
};