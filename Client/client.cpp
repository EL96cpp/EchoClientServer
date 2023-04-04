#include <iostream>
#include <string>

#include "client.hpp"

int main() {    

    Client c;
    c.Connect("127.0.0.1", 60000);
    while (c.IsConnected()) {
        
        c.GetKey();
        c.Send();
    
        if (c.IsConnected()) {

            c.Receive();
    
        } else {
        
            std::cout << "Client is disconnected!\n";
        
        }
    }

    return 0;
}
