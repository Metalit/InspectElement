#pragma once

#include "main.hpp"

class Method {
    public:
    Il2CppTypeEnum returnType;
    std::vector<Il2CppTypeEnum> paramTypes;
    bool retNonSimple = false;
    bool hasNonSimpleParam = false;

    std::string name;
    std::vector<std::string> paramNames;
    
    Il2CppObject* object;
    MethodInfo* method;
    FieldInfo* field;
    bool set; // set or get for field

    Method(Il2CppObject* obj, MethodInfo* method);
    Method(Il2CppObject* obj, FieldInfo* field, bool set);
    Il2CppObject* run(std::vector<std::string> args, std::string* valueRes = nullptr);

    std::string infoString();
};