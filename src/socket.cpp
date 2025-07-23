#include "../include/socket.h"

Socket::Socket():
    _socket_fd(-1),
    _is_connected(false),
    _is_thread_working(false)
{}

Socket::~Socket() {}

Socket::operator int() const{
    return _socket_fd;
}