#pragma once

#include "methods.hpp"
#include "socket_lib/shared/SocketHandler.hpp"

class Manager {
    private:
    void connectEvent(SocketLib::Channel& channel, bool connected);
    void listenOnEvents(SocketLib::Channel& client, const SocketLib::Message& message);

    SocketLib::ServerSocket* serverSocket;
    SocketLib::Channel* client;
    bool initialized;

    Il2CppObject* object;
    std::vector<Method> methods;

    public:
    void Init();
    void SetObject(class Il2CppObject* object);

    static Manager* Instance;
};