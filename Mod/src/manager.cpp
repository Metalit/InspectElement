#include "manager.hpp"
#include "classutils.hpp"
#include "main.hpp"

using namespace SocketLib;
using namespace ClassUtils;

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
    if(!client) return;

    object = obj;
    methods.clear();
    
    auto klass = classofinst(object);
    auto methodInfos = getMethods(klass);
    // convert to our method type
    for(auto& methodInfo : methodInfos) {
        // would rather emplace the whole object, but then it might have to be removed
        auto method = Method(object, methodInfo);
        // if(!method.hasNonSimpleParam)
            methods.emplace_back(method);
    }
    // todo: fields

    // send info
    for(auto& method : methods) {
        client->queueWrite(Message(method.infoString()));
    }
}

void Manager::connectEvent(Channel& channel, bool connected) {
    LOG_INFO("Connected %i status: %s", channel.clientDescriptor, connected ? "connected" : "disconnected");
    client = &channel;

    if(connected)
        client->queueWrite(Message("Successfully connected\n"));
    else
        client = nullptr;
}

void Manager::listenOnEvents(Channel& client, const Message& message) {
    auto msgStr = message.toString();
    LOG_INFO("Received: %s", msgStr.c_str());

    // client.queueWrite(message);
}
