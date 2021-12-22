#pragma once

#include "methods.hpp"
#include "socket_lib/shared/SocketHandler.hpp"

class Manager {
    private:
    void connectEvent(SocketLib::Channel& channel, bool connected);
    void listenOnEvents(SocketLib::Channel& client, const SocketLib::Message& message);

    void processMessage(std::string message);
    void processRun(std::string command);
    void processRunRaw(std::string command);
    void processLoad(std::string command);

    bool awaitingMessage = false;
    std::function<void(std::string)> nextMessageCallback;

    void sendResult(std::string value, std::string classTypeName = "");
    // separating seems difficult
    void setAndSendObject(class Il2CppObject* object);

    SocketLib::ServerSocket* serverSocket;
    SocketLib::Channel* client;
    bool initialized, connected;

    std::stringstream currentMessage;

    Il2CppObject* object;
    std::vector<Method> methods;

    public:
    void Init();
    void SetObject(class Il2CppObject* object);
    void RunMethod(int methodIdx, std::vector<std::string> args);

    static Manager* Instance;
};