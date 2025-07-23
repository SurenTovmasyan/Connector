#include <iostream>
#include "../include/connector.h"

int main(){
    Connector c(10,10,15973,"192.168.2.19",15973);

    std::cout << "sleep1" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(7000));

    Connector::Message msg = c.get_message();
    std::cout << "HALLO   " << msg.id << ' ' << msg.message << std::endl;

    std::cout << "sleep2" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    for (size_t i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Sending " << i+1 << std::endl;
        c.set_message({1,"Hello from exm2"});
    }
    
    std::cout << "sleep3" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    return 0;
}