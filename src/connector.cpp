#include "../include/connector.h"

// @TODO: add 3 classes - Socket, ConfigLoader, Codec
// 3. Codec - codes and decodes given string

#ifdef Codec
constexpr const int END_OF_CODE = 127;
constexpr const int END_OF_INDEX = 126;
constexpr const int END_OF_ID = '>';
#endif

constexpr const int LENGTH_OF_MSG = 125;
constexpr const int SOCKET_TIMEOUT_CYCLES = 50;
constexpr const int THREAD_SLEEP_MS = 25;

void Connector::connect(){
    std::cout << "[CONNECTOR] Trying to connect" << std::endl;

    _socket.connect();

    int cycle_counter_ = 0;
    while(!_socket.is_connected() && cycle_counter_++ < SOCKET_TIMEOUT_CYCLES)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if(!_socket.is_connected()){
        _socket.disconnect();
        std::cout << "[CONNECTOR] Timeout worked, couldn't connect" << std::endl;
        std::exit(-1);
    }

    std::cout << "[CONNECTOR] Creating working thread" << std::endl;
    _thrd = std::thread([&]() { _update(); });
}

void Connector::disconnect(){
    std::cout << "[CONNECTOR] Disconnecting" << std::endl;

    Message msg_ = {0, "EOC"};
    set_message(std::move(msg_));
    int cycle_counter_ = SOCKET_TIMEOUT_CYCLES;
    
    while(_sbuffer.size() != 0 && cycle_counter_-- > 0){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    _socket.disconnect();
}

bool Connector::set_message(Connector::Message&& msg_) noexcept{
    std::lock_guard<std::mutex> lock_(_smtx);
    if(_sbuffer.size() == _send_buffer_size)
        return false;
    
    // std::cout << "[CONNECTOR] Adding msg_ to the _sbuffer" << std::endl;
    _sbuffer.push_back(msg_);
    return true;
}

Connector::Message Connector::get_message(){
    std::lock_guard<std::mutex> lock_(_rmtx);
    if(_rbuffer.size() == 0)
        return {0, ""};

    // std::cout << "[CONNECTOR] Getting _rbuffer.front()" << std::endl;
    Connector::Message res_ = _rbuffer.front();
    while(res_.id == 0){
        // std::cout << "[CONNECTOR] Checking msg_.id" << std::endl;
        _rbuffer.pop_front();
        if(_rbuffer.size() != 0)
            res_ = _rbuffer.front(); 
        else 
            return res_;
    }

    // std::cout << "[CONNECTOR] Deleting and returning msg_" << std::endl;
    _rbuffer.pop_front();
    return res_;
}

Connector::Message Connector::get_message(uint8_t id){
    std::lock_guard<std::mutex> lock_(_rmtx);
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

Connector::Connector(int recv_buf_size_, int send_buf_size_, Socket::Socket_Type type_, \
        int self_port_, const std::string& other_ip_, int other_port_)
    : _rbuffer()
    , _recv_buffer_size(recv_buf_size_)
    , _sbuffer()
    , _send_buffer_size(send_buf_size_)
    , _socket(type_, self_port_, other_ip_, other_port_)
{    
    connect();
}

void Connector::_update(){
    std::cout << "[CONNECTOR] Starting working thread" << std::endl;
    while (_socket.is_connected()){
        if(_sbuffer.size() == 0)
            _recv_message();
        else
            _send_message();
    
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
    }
}

void Connector::_recv_message(){
    char buffer_recv_[LENGTH_OF_MSG] = {0};

    timeval timeout_;
    timeout_.tv_sec = 0;  
    timeout_.tv_usec = Socket::SOCKET_TIMEOUT_US;

    setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));

    // std::cout << "[CONNECTOR] Recieving message" << std::endl;
    int recv_res_ = recv(_socket, buffer_recv_, sizeof(buffer_recv_), 0);

    if(recv_res_ >= 0){
        std::string str_ = buffer_recv_;
        if(str_ == "")
            return;
        
        Message msg_ = _decode(str_);

        if(msg_.id == 0 && msg_.message == "EOC"){
            disconnect();
            return;
        }
    
        // std::cout << "[CONNECTOR] Adding gotten message to the _rbuffer, message is " << str_ << std::endl;
        std::lock_guard<std::mutex> lock_(_rmtx);
        _rbuffer.push_back(msg_);
    }    
}

void Connector::_send_message(){
    std::string str_msg_ ;
    // std::cout << "[CONNECTOR] Getting _sbuffer.front() and checking" << std::endl;
    {
        std::lock_guard<std::mutex> lock_(_smtx);
        Message msg_ = _sbuffer.front();
        
        if(msg_.message.length() > LENGTH_OF_MSG){
            _sbuffer.pop_front();
            return;
        }

        // std::cout << "[CONNECTOR] " << msg_.message << std::endl;

        str_msg_ = _encode(msg_);
        _sbuffer.pop_front();
    }

    // std::cout << "[CONNECTOR] Sending message " << str_msg_.length() << ' ' << str_msg_.c_str() << std::endl;
    send(_socket, str_msg_.c_str(), str_msg_.length(), 0);
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