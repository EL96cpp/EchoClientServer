#include <iostream>
#include <string>

#include "server.hpp"

int main() {
    Server s(60000);
    s.Start();
    while(true) {
        s.Update();    
        s.UpdateClients();
    }
    return 0;
}
