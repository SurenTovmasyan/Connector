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
#include <fstream>
#include <atomic>
#include <thread>
#include <map>
#include <vector>
#include <iostream>
#include <mutex>

#include "socket.h"

class Connector
{
public:
    struct Message{
        uint8_t id;
        std::string message;
    };

    void connect();
    void disconnect();

    bool set_message(Message&&) noexcept;

    Message get_message();
    Message get_message(uint8_t);

    int available() const noexcept;

    Connector(int, int, Socket::Socket_Type, int, const std::string&, int);
    ~Connector();
    
private:
    std::mutex _smtx;
    std::mutex _rmtx;

    std::deque<Message> _rbuffer;
    int _recv_buffer_size;
    std::deque<Message> _sbuffer;
    int _send_buffer_size;

    Socket _socket;

    Connector(const Connector&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector& operator=(Connector&&) = delete;

    std::thread _thrd;
    void _update();

    void _recv_message();
    void _send_message();

    std::string _encode(const Message&);
    Message _decode(std::string&);
};