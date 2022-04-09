#pragma once

#include "methods.hpp"
#include "socket_lib/shared/SocketHandler.hpp"

struct IncomingPacket {
    std::stringstream data;
    size_t expectedLength;
    size_t currentLength; // should we do this?

    [[nodiscard]] constexpr bool isValid() const {
        return expectedLength > 0;
    }

    IncomingPacket(size_t expectedLength) : data(expectedLength), expectedLength(expectedLength) {}

    // by default, invalid packet
    explicit IncomingPacket() : IncomingPacket(0) {}
};

class Manager {
    private:
    void connectEvent(SocketLib::Channel& channel, bool connected);
    void listenOnEvents(SocketLib::Channel& client, const SocketLib::Message& message);

    void sendMessage(std::string_view s);
    void sendMessage(std::span<SocketLib::byte> message);

    void processMessage(std::string const message);
    void processRun(std::string command);
    void processRunRaw(std::string command);
    void processLoad(std::string_view const command);
    void processFind(std::string command);

    bool awaitingMessage = false;
    std::function<void(std::string_view)> nextMessageCallback;

    bool processingMessage = false;
    std::vector<std::string> queuedMessages;

    void sendResult(std::string_view value, std::string_view classTypeName = "");
    // separating seems difficult
    void setAndSendObject(class Il2CppObject* object);

    SocketLib::ServerSocket* serverSocket;
    bool initialized;

    std::unordered_map<SocketLib::Channel*, IncomingPacket> channelIncomingQueue;

    Il2CppObject* object;
    std::vector<Method> methods;

    public:
    void Init();
    void SetObject(class Il2CppObject* object);
    void RunMethod(int methodIdx, std::vector<std::string> args);

    static Manager* Instance;
};