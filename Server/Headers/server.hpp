#pragma once 


#include <deque>
#include <thread>
#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <sstream>

#include <boost/asio.hpp>

#include "tsqueue.hpp"
#include "message.hpp"

using namespace boost;


// ClientConnestion structure, it will help server to store clients
struct ClientConnection : std::enable_shared_from_this<ClientConnection> {

public:
    ClientConnection(asio::ip::tcp::socket socket, asio::io_context& context, 
                     tsqueue<owned_message>& messagesIn) : socket(std::move(socket)),
                                                           messagesIn(messagesIn),
                                                           context(context) {}

    bool IsConnected() {
        return socket.is_open();
    }

    void ReadHeader() {
        asio::async_read(socket, asio::buffer(&tmpMessage.header, sizeof(message_header)),
            [this](std::error_code ec, std::size_t length) {
                if (!ec) {
                    tmpMessage.body.resize(tmpMessage.header.size);
                    ReadBody();
                } else {
                    std::cout << socket.remote_endpoint() << " error reading message size\n";
                    socket.close();
                }
            });
    }

    void ReadBody() {
        asio::async_read(socket, asio::buffer(tmpMessage.body.data(), tmpMessage.body.size()),
            [this](std::error_code ec, std::size_t length){
                if (!ec) {

                    // Message was read, so we can add it to messagesIn. 
                    AddToMessages();
                } else {
                    std::cout << socket.remote_endpoint() << " error reading message size\n";
                    socket.close();
                }
            });
        
    }
    
    // Method to construct and add owned_message to the message-queue. 
    // Server will respond on each owned_message inside 'Update' method
    void AddToMessages() {
        owned_message tmpOwned(this->shared_from_this(), tmpMessage);
        messagesIn.push_back(tmpOwned);
        ReadHeader();
    }

    // Method, called inside 'Update' method of Server
    // Using this method, we'll send data back to the client
    void Send(const message& msg) {
        asio::post(context, [this, msg]() {

                // We push message inside queue of output messages
                // If for some reason at this moment another thread will be writing messages to the socket,
                // we don't need to call WriteHeader() - previous thead won't leave 
                // untill it sends all of the messages, including the one we're currently pushing
                bool bWritingMessage = !messagesOut.empty();
                messagesOut.push_back(msg);
                if (!bWritingMessage) {
                    WriteHeader();
                }
            });
    }


    void WriteHeader() {
        asio::async_write(socket, asio::buffer(&messagesOut.front().header, sizeof(message_header)),
            [this](std::error_code ec, std::size_t length) {
                if (!ec) {
                    WriteBody();
                } else {
                    std::cout << "Send messge error: " << ec.message() << "\n";
                    socket.close();
                }
            });
    }

    void WriteBody() {

        asio::async_write(socket, asio::buffer(messagesOut.front().body.data(), 
                                               messagesOut.front().header.size),
            [this](std::error_code ec, std::size_t length) {
                if (!ec) {
                    messagesOut.pop_front();

                    // In case another thread pushed new message, while we were 
                    // sending our message, we'll send another thread's message too
                    if (!messagesOut.empty()) {
                        WriteHeader();
                    }
                } else {
                    std::cout << "Write body error: " << ec.message() << "\n";
                    socket.close();
                }
            });
    }

private:
    asio::io_context& context;
    asio::ip::tcp::socket socket;

    // Reference to the Server's tsqueue of owned messages
    // Each ClientConnection will push it's messages inside this queue
    tsqueue<owned_message>& messagesIn;

    // Queue of otput messages
    tsqueue<message> messagesOut;
    // Temporary message to store incoming data
    message tmpMessage;  
};


class Server {

public:
    Server(const size_t& port) : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
    
    ~Server() {

        context.stop();
        if (thread.joinable()) thread.join();
        std::cout << "Server stopped!\n";
    }

    bool Start() {
        try {
            
            // We call recursive method WaitForClients, so io_context will never 
            // run out of tasks and stop
            WaitForClients();
            thread = std::thread([this]() { context.run(); });
        
        } catch(std::exception& e) {
            
            std::cout << "Exception: " << e.what() << "\n";
            return false;
        }
        std::cout << "Server started!\n";
        return true;
    }

    // Method, accepting new clients and pushing it into deque of ClientConnections
    void WaitForClients() {
        acceptor.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket) {
                    if (!ec) {
                        std::cout << "Server new connection: " << socket.remote_endpoint() << "\n";

                        std::shared_ptr<ClientConnection> newconn = 
                                std::make_shared<ClientConnection>(std::move(socket), context, messagesIn);
                        connections.push_back(std::move(newconn));
                        connections.back()->ReadHeader();
                    
                    } else {
                        std::cout << "Connection error: " << ec.message() << "\n";
                    }

                    WaitForClients();
                });
    }

    // Method, responding to incoming messages 
    // If messagesIn is empty, we will wait, untill another 
    // thread won't push back some data and notify us
    void Update() {

        messagesIn.wait();
        while (!messagesIn.empty()) {
            auto msg = messagesIn.pop_front();
            RespondToMessage(msg.remote, msg.msg);
            
        }
    }

    // Method to handle invalid clients and remove them from deque of connections
    void UpdateClients() {

        bool bInvalidClients = false;
        for (auto& client : connections) {
            if (!client->IsConnected()) {
                client.reset();
                bInvalidClients = true;
            }
        }

        // Erase-remove idiom
        if (bInvalidClients) {
            connections.erase(
                std::remove(connections.begin(), connections.end(), nullptr), connections.end());
        }
    }

    // Reverse string, if client whants the server to respond with reversed message
    void ReverseString(message& msg) {
        std::reverse(msg.body.begin(), msg.body.end());
    }


    void RespondToMessage(std::shared_ptr<ClientConnection> client, message& msg) {
        switch (msg.header.id) {
            case MsgType::InOrder:
                client->Send(msg);
                break;
            case MsgType::Reverse:
                ReverseString(msg);
                client->Send(msg);
                break;
        }
    }


private:
    std::deque<std::shared_ptr<ClientConnection>> connections;
    asio::io_context context;
    std::thread thread;
    asio::ip::tcp::acceptor acceptor;
    tsqueue<owned_message> messagesIn;
};

