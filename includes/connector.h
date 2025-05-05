#pragma once

#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <deque>
#include <string>
#include <memory>
#include <fstream>
#include <atomic>
#include <thread>
#include <map>
#include <vector>
#include <iostream>

#include "config_loader.h"

class Connector
{
public:
    struct Message{
        uint8_t id;
        std::string message;
    };

    static std::shared_ptr<Connector> get_instance();

    void connect();
    void disconnect();

    bool is_connected() const noexcept;

    bool set_message(Message&) noexcept;

    Message get_message();
    Message get_message(uint8_t);

    ~Connector();
    
private:
    static std::shared_ptr<Connector> _singleton;

    std::deque<Message> _rbuffer;
    int _recv_buffer_size;
    std::deque<Message> _sbuffer;
    int _send_buffer_size;

    std::atomic<bool> _is_connected;

    int _server_socket_fd;
    int _client_socket_fd;
    int _socket_fd;

    Config_Loader _config;

    Connector();

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector& operator=(Connector&&) = delete;

    void _recv_update();
    void _send_update();

    void _create_server_socket();
    void _create_client_socket();
    void _close_sockets();

    void _recv_message();
    void _send_message();

    std::string _encode(const Message&);
    Message _decode(std::string&);
};