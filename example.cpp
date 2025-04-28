#include <iostream>
#include <cstring>
#include <future>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>
#include <unistd.h>
#include "includes/connector.h"

using namespace std;

int main(){
    std::shared_ptr<Connector> pc = Connector::get_instance();

    Connector::Message msg_ = {125, "Taug taug taug SAHUR!!!"};

    pc->set_message(msg_);

    this_thread::sleep_for(chrono::seconds(30));
    return 0;
}