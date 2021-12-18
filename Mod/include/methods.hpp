#pragma once

#include "main.hpp"

class Method {
    Il2CppTypeEnum returnType;
    std::vector<Il2CppTypeEnum> paramTypes;
    bool hasNonSimpleParam;

    std::string name;
    std::vector<std::string> paramNames;
    
    void* object;
    MethodInfo* method;

    Method(void* obj, MethodInfo* method);
    Il2CppObject* run(std::vector<std::string> args);
};