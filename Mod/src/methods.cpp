#include "methods.hpp"

#include <sstream>
#include <iomanip>

bool isSimpleType(Il2CppTypeEnum type) {
    return (type >= IL2CPP_TYPE_BOOLEAN) && (type <= IL2CPP_TYPE_STRING);
}

std::string typeName(Il2CppTypeEnum type) {
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN: return "bool";
        case IL2CPP_TYPE_CHAR: return "char";
        case IL2CPP_TYPE_I1: return "int8";
        case IL2CPP_TYPE_U1: return "uint8";
        case IL2CPP_TYPE_I2: return "int16";
        case IL2CPP_TYPE_U2: return "uint16";
        case IL2CPP_TYPE_I4: return "int32";
        case IL2CPP_TYPE_U4: return "uint32";
        case IL2CPP_TYPE_I8: return "int64";
        case IL2CPP_TYPE_U8: return "uint64";
        case IL2CPP_TYPE_R4: return "float";
        case IL2CPP_TYPE_R8: return "double";
        case IL2CPP_TYPE_STRING: return "string";
        default: return "Il2CppObject";
    }
}

#define ALLOC_RET(type) ret = alloca(sizeof(type)); *((type*)ret)
#define IF_NOT_EMPTY(type, expr) arg.length() > 0 ? expr : (type)(0);
#define INT_RET(intType) ALLOC_RET(intType) = IF_NOT_EMPTY(intType, (intType)(std::stoi(arg))); return ret
inline void* toMethodArg(Il2CppTypeEnum type, std::string arg) {
    void* ret = nullptr;
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN:
            std::transform(arg.begin(), arg.end(), arg.begin(), tolower);
            ALLOC_RET(bool) = IF_NOT_EMPTY(bool, arg == "true");
            return ret;
        case IL2CPP_TYPE_CHAR:
            ALLOC_RET(char) = IF_NOT_EMPTY(char, arg.c_str()[0]);
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
            ALLOC_RET(float) = IF_NOT_EMPTY(float, (float)(std::stod(arg)));
            return ret;
        case IL2CPP_TYPE_R8:
            ALLOC_RET(double) = IF_NOT_EMPTY(double, (double)(std::stod(arg)));
            return ret;
        case IL2CPP_TYPE_STRING:
            return (void*)(CSTR(arg));
        default:
            return nullptr;
    }
}

#define CAST_RES(type) *((type*)res)
#define TO_SSTR(type) ss << CAST_RES(type); return ss.str()
std::string toString(Il2CppTypeEnum type, void* res) {
    std::stringstream ss;
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN:
            return CAST_RES(bool) ? "true" : "false";
        case IL2CPP_TYPE_CHAR:
            TO_SSTR(char);
        case IL2CPP_TYPE_I1:
            TO_SSTR(int8_t);
        case IL2CPP_TYPE_U1:
            TO_SSTR(uint8_t);
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

Method::Method(Il2CppObject* obj, MethodInfo* m) {
    object = obj;
    method = m;
    // LOG_INFO("processing method %s", method->name);
    for(decltype(method->parameters_count) i = 0; i < method->parameters_count; i++) {
        auto param = method->parameters[i];
        // LOG_INFO("name: %s, type: %i", param.name, param.parameter_type->type);
        paramTypes.emplace_back(param.parameter_type->type);
        paramNames.emplace_back(param.name);
        hasNonSimpleParam = hasNonSimpleParam || !isSimpleType(param.parameter_type->type);
    }
    returnType = method->return_type->type;
    name = method->name;
}

// treat fields like properties w/ get/set methods
Method::Method(Il2CppObject* obj, FieldInfo* f, bool s) {
    object = obj;
    field = f;
    set = s;
    auto type = field->type->type;
    paramTypes.emplace_back(type);
    paramNames.emplace_back(field->name);
    hasNonSimpleParam = !isSimpleType(type);
    returnType = set ? IL2CPP_TYPE_VOID : type;
    name = field->name;
}

Il2CppObject* Method::run(std::vector<std::string> args, std::string* valueRes) {
    // only works for simple methods
    if(hasNonSimpleParam) return nullptr;
    // regular method, uses runtime_invoke
    if(method) {
        // turn each string into a parameter
        void* params[paramTypes.size()];
        for(int i = 0; i < paramTypes.size(); i++) {
            // since toMethodArg is inline and uses alloca, all args should be freed on return
            params[i] = toMethodArg(paramTypes[i], args[i]);
        }
        // run the method
        Il2CppException* ex = nullptr;
        auto ret = il2cpp_functions::runtime_invoke(method, object, params, &ex);
        // catch exceptions
        if(ex) {
            LOG_INFO("%s: Failed with exception: %s", il2cpp_functions::method_get_name(method),
                il2cpp_utils::ExceptionToString(ex).c_str());
            return nullptr;
        }
        // check if ret is a value type (mostly copied from bs-hook)
        if(il2cpp_functions::class_is_valuetype(il2cpp_functions::object_get_class(ret))) {
            *valueRes = toString(returnType, ret);
            il2cpp_functions::GC_free(ret);
            return nullptr;
        }
        return ret;
    } else if(field) { // fields :barf:
        if(set) {
            void* param = toMethodArg(paramTypes[0], args[0]);
            il2cpp_functions::field_set_value(object, field, param);
            return nullptr;
        } else {
            // assumes simple field type
            if(!isSimpleType(returnType)) return nullptr;
            void* result = toMethodArg(returnType, "");
            il2cpp_functions::field_get_value(object, field, result);
            *valueRes = toString(returnType, result);
            return nullptr;
        }
    }
    return nullptr;
}

std::string Method::infoString() {
    CRASH_UNLESS(paramNames.size() == paramTypes.size());
    std::stringstream ss;
    if(method) {
        ss << "Method: " << typeName(returnType) << " " << name << "(";
        bool first = true;
        for(int i = 0; i < paramNames.size(); i++) {
            if(!first)
                ss << ", ";
            else
                first = false;
            ss << typeName(paramTypes[i]) << " " << paramNames[i];
        }
        ss << ")\n";
        return ss.str();
    } else if(field) {
        ss << "Field: " << typeName(returnType) << " " << name;
        if(set)
            ss << " (set)\n";
        else
            ss << " (get)\n";
        return ss.str();
    } else
        return "";
}