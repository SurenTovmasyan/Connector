#include "../include/recv_socket.h"

constexpr const int SOCKET_TIMEOUT_US = 500 * 1000; // 500ms

Recv_Socket::Recv_Socket(int self_port_):
    _addr(),
    _timeout()
{
    std::cout << "[RECEIVER] Configurating IP address" << std::endl;
    
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(self_port_);
    _addr.sin_addr.s_addr = INADDR_ANY;
}


Recv_Socket::~Recv_Socket(){
    disconnect();
}

void Recv_Socket::connect() {
    std::cout << "[RECEIVER] " << _is_connected << ' ' << _is_thread_working << std::endl;

    if (_is_connected || _is_thread_working)
        return;

    std::thread receiver_thread_([&]() { _connect(); });

    receiver_thread_.detach();
}

void Recv_Socket::disconnect(){
    if(_is_thread_working)
        _is_thread_working = false;

    if(!_is_connected)
        return;

    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Recv_Socket::_connect(){
    _is_thread_working = true;

    std::cout << "[RECEIVER] Getting listening socket's file descriptor" << std::endl;
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        perror("[RECEIVER] socket");
        _is_thread_working = false;
        return;
    }

    std::cout << "[RECEIVER] Binding and listening" << std::endl;
    if (bind(listen_fd_, (sockaddr*)&_addr, sizeof(_addr)) == -1) {
        perror("[RECEIVER] bind");
        close(listen_fd_);
        _is_thread_working = false;
        return;
    }

    if (listen(listen_fd_, 1) == -1) {
        perror("[RECEIVER] listen");
        close(listen_fd_);
        _is_thread_working = false;
        return;
    }

    fd_set readfds_;

    while(_is_thread_working && !_is_connected){
        std::cout << "[RECEIVER] Waiting for connection" << std::endl;
        FD_ZERO(&readfds_);
        FD_SET(listen_fd_, &readfds_);

        _timeout.tv_sec = 0;
        _timeout.tv_usec = SOCKET_TIMEOUT_US;

        int ready_ = select(listen_fd_ + 1, &readfds_, nullptr, nullptr, &_timeout);

        if (ready_ == 0) {
            std::cout << "[RECEIVER] Connection timeout, retrying..." << std::endl;
            continue;
        }        

        if (ready_ > 0 && FD_ISSET(listen_fd_, &readfds_)) {
            sockaddr_in client_addr_{};
            socklen_t client_len_ = sizeof(client_addr_);

            std::cout << "[RECEIVER] Accepting client..." << std::endl;
            _socket_fd = accept(listen_fd_, (sockaddr*)&client_addr_, &client_len_);

            if (_socket_fd == -1) {
                perror("[RECEIVER] accept");
                close(listen_fd_);
                _is_thread_working = false;
                return;
            }

            char client_ip_[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr_.sin_addr, client_ip_, sizeof(client_ip_));
            std::cout << "[RECEIVER] Connected to client: " << client_ip_ << std::endl;

            _is_connected = true;

            std::cout << "[RECEIVER] Closing listening socket" << std::endl;
            close(listen_fd_);

            _is_thread_working = false;
        }
    }
}