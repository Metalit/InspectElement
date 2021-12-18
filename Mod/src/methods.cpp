#include "methods.hpp"
// #include "classutils.hpp"

bool isSimpleType(Il2CppTypeEnum type) {
    return (type >= IL2CPP_TYPE_BOOLEAN) && (type <= IL2CPP_TYPE_STRING);
}

#define ALLOC_RET(type) ret = alloca(sizeof(type)); *((type*)ret)
#define INT_RET(intType) ALLOC_RET(intType) = (intType)(std::stoi(arg)); return ret
inline void* toMethodArg(Il2CppTypeEnum type, std::string arg) {
    void* ret = nullptr;
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN:
            std::transform(arg.begin(), arg.end(), arg.begin(), tolower);
            ALLOC_RET(bool) = arg == "true";
            return ret;
        case IL2CPP_TYPE_CHAR:
            ALLOC_RET(char) = arg.c_str()[0];
            return ret;
        case IL2CPP_TYPE_I1:
            INT_RET(int8_t);
        case IL2CPP_TYPE_U1:
            INT_RET(uint8_t);
        case IL2CPP_TYPE_I2:
            INT_RET(int16_t);
        case IL2CPP_TYPE_U2:
            INT_RET(uint16_t);
        case IL2CPP_TYPE_I4:
            INT_RET(int32_t);
        case IL2CPP_TYPE_U4:
            INT_RET(uint32_t);
        case IL2CPP_TYPE_I8:
            INT_RET(int64_t);
        case IL2CPP_TYPE_U8:
            INT_RET(uint64_t);
        case IL2CPP_TYPE_R4:
            ALLOC_RET(float) = (float)(std::stod(arg));
            return ret;
        case IL2CPP_TYPE_R8:
            ALLOC_RET(double) = (double)(std::stod(arg));
            return ret;
        case IL2CPP_TYPE_STRING:
            return (void*)(CSTR(arg));
        default:
            return nullptr;
    }
}

#define CAST_RES(type) *((type*)res)
#define TO_SSTR(type) ss << CAST_RES(type); return ss.str()
std::string toString(Il2CppType type, void* res) {
    std::stringstream ss;
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN:
            return CAST_RES(bool) ? "true" : "false";
        case IL2CPP_TYPE_CHAR:
            TO_SSTR(char);
        case IL2CPP_TYPE_I1:
            TO_SSTR(int8_t);
        case IL2CPP_TYPE_U1:
            INT_RET(uint8_t);
        case IL2CPP_TYPE_I2:
            TO_SSTR(int16_t);
        case IL2CPP_TYPE_U2:
            TO_SSTR(uint16_t);
        case IL2CPP_TYPE_I4:
            TO_SSTR(int32_t);
        case IL2CPP_TYPE_U4:
            TO_SSTR(uint32_t);
        case IL2CPP_TYPE_I8:
            TO_SSTR(int64_t);
        case IL2CPP_TYPE_U8:
            TO_SSTR(uint64_t);
        case IL2CPP_TYPE_R4:
            // overdoing the precision should just print out all the precision available
            ss << std::setprecision(1<<8) << CAST_RES(float);
            return ss.str();
        case IL2CPP_TYPE_R8:
            ss << std::setprecision(1<<8) << CAST_RES(double);
            return ss.str();
        case IL2CPP_TYPE_STRING:
            return STR((Il2CppString*)res);
        default:
            return "invalid";
    }
}

Method::Method(void* obj, MethodInfo* m) {
    object = obj;
    method = m;
    for(decltype(method->parameters_count) i = 0; i < method->parameters_count; i++) {
        auto param = method->parameters[i];
        paramTypes.emplace_back(param.parameter_type->type);
        paramNames.emplace_back(param.name);
        hasNonSimpleParam = hasNonSimpleParam || !isSimpleType(param.parameter_type->type);
    }
    returnType = method->return_type->type;
    name = method->name;
}

Il2CppObject* Method::run(std::vector<std::string> args) {
    void* params[paramTypes.size()];
    for(int i = 0; i < paramTypes.size(); i++) {
        params[i] = toMethodArg(paramTypes[i], args[i]);
    }
    Il2CppException* ex = nullptr;
    auto ret = il2cpp_functions::runtime_invoke(method, object, params, &ex);
    if(ex) {
        LOG_INFO("%s: Failed with exception: %s", il2cpp_functions::method_get_name(method),
            il2cpp_utils::ExceptionToString(ex).c_str());
        return nullptr;
    }
    return ret;
}