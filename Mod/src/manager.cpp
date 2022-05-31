#include "manager.hpp"
#include "classutils.hpp"
#include "main.hpp"

#define MESSAGE_LOGGING

using namespace SocketLib;
using namespace ClassUtils;

template<class T>
inline T& ReinterpretBytes(const std::string& bytes) {
    return *(T*) bytes.c_str();
}

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
        scheduleFunction([this, args, methodIdx]() {
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

void Manager::listenOnEvents(Channel& client, const Message& message) {
    processBytes(message.toSpan());
}

void Manager::sendPacket(const PacketWrapper& packet) {
    if(!connected) return;
    
    // send size header
    size_t size = packet.ByteSizeLong();
    client->queueWrite(Message((byte*) &size, sizeof(size_t)));
    // send message with that size
    byte bytes[size];
    packet.SerializeToArray(bytes, size);
    client->queueWrite(Message(bytes));
    // sending a message should mean it is finished processing
    // so send the next queued one if there is
    // processingMessage = false;
    // if(queuedMessages.size() > 0) {
    //     LOG_INFO("Processing queued message");
    //     client->queueWrite(Message("echo" + queuedMessages[0] + "\n\n\n\n\n"));
    //     queuedMessages.erase(queuedMessages.begin());
    // }
}

#pragma region sending
// std::string sanitizeString(std::string messagePart) {
//     std::stringstream ss;
//     for(auto& chr : messagePart)
//         if(chr == '\n')
//             ss << "\\n";
//         else
//             ss << chr;
//     std::string res = ss.str();
//     if(res.length() < 1)
//         return " ";
//     return res;
// }

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

void Manager::setAndSendObject(Il2CppObject* obj, uint64_t id) {
    if(!connected) return;
    if(!obj) return;

    object = obj;
    methods.clear();

    PacketWrapper packet;
    LoadObjectResult& result = *packet.mutable_loadobjectresult();
    result.set_loadid(id);
    Il2CppTypeDetails* packetObject = result.mutable_object();

    // get class info
    // set class info
    // set fields
    // set properties
    // set methods
    // set interfaces
    // change to parent
    // repeat while parent
    
    auto klass = classofinst(object);

    do {
        std::string className(il2cpp_functions::class_get_name(klass));
        packetObject->clazz()->set_clazz(className);
        std::string classNamespace(il2cpp_functions::class_get_namespace(klass));
        packetObject->clazz()->set_namespaze(classNamespace);

        for(auto& iKlass : ClassUtils::GetInterfaces(klass)) {
            auto& interfaceClassPacket = *packetObject->add_interfaces();

            std::string className(il2cpp_functions::class_get_name(iKlass));
            interfaceClassPacket.set_clazz(className);
            std::string classNamespace(il2cpp_functions::class_get_namespace(iKlass));
            interfaceClassPacket.set_namespaze(classNamespace);
        }

        for(auto& field : ClassUtils::GetFields(klass)) {
            methods.emplace_back(Method(object, fieldInfo, false));
            auto& fakeMethodGet = methods.back();
            
            auto& fieldPacket = *packetObject->add_fields();
            fieldPacket.set_name(fakeMethodGet.name);
            fieldPacket.set_id(methods.size() - 1);
            auto& fieldTypePacket = *fieldPacket.mutable_type();
            
        }
    }
    
    // do {
    //     ss << "\n\n\n\n";
    //     ss << sanitizeString(il2cpp_functions::type_get_name(il2cpp_functions::class_get_type(klass)));

    //     auto fieldInfos = GetFields(klass);
    //     // turn each field into fake get/set methods
    //     for(auto& fieldInfo : fieldInfos) {
    //         // gets should always be fine, type-wise
    //         auto fakeMethodGet = Method(object, fieldInfo, false);
    //         methods.emplace_back(fakeMethodGet);
    //         // doesn't need a first check, since the "first" is the class name
    //         ss << "\n\n\n";
    //         ss << methodMessage(fakeMethodGet);

    //         auto fakeMethodSet = Method(object, fieldInfo, true);
    //         if(!fakeMethodSet.hasNonSimpleParam) {
    //             methods.emplace_back(fakeMethodSet);
    //             ss << "\n\n\n";
    //             ss << methodMessage(fakeMethodSet);
    //         }
    //     }

    //     auto methodInfos = GetMethods(klass);
    //     // convert to our method type
    //     for(auto& methodInfo : methodInfos) {
    //         // would rather emplace the whole object, but then it might have to be removed
    //         auto method = Method(object, methodInfo);
    //         if(!method.hasNonSimpleParam) {
    //             methods.emplace_back(method);
    //             ss << "\n\n\n";
    //             ss << methodMessage(method);
    //         }
    //     }

    //     klass = GetParent(klass);
    // } while(klass);
    
    // ss << "\n\n\n\n\n";

    // #ifdef MESSAGE_LOGGING
    // LOG_INFO("Sending: %s", sanitizeString(ss.str()).c_str());
    // #endif
    // // send info
    // sendMessage(ss.str());
    // LOG_INFO("Object set");
}
#pragma endregion

#pragma region parsing
void Manager::processBytes(std::span<byte> bytes) {

    // if there is header length left:
    //   if the packet is longer:
    //     fill the rest of the header
    //     remove removed bytes from the packet
    //     set message length left to the header
    //     set header length left to 0
    //   else:
    //     add whole packet to header
    //     update header length left
    //     return
    // if there is message length left:
    //   if the packet is longer:
    //     fill the rest of the message
    //     remove removed bytes from the packet
    //     set header length left to 4
    //     process message
    //     call again with remaining packet
    //   else:
    //     add whole packet to message
    //     update message length left
    //     return

    auto headerRemaining = header.GetRemaining();
    if(headerRemaining > 0) {
        header.AddBytes(bytes.data());
        if(int* len = header.Resolve<int>()) {
            bytes = bytes.subspan(headerRemaining);
            packetBytes.Init(*len);
        } else
            return;
    }
    auto packetRemaining = packetBytes.GetRemaining();
    if(packetRemaining > 0) {
        packetBytes.AddBytes(bytes.data());
        if(byte** pkt = packetBytes.Resolve<byte*>()) {
            bytes = bytes.subspan(packetRemaining);
            processMessage(PacketWrapper::ParseFromArray(*pkt, packetBytes.size()));
            header.Clear();
            if(!bytes.empty())
                listenOnEvents(bytes);
        }
    }
}

void Manager::processMessage(const PacketWrapper& packet) {
    switch(packet.Packet_case()) {
    case PacketWrapper::kInvokeMethod:
        invokeMethod(packet.invokemethod());
    case PacketWrapper::kLoadObject:
        loadObject(packet.loadobject());
    case PacketWrapper::kSearchObjects:
        searchObjects(packet.searchobjects());
    default:
        LOG_INFO("Invalid packet type! %i", packet.Packet_case());
    }
}

void invokeMethod(const InvokeMethod& packet) {
    // TODO: type checking?
    PacketWrapper wrapper;
    InvokeMethodResult& result = *wrapper.mutable_invokemethodresult();
    result.set_invokeuuid(packet.invokeuuid());
    
    int methodIdx = packet.methodid();
    result.set_methodid(packet.invokeuuid());
    if(methodIdx >= methods.size() || methodIdx < 0) {
        result.set_status(InvokeMethodResult::NOT_FOUND);
        sendPacket(packet);
        return;
    }
    
    int argNum = packet.args_size();
    void* args[argNum];
    for(int i = 0; i < argNum; i++) {
        args[i] = packet.mutable_args(i);
    }
    
    scheduleFunction([this, args, methodIdx, wrapper, &result] {
        std::string err = "";
        auto res = methods[methodIdx].run(args, err);

        if(!err.empty()) {
            result.set_status(InvokeMethodResult::ERR);
            result.set_error(err);
            sendPacket(packet);
            return;
        }
        result.set_status(InvokeMethodResult::OK);
        Il2CppData& data = result.result();

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
}

void loadObject(const LoadObject& packet) {
    auto ptr = ReinterpretBytes<Il2CppObject*>(packet.pointer());
    setAndSendObject(ptr, packet.loadid());
}

void searchObjects(const SearchObjects& packet) {

}

void Manager::processMessage(std::string message) {
    // only allow one message to be processed at a time
    // if(processingMessage) {
    //     #ifdef MESSAGE_LOGGING
    //     LOG_INFO("Adding message to queue: %s", sanitizeString(message).c_str());
    //     #endif
    //     queuedMessages.emplace_back(message);
    //     return;
    // }
    // processingMessage = true;
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
        if(command == "find") {
            processFind(message.substr(idx + 4));
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
    if(!awaitingMessage) {
        #ifdef MESSAGE_LOGGING
        LOG_INFO("Run: Index %i, Num args: %lu", index, args.size()); // gets logged
        #endif
        RunMethod(index, args);
    }
    // else: handle constructor arguments
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

void Manager::processFind(std::string command) {
    #ifdef MESSAGE_LOGGING
    LOG_INFO("Find: %s", sanitizeString(command).c_str());
    #endif
    try {
        // get args
        auto args = parse(command, "\n\n\n\n");
        bool nameSearch = std::stoi(args[0]);
        std::string name = args[1];
        std::string namespaceName = args[2];
        std::string className = args[3];
        // find class if provided
        Il2CppClass* klass = nullptr;
        static auto objClass = il2cpp_utils::GetClassFromName("UnityEngine", "Object");
        // account for global/unnamed namespace
        if(namespaceName == " " || namespaceName == "Global" || namespaceName == "GlobalNamespace")
            namespaceName = "";
        if(className != " ") {
            klass = il2cpp_utils::GetClassFromName(namespaceName, className);
            if(!klass) {
                LOG_INFO("Could not find class %s.%s", namespaceName.c_str(), className.c_str());
            }
            // ensure class is a subclass of UnityEngine.Object
            if(klass && !il2cpp_functions::class_is_subclass_of(klass, objClass, false)) {
                LOG_INFO("Class must be a subclass of Object to search");
                return;
            }
        }
        if(!klass)
            klass = objClass;
        // get all objects of its class
        static auto findAllMethod = il2cpp_utils::FindMethodUnsafe("UnityEngine", "Resources", "FindObjectsOfTypeAll", 1);
        auto objects = unwrap_optionals(il2cpp_utils::RunMethod<ArrayW<Il2CppObject*>, false>(nullptr, findAllMethod, il2cpp_utils::GetSystemType(klass)));
        // do search or return first object
        if(nameSearch && name != " ") {
            LOG_INFO("name");
            auto nameMethod = il2cpp_functions::class_get_method_from_name(klass, "get_name", 0);
            if(!nameMethod) {
                LOG_INFO("Class must have a get_name() method to search by name");
                return;
            }
            auto obj = objects.FirstOrDefault([&name, &nameMethod](auto x) {
                return unwrap_optionals(il2cpp_utils::RunMethod<StringW, false>(x, nameMethod)) == name;
            });
            if(!obj) {
                LOG_INFO("Could not find object with name '%s'", name.c_str());
                return;
            }
            SetObject(obj);
            return;
        } else {
            if(objects.Length() > 0)
                SetObject(objects[0]);
            else
                LOG_INFO("No objects found");
            return;
        }
    } catch(...) {
        LOG_INFO("Could not parse find command");
    }
}
#pragma endregion
