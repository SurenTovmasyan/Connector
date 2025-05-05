#include "../includes/connector.h"

// @TODO: add 3 classes - Socket, ConfigLoader, Codec
// 3. Codec - codes and decodes given string

constexpr const int END_OF_CODE = 127;
constexpr const int END_OF_INDEX = 126;
constexpr const int END_OF_ID = '>';
constexpr const int LENGTH_OF_MSG = 125;
constexpr const int BUFFER_SIZE = 4096;
constexpr const int THREAD_SLEEP_MS = 25;
constexpr const int SOCKET_TIMEOUT_US = 50 * 1000; // 50ms
constexpr const int SOCKET_TIMEOUT_CYCLES = 50; // 50 * 100ms = 5s

std::shared_ptr<Connector> Connector::_singleton;

std::shared_ptr<Connector> Connector::get_instance(){
    if(!_singleton.get()){
       _singleton.reset(new Connector());
    }

    std::cout << "Returning instance" << std::endl;

    return _singleton; 
}

void Connector::connect(){
    std::cout << "Trying to connect" << std::endl;

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
        std::cout << "Timeout worked, couldn't connect" << std::endl;
        return;
    }

    std::cout << "Creating recv and send threads" << std::endl;
    std::thread recv_thread_([&]() { _recv_update(); });
    std::thread send_thread_([&]() { _send_update(); });

    recv_thread_.detach();
    send_thread_.detach();
}

void Connector::disconnect(){
    std::cout << "Disconnecting" << std::endl;

    Message msg_ = {0, "EOC"};
    set_message(msg_);
    
    while(_sbuffer.size() != 0){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    _recv_socket.disconnect();
    _send_socket.disconnect();
}

bool Connector::set_message(Connector::Message& msg_) noexcept{
    if(_sbuffer.size() == _send_buffer_size)
        return false;
    
    std::cout << "Adding msg_ to the _sbuffer" << std::endl;
    _sbuffer.push_back(msg_);
    return true;
}

Connector::Message Connector::get_message(){
    if(_rbuffer.size() == 0)
        throw std::runtime_error("Recieve buffer is empty");

    std::cout << "Getting _rbuffer.front()" << std::endl;
    Connector::Message res_ = _rbuffer.front();
    while(res_.id == 0){
        std::cout << "Checking msg_.id" << std::endl;
        _rbuffer.pop_front();
        res_ = _rbuffer.front();    
    }

    std::cout << "Deleting and returning msg_" << std::endl;
    _rbuffer.pop_front();
    return res_;
}

Connector::Message Connector::get_message(uint8_t id){
    if(_rbuffer.size() == 0)
        throw std::runtime_error("Recieve buffer is empty");

    Message res_ = {0,""};

    std::cout << "Looking for the msg_ with ID " << id << std::endl;
    for (Message& msg_ : _rbuffer){
        if(msg_.id == id){
            std::cout << "Found(setting {0,\"\"})" << std::endl;
            res_ = msg_;
            msg_ = {0,""};
            return res_;
        }
    }
    
    return res_;
}

Connector::~Connector(){
    disconnect();
}

Connector::Connector():
    _rbuffer(),
    _recv_buffer_size(0),
    _sbuffer(),
    _send_buffer_size(0),
    _recv_socket("../config.json"),
    _send_socket("../config.json"),
    _config("../config.json")
{
    std::cout << "Loading config" << std::endl;

    _send_buffer_size = _config.get_config()["send_buffer_size"];
    _recv_buffer_size = _config.get_config()["recieve_buffer_size"];
    
    connect();
}

void Connector::_recv_update(){
    std::cout << "Starting recv thread" << std::endl;
    while (_recv_socket.is_connected()){
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        _recv_message();
    }
}

void Connector::_send_update(){
    std::cout << "Starting send thread" << std::endl;
    while (_send_socket.is_connected()){
        if(_sbuffer.size() == 0)
            std::cout << "_sbuffer is empty" << std::endl;
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

    std::cout << "Recieving message" << std::endl;
    int recv_res_ = recv(_recv_socket, buffer_recv_, sizeof(buffer_recv_), 0);

    if(recv_res_ >= 0){
        std::string str_ = buffer_recv_;
        if(str_ == "")
            return;
        
        Message msg_ = _decode(str_);

        if(msg_.id == 0 && msg_.message == "EOC"){
            std::cout << "Disconnecting" << std::endl;

            _recv_socket.disconnect();
            _send_socket.disconnect();
            return;
        }
    
        std::cout << "Adding gotten message to the _rbuffer, message is" << str_ << std::endl;
        _rbuffer.push_back(msg_);
    }    
}

void Connector::_send_message(){
    std::cout << "Getting _sbuffer.front() and checking" << std::endl;
    Message msg_ = _sbuffer.front();
    if(msg_.message.length() > LENGTH_OF_MSG){
        _sbuffer.pop_front();
        return;
    }

    std::cout << msg_.message << std::endl;

    std::string str_msg_ = _encode(msg_);
    _sbuffer.pop_front();

    char buffer_send_[BUFFER_SIZE];

    std::cout << "Copying encoded message into buffer" << std::endl;
    std::strcpy(buffer_send_, str_msg_.c_str());

    std::cout << "Sending message " << str_msg_.length() << ' ' << buffer_send_ << std::endl;
    send(_send_socket, buffer_send_, str_msg_.length(), 0);
}

std::string Connector::_encode(const Connector::Message& msg_){
    if(msg_.message.length() == 0)
        return "";

    std::cout << "Distinguishing symbols" << std::endl;
    std::map<char, std::vector<int>> symbols_;

    for (size_t ind_ = 0; ind_ < msg_.message.length(); ind_++){
        char char_ = msg_.message[ind_];

        if(!symbols_.count(char_))
            symbols_[char_] = std::vector<int>();

        symbols_[char_].push_back(ind_);
    }

    std::string res_;
    std::cout << "Adding ID" << std::endl;
    res_ += ~(msg_.id);
    res_ += END_OF_ID;

    std::cout << "Encoding symbols" << std::endl;
    for (const auto& node_ : symbols_){
        res_ += ~(node_.first);
        for (const auto& ind_ : node_.second){
            res_ += END_OF_INDEX;
            res_ += ~(ind_);
        }

        res_+=END_OF_CODE;
    }
    
    res_ += ~(msg_.message.length());

    std::cout << "Returning encoded message" << std::endl;
    return res_;
}

Connector::Message Connector::_decode(std::string& str_){
    if(str_ == ""){
        Message msg_ = {0,""};
        return msg_;
    }

    std::cout << "Getting ID" << std::endl;
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
    
    std::cout << "Decoding message" << std::endl;
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
    
    std::cout << "Returning decoded message" << std::endl;
    return msg_;
}