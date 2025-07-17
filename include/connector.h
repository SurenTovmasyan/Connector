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
#include "server_socket.h"
#else
#include "client_socket.h"
#endif

class Connector
{
public:
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
    Server_Socket _recv_socket;
#else
    Client_Socket _send_socket;
#endif

    Config_Loader _config;

    Connector();

    Connector(const Connector&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector& operator=(Connector&&) = delete;
};