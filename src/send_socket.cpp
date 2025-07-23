#include "../include/send_socket.h"

constexpr const int SOCKET_TIMEOUT_MS = 250; // 250ms

Send_Socket::Send_Socket(const std::string& other_side_ip_, int other_side_port_):
    _peer_addr(),
    _other_side_ip(other_side_ip_)
{
    std::cout << "[SENDER] Configurating other side's IP and port" << std::endl;
    
    _peer_addr.sin_family = AF_INET;
    _peer_addr.sin_port = htons(other_side_port_);
}

Send_Socket::~Send_Socket(){
    disconnect();
}

void Send_Socket::connect() {
    std::cout << "[SENDER] " << _is_connected << ' ' << _is_thread_working << std::endl;
    if (_is_connected || _is_thread_working)
        return;

    std::thread sender_thread_([&]() { _connect(); });

    sender_thread_.detach();
}

void Send_Socket::disconnect(){
    if(_is_thread_working)
        _is_thread_working = false;

    if(!_is_connected)
        return;

    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Send_Socket::_connect() {
    _is_thread_working = true;

    std::cout << "[SENDER] Waiting some time before connecting other side" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    inet_pton(AF_INET, ((std::string)(_other_side_ip)).c_str(), &_peer_addr.sin_addr);

    while(!_is_connected && _is_thread_working) {
        std::cout << "[SENDER] Trying to connect to the other side" << std::endl;
        int temp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

        if (::connect(temp_socket_, (sockaddr*)&_peer_addr, sizeof(_peer_addr)) == 0) {
            std::cout << "[SENDER] Setting socket SENDER" << std::endl;
            _socket_fd = temp_socket_;
            _is_connected = true;

            _is_thread_working = false;
        }
        else {
            close(temp_socket_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SOCKET_TIMEOUT_MS));
    }
}