#include <iostream>
#include "connector.h"

int main(){
    Connector c;

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    std::string msg = c.recv_message();

    std::cout << msg << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    return 0;
}