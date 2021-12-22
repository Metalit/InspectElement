#pragma once

#include "main.hpp"

class Method {
    public:
    Il2CppType* returnType;
    std::vector<Il2CppType*> paramTypes;
    bool retNonSimple = false;
    bool hasNonSimpleParam = false;

    std::string name;
    std::vector<std::string> paramNames;
    
    Il2CppObject* object = nullptr;
    MethodInfo* method = nullptr;
    FieldInfo* field = nullptr;
    bool set; // set or get for field

    Method(Il2CppObject* obj, MethodInfo* method);
    Method(Il2CppObject* obj, FieldInfo* field, bool set);
    Il2CppObject* run(std::vector<std::string> args, std::string* valueRes = nullptr);

    std::string infoString();
};

std::string typeName(Il2CppType* type);
bool isSimpleType(Il2CppTypeEnum type);
