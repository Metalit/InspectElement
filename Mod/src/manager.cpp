#include "manager.hpp"
#include "classutils.hpp"
#include "main.hpp"

using namespace SocketLib;

Manager* Manager::Instance = nullptr;

void Manager::Init() {
    Manager::Instance = this;
    initialized = true;
    LOG_INFO("Starting server at port 3306");
    SocketHandler& socketHandler = SocketHandler::getCommonSocketHandler();

    serverSocket = socketHandler.createServerSocket(3306);
    serverSocket->bindAndListen();
    LOG_INFO("Started server");

    ServerSocket& serverSocket = *this->serverSocket;
    
    serverSocket.connectCallback += [this](Channel& client, bool connected){
        connectEvent(client, connected);
    };

    serverSocket.listenCallback += [this](Channel& client, const Message& message){
        listenOnEvents(client, message);
    };

    LOG_INFO("Server fully initialized");
}

void Manager::SetObject(Il2CppObject* obj) {
    if(!initialized) return;

    
}

void Manager::connectEvent(Channel& channel, bool connected) {
    LOG_INFO("Connected %i status: %s", channel.clientDescriptor, connected ? "connected" : "disconnected");
    this->connected = connected;
    client = &channel;

    if(connected)
        channel.queueWrite(Message("Hello!/n"));
    else
        client = nullptr;
}

void Manager::listenOnEvents(Channel& client, const Message& message) {
    auto msgStr = message.toString();
    LOG_INFO("Received: %s", msgStr.c_str());

    // client.queueWrite(message);
}
