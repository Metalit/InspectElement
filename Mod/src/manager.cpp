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
    if(!connected) return;

    object = obj;
    methods.clear();
    
    auto klass = classofinst(object);
    auto methodInfos = getMethods(klass);
    // convert to our method type
    for(auto& methodInfo : methodInfos) {
        // would rather emplace the whole object, but then it might have to be removed
        auto method = Method(object, methodInfo);
        if(!method.hasNonSimpleParam)
            methods.emplace_back(method);
    }
    // todo: fields

    // just one parent for now
    auto parent = getParent(klass);
    if(parent) {
        auto methodInfos = getMethods(parent);
        for(auto& methodInfo : methodInfos) {
            auto method = Method(object, methodInfo);
            if(!method.hasNonSimpleParam)
                methods.emplace_back(method);
        }
    }

    // send info
    for(int i = 0; i < methods.size(); i++) {
        client->queueWrite(Message(std::to_string(i) + " " + methods[i].infoString()));
    }
    LOG_INFO("Object set");
}

void Manager::connectEvent(Channel& channel, bool connected) {
    LOG_INFO("Connected %i status: %s", channel.clientDescriptor, connected ? "connected" : "disconnected");
    this->connected = connected;
    client = &channel;

    if(connected)
        client->queueWrite(Message("Successfully connected\n"));
    else
        client = nullptr;
}

std::vector<std::string> parse(std::string& str, std::string delimiter) {
    std::vector<std::string> ret = {};
    int start = 0;
    auto end = str.find(delimiter);
    while (end != std::string::npos) {
        ret.emplace_back(str.substr(start, end - start));
        start = end + delimiter.size();
        end = str.find(delimiter, start);
    }
    ret.emplace_back(str.substr(start, end - start));
    return ret;
}

void Manager::listenOnEvents(Channel& client, const Message& message) {
    auto msgStr = message.toString();
    LOG_INFO("Received: %s", msgStr.c_str());

    auto vec = parse(msgStr, ",");
    int idx = std::stoi(vec[0]);
    vec.erase(vec.begin());
    
    if(idx < methods.size() && idx >= 0) {
        std::string out = "";
        auto res = methods[idx].run(vec, &out);
        if(out.length() > 0) {
            client.queueWrite(Message("Returned: " + out + "\n"));
        } else if(res) {
            client.queueWrite(Message("Returned new object\n"));
            SetObject(res);
        } else
            client.queueWrite(Message("Returned\n"));
    } else {
        client.queueWrite(Message("Invalid method"));
    }
}
