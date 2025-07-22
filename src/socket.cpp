#include "../include/socket.h"

Socket::Socket(const std::string& file_path_):
    _socket_fd(-1),
    _is_connected(0),
    _is_thread_working(0),
    _config(file_path_)
{}

Socket::~Socket() {}

Socket::operator int() const{
    return _socket_fd;
}