#include "../includes/connector.h"

// @TODO: add 3 classes - Socket, ConfigLoader, Codec
// 1. Socket - creates socket and closses automaticly
// 2. ConfigLoader - loads and configurates config.json, may work with JSON, YAML, XML
// 3. Codec - codes and decodes given string

constexpr const int END_OF_CODE = 127;
constexpr const int END_OF_INDEX = 126;
constexpr const int END_OF_ID = '>';
constexpr const int LENGTH_OF_MSG = 125;
constexpr const int BUFFER_SIZE = 4096;
constexpr const int THREADS_SLEEP_MS = 50;
constexpr const int SOCKET_TIMEOUT_US = 500 * 1000; // 500ms

std::shared_ptr<Connector> Connector::_singleton;

std::shared_ptr<Connector> Connector::get_instance(){
    if(!_singleton.get()){
       _singleton.reset(new Connector());
    }

    std::cout << "Returning instance" << std::endl;

    return _singleton; 
}

void Connector::connect(){
    if(_is_connected || _socket_fd != -1)
        return;

    std::cout << "Creating server and client connection threads" << std::endl;

    std::thread server_thread_([&]() { _create_server_socket(); });
    std::thread client_thread_([&]() { _create_client_socket(); });

    server_thread_.detach();
    client_thread_.detach();

    while(!_is_connected){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Creating recv and send threads" << std::endl;
    std::thread recv_thread_([&]() { _recv_update(); });
    std::thread send_thread_([&]() { _send_update(); });

    recv_thread_.detach();
    send_thread_.detach();
}

void Connector::disconnect(){
    if(!_is_connected)
        return;

    std::cout << "Disconnecting" << std::endl;

    Message msg_ = {0, "EOC"};
    set_message(msg_);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    _is_connected = false;
    _close_sockets();
}

bool Connector::is_connected() const noexcept{
    std::cout << "Returning _is_connected" << std::endl;
    return _is_connected;
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
    _is_connected(false),
    _server_socket_fd(-1),
    _client_socket_fd(-1),
    _socket_fd(-1),
    _config("../config.json")
{
    std::cout << "Loading config" << std::endl;

    _send_buffer_size = _config.get_config()["send_buffer_size"];
    _recv_buffer_size = _config.get_config()["recieve_buffer_size"];
    
    connect();
}

void Connector::_recv_update(){
    std::cout << "Starting recv thread" << std::endl;
    while (_is_connected){
        std::this_thread::sleep_for(std::chrono::milliseconds(THREADS_SLEEP_MS));
        _recv_message();
    }
}

void Connector::_send_update(){
    std::cout << "Starting send thread" << std::endl;
    while (_is_connected){
        if(_sbuffer.size() == 0){
            std::this_thread::sleep_for(std::chrono::milliseconds(THREADS_SLEEP_MS));
            std::cout << "_sbuffer is empty" << std::endl;
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(THREADS_SLEEP_MS));
            _send_message();
        }
    }
}

void Connector::_create_server_socket(){
    if (_is_connected)
        return;

    std::cout << "[SERVER] Getting server socket and configurating IP address" << std::endl;
    _server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr_{};

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(_config.get_config()["self_connector_port"]);
    addr_.sin_addr.s_addr = INADDR_ANY;

    std::cout << "[SERVER] Binding and listening" << std::endl;
    bind(_server_socket_fd, (sockaddr*)&addr_, sizeof(addr_));
    listen(_server_socket_fd, 1);
        
    std::cout << "[SERVER] Setting timeout" << std::endl;
    timeval timeout_{};
    timeout_.tv_sec = 0;
    timeout_.tv_usec = SOCKET_TIMEOUT_US; 

    while (!_is_connected) {
        fd_set readfds_;
        FD_ZERO(&readfds_);
        FD_SET(_server_socket_fd, &readfds_);

        std::cout << "[SERVER] Checking available connections" << std::endl;
        int result_ = select(_server_socket_fd + 1, &readfds_, nullptr, nullptr, &timeout_);

        if (result_ > 0 && FD_ISSET(_server_socket_fd, &readfds_)) {
            sockaddr_in client_addr_{};
            socklen_t client_len_ = sizeof(client_addr_);

            std::cout << "[SERVER] Accepting other side" << std::endl;
            int client_fd_ = accept(_server_socket_fd, (sockaddr*)&client_addr_, &client_len_);

            if (!_is_connected) {
                std::cout << "[SERVER] Setting socket SERVER" << std::endl;
                _socket_fd = client_fd_;
                _is_connected = true;
            } else {
                close(client_fd_);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(200));
    }

    std::cout << "[SERVER] Closing server socket" << std::endl;
    close(_server_socket_fd);
}

void Connector::_create_client_socket(){
    if(_is_connected)
        return;

    std::cout << "[CLIENT] Waiting some time before connecting other side" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[CLIENT] Configurating other side's ip and port" << std::endl;
    sockaddr_in peer_addr_{};
    peer_addr_.sin_family = AF_INET;
    peer_addr_.sin_port = htons(_config.get_config()["other_connector_port"]);

    inet_pton(AF_INET, ((std::string)(_config.get_config()["other_connector_ip"])).c_str(), &peer_addr_.sin_addr);

    while(!_is_connected){
        std::cout << "[CLIENT] Trying to connect to the other side" << std::endl;
        int temp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

        if (::connect(temp_socket_, (sockaddr*)&peer_addr_, sizeof(peer_addr_)) == 0) {
            if (!_is_connected) {
                std::cout << "[CLIENT] Setting socket CLIENT" << std::endl;
                _socket_fd = temp_socket_;
                _is_connected = true;
            }
            else {
                close(temp_socket_);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Connector::_close_sockets(){
    std::cout << "Closing socket" << std::endl;

    close(_server_socket_fd);
    close(_client_socket_fd);
    close(_socket_fd);
    
    _socket_fd = -1;
    _server_socket_fd = -1;
    _client_socket_fd = -1;
}

void Connector::_recv_message(){
    char buffer_recv_[BUFFER_SIZE] = {0};

    timeval timeout_;
    timeout_.tv_sec = 0;  
    timeout_.tv_usec = SOCKET_TIMEOUT_US;

    setsockopt(_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));

    std::cout << "Recieving message" << std::endl;
    int recv_res_ = recv(_socket_fd, buffer_recv_, sizeof(buffer_recv_), 0);

    if(recv_res_ >= 0){
        std::string str_ = buffer_recv_;
        if(str_ == "")
            return;
        
        Message msg_ = _decode(str_);


        if(msg_.id == 0 && msg_.message == "EOC")
            disconnect();
    
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
    for (size_t i = 0; i < str_msg_.length(); i++){
        buffer_send_[i] = str_msg_[i];
    }
    

    std::cout << "Sending message " << str_msg_.length() << ' ' << buffer_send_ << std::endl;
    send(_socket_fd, buffer_send_, str_msg_.length(), 0);
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