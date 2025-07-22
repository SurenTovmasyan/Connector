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

#include "config_loader.h"
#ifdef RECV
#include "recv_socket.h"
#else
#include "send_socket.h"
#endif

class Connector
{
public:
    explicit Connector();

    void connect();
    void disconnect();

#ifdef RECV
    std::string recv_message();
#else
    void send_message(const std::string&);
#endif

    ~Connector();
    
private:
#ifdef RECV
    Recv_Socket _recv_socket;
#else
    Send_Socket _send_socket;
#endif

    Config_Loader _config;

    Connector(const Connector&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector& operator=(Connector&&) = delete;
};