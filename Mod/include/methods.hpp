#pragma once

#include "main.hpp"

#include "protobuf/qrue.pb.h"

struct RetWrapper {
    private:
    void* val = nullptr;
    size_t valSize = sizeof(void*);

    public:
    RetWrapper(void* value, size_t size = sizeof(void*))
        : val(value), valSize(size) {}
    ~RetWrapper() { if(val) free(val); }

    RetWrapper(const BoxWrapper&) = delete;
    RetWrapper& operator=(const BoxWrapper&) = delete;

    bool HasValue() const { return val != nullptr; }
    std::span<byte> GetAsBytes() const { return {(byte*) val, valSize}; }
    std::string GetAsString() const { return {(char*) val, valSize}; }
};

class Method {
    private:
    const Il2CppType* returnType;
    std::vector<const Il2CppType*> paramTypes;

    std::string name;
    std::vector<std::string> paramNames;
    
    Il2CppObject* object = nullptr;
    MethodInfo* method = nullptr;
    FieldInfo* field = nullptr;
    bool set; // set or get for field

    public:
    Method(Il2CppObject* obj, MethodInfo* method);
    Method(Il2CppObject* obj, FieldInfo* field, bool set);
    RetWrapper Run(void** args, std::string& error, bool derefReferences = true);

    Il2CppClassInfo ReturnClassInfo();
    Il2Cpp
};
