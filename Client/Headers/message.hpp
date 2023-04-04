#pragma once 

#include <string>
#include <vector>
#include <cstring>
#include <memory>


// Forward declaration of Connection class
class Connection;

// Type of messages to let echo-server know,
// which type of response client wants: in order or reverse
enum MsgType {
   
    InOrder,
    Reverse

};


// Field id stores type of the message
// Field size stores the size of the message's std::vector<char>

struct message_header {
    
    MsgType id;
    size_t size = 0;

};


// Struct, which contains header, which will provide information about 
// type and size of message and std::vector<char> - body of message

class message {

public:

    message_header header;
    std::vector<char> body;

    
    // Pushes char data from string into message
    friend message& operator << (message& msg, const std::string& str) {
        
        // Cache current size of vector, so we'll have point to insert data
        size_t i = msg.body.size();

        msg.body.resize(msg.body.size() + str.length());
        std::memcpy(msg.body.data() + i, str.data(), str.length());
        msg.header.size = msg.body.size();
        return msg;
    }

    // Outputs whole char data into one string
    friend message& operator >> (message& msg, std::string& str) {
        
        std::memcpy(str.data(), msg.body.data(), msg.header.size);
        msg.body.clear();
        msg.header.size = 0;
        return msg;
    }

    void clear() {
        body.clear();
        header.size = 0;
    }
};


// Owned message is identical to message, but it also has a field "remote", which is shared_ptr 
// to connection object. We'll use this type only in server class to help it identify, 
// which client ownes the message
struct owned_message {

    owned_message(std::shared_ptr<Connection>& remote, message& msg) : remote(remote), msg(msg) {}

    std::shared_ptr<Connection> remote;
    message msg;

};
