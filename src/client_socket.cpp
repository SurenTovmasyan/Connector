#include "../include/client_socket.h"

constexpr const int SOCKET_TIMEOUT_MS = 250; // 250ms

Client_Socket::Client_Socket(const std::string& file_path_):
    Socket(file_path_),
    _peer_addr()
{
    std::cout << "[CLIENT] Configurating other side's IP and port" << std::endl;
    
    _peer_addr.sin_family = AF_INET;
    _peer_addr.sin_port = htons(_config.get_config()["other_connector_port"]);

}

Client_Socket::~Client_Socket(){
    disconnect();
}

void Client_Socket::connect() {
    if (_is_connected || _is_thread_working)
        return;

    std::thread client_thread_([&]() { _connect(); });

    client_thread_.detach();
}

void Client_Socket::disconnect(){
    if(_is_thread_working)
        _is_thread_working = false;

    if(!_is_connected)
        return;

    _is_connected = false;

    shutdown(_socket_fd, SHUT_RDWR);
    close(_socket_fd);
}

void Client_Socket::_connect() {
    _is_thread_working = true;

    std::cout << "[CLIENT] Waiting some time before connecting other side" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    inet_pton(AF_INET, ((std::string)(_config.get_config()["other_connector_ip"])).c_str(), &_peer_addr.sin_addr);

    while(!_is_connected && _is_thread_working) {
        std::cout << "[CLIENT] Trying to connect to the other side" << std::endl;
        int temp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

        if (::connect(temp_socket_, (sockaddr*)&_peer_addr, sizeof(_peer_addr)) == 0) {
            std::cout << "[CLIENT] Setting socket CLIENT" << std::endl;
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