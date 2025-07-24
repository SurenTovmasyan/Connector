#include "../include/socket.h"

const int Socket::SOCKET_TIMEOUT_US = 500 * 1000;

Socket::Socket(Socket_Type type_, int sport_, const std::string& oip_, int oport_):
    _socket_fd(-1),
    _type(type_),
    _is_connected(false),
    _is_thread_working(false),
    _self_port(sport_),
    _other_ip(oip_),
    _other_port(oport_)
{}

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
    _is_thread_working = false;
    if(!_is_connected) return;

    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Socket::_server_connect(){ // MAKE THIS SHIT SMALLER AND MORE READABLE
    sockaddr_in addr_;
    timeval timeout_;

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(_self_port);
    addr_.sin_addr.s_addr = INADDR_ANY;

    _is_thread_working = true;

    std::cout << "[SERVER] Getting listening socket's file descriptor" << std::endl;
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        perror("[SERVER] socket");
        _is_thread_working = false;
        return;
    }

    std::cout << "[SERVER] Binding and listening" << std::endl;
    if (bind(listen_fd_, (sockaddr*)&addr_, sizeof(addr_)) == -1) {
        perror("[SERVER] bind");
        close(listen_fd_);
        _is_thread_working = false;
        return;
    }

    if (listen(listen_fd_, 1) == -1) {
        perror("[SERVER] listen");
        close(listen_fd_);
        _is_thread_working = false;
        return;
    }

    fd_set readfds_;

    while(_is_thread_working && !_is_connected){
        std::cout << "[SERVER] Waiting for connection" << std::endl;
        FD_ZERO(&readfds_);
        FD_SET(listen_fd_, &readfds_);

        timeout_.tv_sec = 0;
        timeout_.tv_usec = SOCKET_TIMEOUT_US;

        int ready_ = select(listen_fd_ + 1, &readfds_, nullptr, nullptr, &timeout_);

        if (ready_ == 0) {
            std::cout << "[SERVER] Connection timeout, retrying..." << std::endl;
            continue;
        }        

        if (ready_ > 0 && FD_ISSET(listen_fd_, &readfds_)) {
            sockaddr_in client_addr_{};
            socklen_t client_len_ = sizeof(client_addr_);

            std::cout << "[SERVER] Accepting client..." << std::endl;
            _socket_fd = accept(listen_fd_, (sockaddr*)&client_addr_, &client_len_);

            if (_socket_fd == -1) {
                perror("[SERVER] accept");
                close(listen_fd_);
                _is_thread_working = false;
                return;
            }

            char client_ip_[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr_.sin_addr, client_ip_, sizeof(client_ip_));
            std::cout << "[SERVER] Connected to client: " << client_ip_ << std::endl;

            _is_connected = true;

            std::cout << "[SERVER] Closing listening socket" << std::endl;
            close(listen_fd_);

            _is_thread_working = false;
        }
    }
}

void Socket::_client_connect(){
    sockaddr_in peer_addr_;

    peer_addr_.sin_family = AF_INET;
    peer_addr_.sin_port = htons(_other_port);

    _is_thread_working = true;

    std::cout << "[CLIENT] Waiting some time before connecting other side" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    inet_pton(AF_INET, ((std::string)(_other_ip)).c_str(), &peer_addr_.sin_addr);

    while(!_is_connected && _is_thread_working) {
        std::cout << "[CLIENT] Trying to connect to the other side" << std::endl;
        int temp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

        if (::connect(temp_socket_, (sockaddr*)&peer_addr_, sizeof(peer_addr_)) == 0) {
            std::cout << "[CLIENT] Setting socket CLIENT" << std::endl;
            _socket_fd = temp_socket_;
            _is_connected = true;

            _is_thread_working = false;
        }
        else {
            close(temp_socket_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SOCKET_TIMEOUT_US / 1000));
    }
}