#include <iostream>
#include "connector.h"

int main(){
    Connector c;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    c.send_message("Hello from sender");

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    return 0;
}