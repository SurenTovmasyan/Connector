#include "../include/connector.h"

// @TODO: add 3 classes - Socket, ConfigLoader, Codec
// 3. Codec - codes and decodes given string

#ifdef Codec
constexpr const int END_OF_CODE = 127;
constexpr const int END_OF_INDEX = 126;
constexpr const int END_OF_ID = '>';
#endif

constexpr const int LENGTH_OF_MSG = 125;
constexpr const int BUFFER_SIZE = 4096;
constexpr const int THREAD_SLEEP_MS = 25;
constexpr const int SOCKET_TIMEOUT_US = 50 * 1000; // 50ms
constexpr const int SOCKET_TIMEOUT_CYCLES = 500; // 50 * 100ms = 5s

void Connector::connect(){
    std::cout << "[CONNECTOR] Trying to connect" << std::endl;

    _recv_socket.connect();
    _send_socket.connect();

    int cycle_counter_ = 0;
    while((!_recv_socket.is_connected() || !_send_socket.is_connected()) 
            && cycle_counter_ < SOCKET_TIMEOUT_CYCLES)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cycle_counter_++;
    }

    if(!_recv_socket.is_connected() || !_send_socket.is_connected()){
        _recv_socket.disconnect();
        _send_socket.disconnect();
        std::cout << "[CONNECTOR] Timeout worked, couldn't connect" << std::endl;
        return;
    }

    std::cout << "[CONNECTOR] Creating working thread" << std::endl;
    _thr = std::thread([&]() { _update(); });
}

void Connector::disconnect(){
    std::cout << "[CONNECTOR] Disconnecting" << std::endl;

    Message msg_ = {0, "EOC"};
    set_message(std::move(msg_));
    int cycle_counter_ = SOCKET_TIMEOUT_CYCLES;
    
    while(_sbuffer.size() != 0 && cycle_counter_-- > 0){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    _recv_socket.disconnect();
    _send_socket.disconnect();
}

bool Connector::set_message(Connector::Message&& msg_) noexcept{
    if(_sbuffer.size() == _send_buffer_size)
        return false;
    
    // std::cout << "[CONNECTOR] Adding msg_ to the _sbuffer" << std::endl;
    _sbuffer.push_back(msg_);
    return true;
}

Connector::Message Connector::get_message(){
    if(_rbuffer.size() == 0)
        return {0, ""};

    // std::cout << "[CONNECTOR] Getting _rbuffer.front()" << std::endl;
    Connector::Message res_ = _rbuffer.front();
    while(res_.id == 0){
        // std::cout << "[CONNECTOR] Checking msg_.id" << std::endl;
        _rbuffer.pop_front();
        res_ = _rbuffer.front();    
    }

    // std::cout << "[CONNECTOR] Deleting and returning msg_" << std::endl;
    _rbuffer.pop_front();
    return res_;
}

Connector::Message Connector::get_message(uint8_t id){
    if(_rbuffer.size() == 0)
        return {0, ""};

    Message res_ = {0,""};

    // std::cout << "[CONNECTOR] Looking for the msg_ with ID " << id << std::endl;
    for (Message& msg_ : _rbuffer){
        if(msg_.id == id){
            // std::cout << "[CONNECTOR] Found(setting {0,\"\"})" << std::endl;
            res_ = msg_;
            msg_ = {0,""};
            return res_;
        }
    }
    
    return res_;
}

int Connector::available() const noexcept{
    return _rbuffer.size();
}

Connector::~Connector(){
    disconnect();
}

Connector::Connector(int recv_buf_size_, int send_buf_size_, 
        int self_port_, const std::string& other_ip_, int other_port_):
    _rbuffer(),
    _recv_buffer_size(recv_buf_size_),
    _sbuffer(),
    _send_buffer_size(send_buf_size_),
    _recv_socket(self_port_),
    _send_socket(other_ip_, other_port_)
{    
    connect();
}

void Connector::_update(){
    std::cout << "[CONNECTOR] Starting working thread" << std::endl;
    while (_send_socket.is_connected() && _recv_socket.is_connected()){
        if(_sbuffer.size() == 0)
            _recv_message();
        else
            _send_message();
    
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }
}

void Connector::_recv_message(){
    char buffer_recv_[BUFFER_SIZE] = {0};

    timeval timeout_;
    timeout_.tv_sec = 0;  
    timeout_.tv_usec = SOCKET_TIMEOUT_US;

    setsockopt(_recv_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));

    // std::cout << "[CONNECTOR] Recieving message" << std::endl;
    int recv_res_ = recv(_recv_socket, buffer_recv_, sizeof(buffer_recv_), 0);

    if(recv_res_ >= 0){
        std::string str_ = buffer_recv_;
        if(str_ == "")
            return;
        
        Message msg_ = _decode(str_);

        if(msg_.id == 0 && msg_.message == "EOC"){
            std::cout << "[CONNECTOR] Disconnecting" << std::endl;

            _recv_socket.disconnect();
            _send_socket.disconnect();
            return;
        }
    
        // std::cout << "[CONNECTOR] Adding gotten message to the _rbuffer, message is " << str_ << std::endl;
        _rbuffer.push_back(msg_);
    }    
}

void Connector::_send_message(){
    // std::cout << "[CONNECTOR] Getting _sbuffer.front() and checking" << std::endl;
    Message msg_ = _sbuffer.front();
    
    if(msg_.message.length() > LENGTH_OF_MSG){
        _sbuffer.pop_front();
        return;
    }

    // std::cout << "[CONNECTOR] " << msg_.message << std::endl;

    std::string str_msg_ = _encode(msg_);
    _sbuffer.pop_front();

    // std::cout << "[CONNECTOR] Sending message " << str_msg_.length() << ' ' << str_msg_.c_str() << std::endl;
    send(_send_socket, str_msg_.c_str(), str_msg_.length(), 0);
}

std::string Connector::_encode(const Connector::Message& msg_){
    if(msg_.message.length() == 0)
        return "";

#ifdef CODEC
    // std::cout << "[CONNECTOR] Distinguishing symbols" << std::endl;
    std::map<char, std::vector<int>> symbols_;

    for (size_t ind_ = 0; ind_ < msg_.message.length(); ind_++){
        char char_ = msg_.message[ind_];

        if(!symbols_.count(char_))
            symbols_[char_] = std::vector<int>();

        symbols_[char_].push_back(ind_);
    }

    std::string res_;
    // std::cout << "[CONNECTOR] Adding ID" << std::endl;
    res_ += ~(msg_.id);
    res_ += END_OF_ID;

    // std::cout << "[CONNECTOR] Encoding symbols" << std::endl;
    for (const auto& node_ : symbols_){
        res_ += ~(node_.first);
        for (const auto& ind_ : node_.second){
            res_ += END_OF_INDEX;
            res_ += ~(ind_);
        }

        res_+=END_OF_CODE;
    }
    
    res_ += ~(msg_.message.length());
#else
    std::string res_;
    res_ = std::to_string(msg_.id) + " " + msg_.message;
#endif

    // std::cout << "[CONNECTOR] Returning encoded message" << std::endl;
    return res_;
}

Connector::Message Connector::_decode(std::string& str_){
    if(str_ == ""){
        Message msg_ = {0,""};
        return msg_;
    }

#ifdef CODEC
    // std::cout << "[CONNECTOR] Getting ID" << std::endl;
    uint8_t id_ = ~(str_[0]);
    str_ = str_.substr(2);

    int length_ = ~(str_.back());

    if(length_ > LENGTH_OF_MSG){
        Message msg_ = {0, ""};
        return msg_;
    }

    std::string res_;
    res_.reserve(length_);
    res_.resize(length_);
    
    // std::cout << "[CONNECTOR] Decoding message" << std::endl;
    while (str_.length() != 1){
        char char_ = ~(str_.front());
        int ind_ = 2;

        while (str_[ind_] != END_OF_CODE){
            if(str_[ind_] != END_OF_INDEX){
                int char_ind_ = ~(str_[ind_]);
                res_[char_ind_] = char_;
            }
            ind_++;
        }
        
        str_ = str_.substr(ind_+1);
    }

    Message msg_ = {id_, res_};
#else
    Message msg_ = {static_cast<uint8_t>(str_[0] - 48), str_.substr(2)};
#endif
    
    // std::cout << "[CONNECTOR] Returning decoded message" << std::endl;
    return msg_;
}