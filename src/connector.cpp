#include "../include/connector.h"

constexpr const int BUFFER_SIZE = 4096;
constexpr const int SOCKET_TIMEOUT_US = 50 * 1000; // 50ms
constexpr const int SOCKET_TIMEOUT_CYCLES = 50; // 50 * 100ms = 5s

Connector::Connector():
#ifdef RECV
    _recv_socket("../config.json"),
#else
    _send_socket("../config.json"),
#endif
    _config("../config.json")
{
    connect();
}

void Connector::connect(){
#ifdef RECV
    _recv_socket.connect();
#else
    _send_socket.connect();
#endif

    int cycle_counter_ = 0;
#ifdef RECV
    while((!_recv_socket.is_connected())&& cycle_counter_ < SOCKET_TIMEOUT_CYCLES)
#else
    while((!_send_socket.is_connected()) && cycle_counter_ < SOCKET_TIMEOUT_CYCLES)
#endif      
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cycle_counter_++;
    }

#ifdef RECV
    if(!_recv_socket.is_connected()){
        _recv_socket.disconnect();
#else
    if(!_send_socket.is_connected()){
        _send_socket.disconnect();
#endif
        std::cout << "Timeout worked, couldn't connect" << std::endl;
        return;
    }
}

void Connector::disconnect(){
    std::cout << "Disconnecting" << std::endl;

#ifdef RECV
    _recv_socket.disconnect();
#else
    send_message("EOC");

    _send_socket.disconnect();
#endif
}

#ifdef RECV
std::string Connector::recv_message(){
    char buffer_recv_[BUFFER_SIZE] = {0};

    timeval timeout_;
    timeout_.tv_sec = 0;  
    timeout_.tv_usec = SOCKET_TIMEOUT_US;

    setsockopt(_recv_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));

    std::cout << "Recieving message" << std::endl;
    int recv_res_ = recv(_recv_socket, buffer_recv_, sizeof(buffer_recv_), 0);

    if(recv_res_ >= 0){
        std::string msg_ = buffer_recv_;
        
        if(msg_ == "EOC"){
            std::cout << "Disconnecting" << std::endl;

            disconnect();
        }

        return msg_;
    }    

    return "";
}
#else
void Connector::send_message(const std::string& msg_){
    char buffer_send_[BUFFER_SIZE];

    std::cout << "Copying encoded message into buffer" << std::endl;
    std::strcpy(buffer_send_, msg_.c_str());

    std::cout << "Sending message " << msg_.length() << ' ' << buffer_send_ << std::endl;
    send(_send_socket, buffer_send_, msg_.length(), 0);
}
#endif

Connector::~Connector(){
    disconnect();
}