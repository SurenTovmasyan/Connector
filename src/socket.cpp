#include "../includes/socket.h"

Socket::Socket(const std::string& file_path_):
    _socket_fd(-1),
    _config(file_path_)
{}

Socket::~Socket() {}

Socket::operator int() const{
    return _socket_fd;
}