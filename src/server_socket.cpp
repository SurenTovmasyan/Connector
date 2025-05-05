#include "../includes/server_socket.h"

constexpr const int SOCKET_TIMEOUT_US = 500 * 1000; // 500ms

Server_Socket::Server_Socket(const std::string& file_path_):
    Socket(file_path_),
    _addr(),
    _timeout()
{
    std::cout << "[SERVER] Configurating IP address" << std::endl;
    
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(_config.get_config()["self_connector_port"]);
    _addr.sin_addr.s_addr = INADDR_ANY;
}


Server_Socket::~Server_Socket(){
    disconnect();
}

void Server_Socket::connect() {
    if (_is_connected || _is_thread_working)
        return;

    std::thread server_thread_([&]() { _connect(); });

    server_thread_.detach();
}

void Server_Socket::disconnect(){
    if(_is_thread_working)
        _is_thread_working = false;

    if(!_is_connected)
        return;

    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Server_Socket::_connect(){
    _is_thread_working = true;

    std::cout << "[SERVER] Getting listening socket's file descriptor" << std::endl;
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1) {
        perror("[SERVER] socket");
        _is_thread_working = false;
        return;
    }

    std::cout << "[SERVER] Binding and listening" << std::endl;
    if (bind(listen_fd_, (sockaddr*)&_addr, sizeof(_addr)) == -1) {
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

        _timeout.tv_sec = 0;
        _timeout.tv_usec = SOCKET_TIMEOUT_US;

        int ready_ = select(listen_fd_ + 1, &readfds_, nullptr, nullptr, &_timeout);

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