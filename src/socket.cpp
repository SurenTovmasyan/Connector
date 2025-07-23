#include "../include/socket.h"

constexpr const int SOCKET_TIMEOUT_US = 500 * 1000;

Socket::Socket(Socket_Type type_, int sport_, const std::string& oip_, int oport_):
    _socket_fd(-1),
    _type(type_),
    _is_connected(false),
    _is_thread_working(false),
    _self_port(sport_),
    _other_ip(oip_),
    _other_port(oport_)
{   
    connect();
}

Socket::~Socket(){
    disconnect();
}

Socket::operator int() const{
    return _socket_fd;
}

void Socket::connect(){
    if(_is_connected || _is_thread_working) return;

    if(_type == SERVER){
        std::thread thrd_([&]() { _server_connect(); });
        thrd_.detach();
    }
    else{
        std::thread thrd_([&]() { _client_connect(); });
        thrd_.detach();
    }
}

void Socket::disconnect(){
    if(!_is_connected) return;

    _is_thread_working = false;
    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Socket::_server_connect(){ // MAKE THIS SHIT SMALLER AND MORE READABLE
    sockaddr_in addr_;
    timeval timeout_;

    _is_thread_working = true;

    std::cout << "[RECEIVER] Getting listening socket's file descriptor" << std::endl;
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        perror("[RECEIVER] socket");
        _is_thread_working = false;
        return;
    }

    std::cout << "[RECEIVER] Binding and listening" << std::endl;
    if (bind(listen_fd_, (sockaddr*)&addr_, sizeof(addr_)) == -1) {
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

        timeout_.tv_sec = 0;
        timeout_.tv_usec = SOCKET_TIMEOUT_US;

        int ready_ = select(listen_fd_ + 1, &readfds_, nullptr, nullptr, &timeout_);

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

void Socket::_client_connect(){

}