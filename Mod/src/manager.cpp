#include "manager.hpp"
#include "classutils.hpp"
#include "main.hpp"

#define MESSAGE_LOGGING

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
    setAndSendObject(obj);
}

void Manager::RunMethod(int methodIdx, std::vector<std::string> args) {
    if(methodIdx < methods.size() && methodIdx >= 0) {
        scheduleFunction([this, args, methodIdx](){
            std::string out = "";
            auto res = methods[methodIdx].run(args, &out);
            // check if we had a non pointer type output
            if(out.length() > 0) {
                sendResult(out);
            } else if(res) {
                // convert pointer to string
                std::string resultString = std::to_string(*(std::uintptr_t*)(&res));
                sendResult(resultString, il2cpp_functions::class_get_name(classofinst(res)));
            } else {
                LOG_INFO("Run returned nothing"); // either a failure or a field set
                sendResult("");
            }
        });
    } else {
        LOG_INFO("Invalid run index");
        sendResult("");
    }
}

void Manager::connectEvent(Channel& channel, bool connected) {
    LOG_INFO("Connected %i status: %s", channel.clientDescriptor, connected ? "connected" : "disconnected");
    this->connected = connected;
    client = &channel;

    if(!connected)
        client = nullptr;
}

std::vector<std::string> parse(std::string& str, std::string delimiter);
std::string sanitizeString(std::string messagePart);
void Manager::listenOnEvents(Channel& client, const Message& message) {
    auto msgStr = message.toString();
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Received: %s", sanitizeString(msgStr.data()).c_str());
    #endif

    // gamer delimiters
    currentMessage << msgStr;
    std::string currentStr = currentMessage.str();
    auto strs = parse(currentStr, "\n\n\n\n\n");
    // avoid overwriting stream without a full message present
    if(strs.size() > 1) {
        // replace current message stringstream with the latter of the message
        currentMessage.str(strs[strs.size() - 1]);
        strs.erase(strs.end() - 1);
        // process each message found
        for(auto& str : strs) {
            processMessage(str);
        }
    }
}

void Manager::sendMessage(std::string message) {
    if(!connected) return;
    
    client->queueWrite(Message(message));
    // sending a message should mean it is finished processing
    // so send the next queued one if there is
    processingMessage = false;
    if(queuedMessages.size() > 0) {
        LOG_INFO("Processing queued message");
        client->queueWrite(Message("echo" + queuedMessages[0] + "\n\n\n\n\n"));
        queuedMessages.erase(queuedMessages.begin());
    }
}

#pragma region sending
std::string sanitizeString(std::string messagePart) {
    std::stringstream ss;
    for(auto& chr : messagePart)
        if(chr == '\n')
            ss << "\\n";
        else
            ss << chr;
    std::string res = ss.str();
    if(res.length() < 1)
        return " ";
    return res;
}

std::string methodMessage(Method& method) {
    std::stringstream ss;
    // field marker
    ss << (method.field ? "1" : "0");
    ss << "\n\n";
    // return type
    if(isSimpleType(method.returnType->type))
        ss << sanitizeString(typeName(method.returnType));
    else
        ss << sanitizeString(il2cpp_functions::type_get_name(method.returnType));
    // argument types
    for(auto& paramType : method.paramTypes) {
        ss << "\n";
        if(isSimpleType(paramType->type))
            ss << sanitizeString(typeName(paramType));
        else
            ss << sanitizeString(il2cpp_functions::type_get_name(paramType));
    }
    ss << "\n\n";
    // name
    ss << sanitizeString(method.name);
    // argument names
    for(auto& paramName : method.paramNames) {
        ss << "\n";
        ss << sanitizeString(paramName);
    }
    return ss.str();
}

void Manager::sendResult(std::string value, std::string classTypeName) {
    if(!connected) return;

    std::stringstream ss;
    ss << "result\n\n\n\n";
    ss << sanitizeString(classTypeName);
    ss << "\n\n\n\n";
    ss << sanitizeString(value);
    ss << "\n\n\n\n\n";
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Sending: %s", sanitizeString(ss.str()).c_str());
    #endif
    sendMessage(ss.str());
}

void Manager::setAndSendObject(Il2CppObject* obj) {
    if(!connected) return;
    if(!obj) return;

    object = obj;
    methods.clear();

    std::stringstream ss;
    ss << "class_info\n\n\n\n";
    // convert pointer to string
    ss << sanitizeString(std::to_string(*(std::uintptr_t*)(&object)));
    auto klass = classofinst(object);
    
    do {
        ss << "\n\n\n\n";
        ss << sanitizeString(il2cpp_functions::type_get_name(il2cpp_functions::class_get_type(klass)));

        auto fieldInfos = GetFields(klass);
        // turn each field into fake get/set methods
        for(auto& fieldInfo : fieldInfos) {
            // gets should always be fine, type-wise
            auto fakeMethodGet = Method(object, fieldInfo, false);
            methods.emplace_back(fakeMethodGet);
            // doesn't need a first check, since the "first" is the class name
            ss << "\n\n\n";
            ss << methodMessage(fakeMethodGet);

            auto fakeMethodSet = Method(object, fieldInfo, true);
            if(!fakeMethodSet.hasNonSimpleParam) {
                methods.emplace_back(fakeMethodSet);
                ss << "\n\n\n";
                ss << methodMessage(fakeMethodSet);
            }
        }

        auto methodInfos = GetMethods(klass);
        // convert to our method type
        for(auto& methodInfo : methodInfos) {
            // would rather emplace the whole object, but then it might have to be removed
            auto method = Method(object, methodInfo);
            if(!method.hasNonSimpleParam) {
                methods.emplace_back(method);
                ss << "\n\n\n";
                ss << methodMessage(method);
            }
        }

        klass = GetParent(klass);
    } while(klass);
    
    ss << "\n\n\n\n\n";

    #ifdef MESSAGE_LOGGING
    LOG_INFO("Sending: %s", sanitizeString(ss.str()).c_str());
    #endif
    // send info
    sendMessage(ss.str());
    LOG_INFO("Object set");
}
#pragma endregion

#pragma region parsing
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

void Manager::processMessage(std::string message) {
    // only allow one message to be processed at a time
    if(processingMessage) {
        #ifdef MESSAGE_LOGGING
        LOG_INFO("Adding message to queue: %s", sanitizeString(message).c_str());
        #endif
        queuedMessages.emplace_back(message);
        return;
    }
    processingMessage = true;
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Processing: %s", sanitizeString(message).c_str());
    #endif
    // find command type
    auto idx = message.find("\n\n\n\n");
    if(idx != std::string::npos) {
        std::string command = message.substr(0, idx);
        if(awaitingMessage) {
            // dispatch truncated message to set callback
            nextMessageCallback(message.substr(idx + 4));
            return;
        }
        // otherwise, find based on command name
        if(command == "run") {
            processRun(message.substr(idx + 4));
            return;
        }
        if(command == "run_raw") {
            processRunRaw(message.substr(idx + 4));
            return;
        }
        if(command == "load") {
            processLoad(message.substr(idx + 4));
            return;
        }
    }
    LOG_INFO("Message does not match a command.");
}

void Manager::processRun(std::string command) {
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Run: %s", sanitizeString(command).c_str());
    #endif
    int index;
    std::vector<std::string> args;
    try {
        // separate into index, then list of args
        auto base_args = parse(command, "\n\n\n\n");
        index = std::stoi(base_args[0]);
        for(int i = 1; i < base_args.size(); i++) {
            // check if each arg wants a constructor
            auto arg = parse(base_args[i], "\n\n\n");
            if(std::stoi(arg[0]) > 0) {
                // use the presence of empty strings to determine constructor arguments
                // alright because empty strings are not allowed in messages
                args.emplace_back("");
                awaitingMessage = true;
                nextMessageCallback = [this](std::string str){ processRun(str); };
            } else
                args.emplace_back(arg[1]);
        }
    } catch(...) {
        LOG_INFO("Could not parse run command");
        // in case it was set before the exception
        // no need to reset the function since it won't be called without this set
        awaitingMessage = false;
        return;
    }
    if(!awaitingMessage)
        #ifdef MESSAGE_LOGGING
        LOG_INFO("Run: Index %i, Num args: %lu", index, args.size()); // gets logged
        #endif
        RunMethod(index, args);
    // else: handle constructor arguments
}

void Manager::processRunRaw(std::string command) {
    #ifdef MESSAGE_LOGGING
    LOG_INFO("RunRaw: %s", sanitizeString(command).c_str());
    #endif
    Il2CppObject* target;
    Il2CppClass* klass;
    std::vector<std::string> baseArgs;
    std::string methodName;
    int argNum;
    std::vector<std::string> args;
    try {
        // object pointer, method name, argument number, arguments
        baseArgs = parse(command, "\n\n\n\n");
        std::uintptr_t ptr_int = (std::uintptr_t)std::stol(baseArgs[0]);
        if(ptr_int < 10)
            throw;
        target = *(reinterpret_cast<Il2CppObject**>(&ptr_int));
        klass = classofinst(target);
    } catch(...) {
        LOG_INFO("Could not parse run_raw argument as a pointer");
        return;
    }
    try {
        methodName = baseArgs[1];
        argNum = std::stoi(baseArgs[2]);
        args.reserve(baseArgs.size() - 3);
        args.insert(args.end(), baseArgs.begin(), baseArgs.end());
    } catch(...) {
        LOG_INFO("Could not process run_raw command");
        return;
    }
    // could do type checking, but this doesn't allow user input anyway
    #ifdef MESSAGE_LOGGING
    LOG_INFO("RunRaw: Object %p, MethodName %s, ArgNum %i", target, methodName.c_str(), argNum);
    #endif
    auto info = const_cast<MethodInfo*>(il2cpp_functions::class_get_method_from_name(klass, methodName.c_str(), argNum));
    methods.emplace_back(Method(target, info));
    RunMethod(methods.size() - 1, args);
}

void Manager::processLoad(std::string command) {
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Load: %s", sanitizeString(command).c_str());
    #endif
    try {
        std::uintptr_t ptr_int = (std::uintptr_t)std::stol(command);
        if(ptr_int < 10)
            throw; // nullptr would probably cause a crash regardless of catch
        Il2CppObject* ptr = *(reinterpret_cast<Il2CppObject**>(&ptr_int));
        // this is why I don't allow user input for this
        // I don't know how to check that this pointer isn't garbage
        // maybe if I'm lucky catch(...) can prevent crashes
        #ifdef MESSAGE_LOGGING
        LOG_INFO("Load: %p", ptr);
        #endif
        SetObject(ptr);
    } catch(...) {
        LOG_INFO("Could not parse load argument as a pointer");
    }
}
#pragma endregion
