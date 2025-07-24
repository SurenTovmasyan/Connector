#include <iostream>
#include "../include/connector.h"

int main(){
    Connector c(10, 10, Socket::CLIENT, 15973, "192.168.2.19", 15974);

    std::cout << "sleep1" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    c.set_message({1,"Hello from exm1"});

    std::cout << "sleep2" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(8000));

    int i = 1;
    while(c.available() != 0){
        Connector::Message msg = c.get_message();
        std::cout << "HALLO   " << i++ << ' ' << msg.message << std::endl;
    }

    std::cout << "sleep3" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    return 0;
}