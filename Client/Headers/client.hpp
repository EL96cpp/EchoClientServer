#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#include "message.hpp"

using namespace boost;


// Synchronous client for echo-server

class Client {

public:
    
    Client() : socket(context) {}

    void Connect(const std::string& ip, const size_t& port) {

        try {
            
            socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port));

        } catch(std::error_code& ec) {

            std::cout << "Connection error: " << ec.message() << "\n";
        
        }
    }

    bool IsConnected() {
        return socket.is_open();
    }

    // This functions will help us to know, which type of echo-response
    // user whants to get
    void GetKey() {

        std::cout << "Enter integer:\n1)Get in order response\n2)Get reverse response\n";
        std::cin >> key;
        if (key != 1 && key != 2) {
            std::cout << "Invalid integer!\n";
            GetKey();
        }
    }

    // Get message and call WriteMessage to send it to the server
    void Send() {
        
        std::cout << "Enter message to send:\n";
        std::cin >> str;
        messageOut << str;
        switch(key) {
            case 1:
                messageOut.header.id = MsgType::InOrder;
                break;

            case 2:
                messageOut.header.id = MsgType::Reverse;
                break;
        }
        WriteMessage();
    }

    void WriteMessage() {
        asio::write(socket, asio::buffer(&messageOut.header, sizeof(message_header)));
        asio::write(socket, asio::buffer(messageOut.body.data(), messageOut.body.size()));

        // We need to clear messageOut, so the next time we will whant to 
        // send message to the server, messageOut will be ready to store new data
        messageOut.clear();
    }


    void Receive() {

        // Read header, get size and prepare space for incoming data
        size_t read_header = 0;
        while (read_header < sizeof(message_header)) {
            read_header += socket.read_some(asio::buffer(&messageIn.header, sizeof(message_header))); 
        }
        messageIn.body.resize(messageIn.header.size);
        
        // Read body of message
        size_t read_body = 0;
        while (read_body < messageIn.header.size) {
            read_body += socket.read_some(asio::buffer(messageIn.body.data(), messageIn.body.size()));
        }

        // Size of messageIn will be 0, and body will be empty and ready for new data
        // after this output operation
        messageIn >> str;
        std::cout << "Response: " << str << "\n\n\n";
        
        // Prepare str for the next iteration.
        str.clear();
    }
    
private:
    asio::io_context context;
    std::thread thread;
    asio::ip::tcp::socket socket;
    std::string str;
    message messageOut;
    message messageIn;
    size_t key;
};



