#include "methods.hpp"

#include <sstream>
#include <iomanip>

// TODO: fix consts

void* toPointer(std::uintptr_t value) {
    return *((void**)(&value));
}

bool isSimpleType(Il2CppTypeEnum type) {
    return ((type >= IL2CPP_TYPE_BOOLEAN) && (type <= IL2CPP_TYPE_PTR))
    || (type == IL2CPP_TYPE_I)
    || (type == IL2CPP_TYPE_U);
}

#define CASE(il2cpptypename, ret) case IL2CPP_TYPE_##il2cpptypename: return ret;
#define BASIC_CASE(il2cpptypename) CASE(il2cpptypename, #il2cpptypename)
std::string typeName(Il2CppType* type) {
    auto typeEnum = type->type;
    switch (typeEnum) {
        BASIC_CASE(END)
        BASIC_CASE(BYREF)
        BASIC_CASE(VAR)
        BASIC_CASE(GENERICINST)
        BASIC_CASE(TYPEDBYREF)
        BASIC_CASE(FNPTR)
        BASIC_CASE(MVAR)
        BASIC_CASE(INTERNAL)
        CASE(VOID, "void")
        CASE(BOOLEAN, "bool")
        CASE(CHAR, "char")
        CASE(I1, "int8")
        CASE(U1, "uint8")
        CASE(I2, "int16")
        CASE(U2, "uint16")
        CASE(I4, "int32")
        CASE(U4, "uint32")
        CASE(I8, "int64")
        CASE(U8, "uint64")
        CASE(R4, "float")
        CASE(R8, "double")
        CASE(STRING, "string")
        CASE(PTR, "pointer")
        CASE(ARRAY, "array (unbounded)")
        CASE(SZARRAY, "array (bounded)")
        CASE(I, "\"int\" pointer")
        CASE(U, "\"uint\" pointer")
        case IL2CPP_TYPE_VALUETYPE: return il2cpp_functions::type_get_name(type);
        case IL2CPP_TYPE_OBJECT: return il2cpp_functions::type_get_name(type);
        case IL2CPP_TYPE_CLASS: return il2cpp_functions::type_get_name(type);
        default: return "Other Type " + std::to_string(typeEnum);
    }
}

#define ALLOC_RET(type) ret = malloc(sizeof(type)); *((type*)ret)
#define IF_NOT_EMPTY(type, expr) (arg != " " && arg != "") ? (expr) : (type)(0)
#define GEN_RET(type, expr) ALLOC_RET(type) = IF_NOT_EMPTY(type, expr); return ret
#define INT_RET(intType) GEN_RET(intType, (intType)std::stoi(arg))
void* toMethodArg(Il2CppTypeEnum type, std::string arg) {
    void* ret = nullptr;
    switch (type) {
        case IL2CPP_TYPE_BOOLEAN:
            std::transform(arg.begin(), arg.end(), arg.begin(), tolower);
            GEN_RET(bool, arg == "true");
        case IL2CPP_TYPE_CHAR:
            GEN_RET(char, arg.c_str()[0]);
        case IL2CPP_TYPE_I1:
            // INT_RET(int8_t);
        case IL2CPP_TYPE_U1:
            INT_RET(uint8_t);
        case IL2CPP_TYPE_I2:
            // INT_RET(int16_t);
        case IL2CPP_TYPE_U2:
            INT_RET(uint16_t);
        case IL2CPP_TYPE_I4:
            // INT_RET(int32_t);
        case IL2CPP_TYPE_U4:
            INT_RET(uint32_t);
        case IL2CPP_TYPE_I8:
            // INT_RET(int64_t);
        case IL2CPP_TYPE_U8:
            INT_RET(uint64_t);
        case IL2CPP_TYPE_R4:
            GEN_RET(float, (float)std::stod(arg));
        case IL2CPP_TYPE_R8:
            GEN_RET(double, (double)std::stod(arg));
        case IL2CPP_TYPE_STRING:
            return (void*)(il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>(arg));
        case IL2CPP_TYPE_I:
        case IL2CPP_TYPE_U:
        case IL2CPP_TYPE_PTR:
            GEN_RET(void*, toPointer(std::stol(arg)));
        default:
            return nullptr;
    }
}

#define CAST_RES(type) *((type*)res)
#define TO_SSTR(type) ss << CAST_RES(type); return ss.str()
std::string toString(Il2CppTypeEnum type, void* res) {
    if(!res) return "nullptr";
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
            ss << std::fixed << std::setprecision(10) << CAST_RES(float);
            return ss.str();
        case IL2CPP_TYPE_R8:
            ss << std::fixed << std::setprecision(10) << CAST_RES(double);
            return ss.str();
        case IL2CPP_TYPE_STRING:
            return STR((Il2CppString*)res);
        case IL2CPP_TYPE_I:
        case IL2CPP_TYPE_U:
        case IL2CPP_TYPE_PTR:
            TO_SSTR(std::uintptr_t);
        default:
            return "invalid type";
    }
}

Method::Method(Il2CppObject* obj, MethodInfo* m) {
    object = obj;
    method = m;
    for(decltype(method->parameters_count) i = 0; i < method->parameters_count; i++) {
        auto param = method->parameters[i];
        paramTypes.emplace_back(const_cast<Il2CppType*>(param.parameter_type));
        paramNames.emplace_back(param.name);
        hasNonSimpleParam = hasNonSimpleParam || !isSimpleType(param.parameter_type->type);
    }
    // not doing generic methods yet
    hasNonSimpleParam = hasNonSimpleParam || method->is_generic;
    returnType = const_cast<Il2CppType*>(method->return_type);
    retNonSimple = !isSimpleType(returnType->type);
    name = method->name;
}

// treat fields like properties w/ get/set methods
Method::Method(Il2CppObject* obj, FieldInfo* f, bool s) {
    object = obj;
    field = f;
    set = s;
    auto type = field->type;
    paramTypes.emplace_back(const_cast<Il2CppType*>(type));
    paramNames.emplace_back(field->name);
    hasNonSimpleParam = !isSimpleType(type->type) && set;
    returnType = const_cast<Il2CppType*>(type);//set ? IL2CPP_TYPE_VOID : type;
    retNonSimple = !isSimpleType(returnType->type);
    name = field->name;
}

Il2CppObject* Method::run(std::vector<std::string> args, std::string* valueRes) {
    // regular method, uses runtime_invoke
    if(method) {
        LOG_INFO("Running method %s", name.c_str());
        // only works for simple methods
        if(hasNonSimpleParam) return nullptr;
        // turn each string into a parameter
        LOG_INFO("Getting parameters");
        if(args.size() != paramTypes.size()) {
            LOG_INFO("Method and parameter mismatch");
            return nullptr;
        }

        void* params[paramTypes.size()];
        for(int i = 0; i < paramTypes.size(); i++) {
            LOG_INFO("Processing parameter of type: %i = %s", paramTypes[i]->type, typeName(paramTypes[i]).c_str());
            // mallocs all params
            try {
                params[i] = toMethodArg(paramTypes[i]->type, args[i]);
            } catch(...) {
                // free all set params, including the current one
                // since toMethodArg mallocs first, then tries to add the data
                for(int j = 0; j <= i; j++) {
                    free(params[j]);
                }
                LOG_INFO("Could not process arguments");
                return nullptr;
            }
        }
        // run the method
        Il2CppException* ex = nullptr;
        auto ret = il2cpp_functions::runtime_invoke(method, object, params, &ex);
        // free params
        for(auto& param : params) {
            free(param);
        }
        // catch exceptions
        if(ex) {
            LOG_INFO("%s: Failed with exception: %s", name.c_str(), il2cpp_utils::ExceptionToString(ex).c_str());
            *valueRes = "Error: " + il2cpp_utils::ExceptionToString(ex); // Error: Failed with exception: lol
            return nullptr;
        }
        LOG_INFO("Returning");
        LOG_INFO("Return type: %i = %s", returnType->type, typeName(returnType).c_str());
        // check if ret is a value type (mostly copied from bs-hook)
        // il2cpp will return boxed values from methods, but not store them in fields
        if(!retNonSimple && il2cpp_functions::class_is_valuetype(il2cpp_functions::object_get_class(ret))) {
            *valueRes = toString(returnType->type, il2cpp_functions::object_unbox(ret));
            il2cpp_functions::GC_free(ret);
            return nullptr;
        } else if(returnType->type == IL2CPP_TYPE_STRING) {
            *valueRes = toString(returnType->type, ret);
            il2cpp_functions::GC_free(ret);
            return nullptr;
        }
        return ret;
    } else if(field) { // fields :barf:
        if(set) {
            LOG_INFO("Setting field %s", name.c_str());
            LOG_INFO("Field type: %i = %s", returnType->type, typeName(returnType).c_str());
            void* param = toMethodArg(paramTypes[0]->type, args[0]);
            il2cpp_functions::field_set_value(object, field, param);
            *valueRes = args[0];
            free(param);
            return nullptr;
        } else {
            LOG_INFO("Getting field %s", name.c_str());
            LOG_INFO("Field type: %i = %s", returnType->type, typeName(returnType).c_str());
            // might want to consider more possible field types, but this works for now
            if(retNonSimple) {
                void* result = nullptr;
                il2cpp_functions::field_get_value(object, field, &result);
                // temporary solution
                if(returnType->type == IL2CPP_TYPE_VALUETYPE) {
                    LOG_INFO("Value type return");
                    return nullptr;
                }
                return (Il2CppObject*)result;
            } else {
                void* result = toMethodArg(returnType->type, "");
                il2cpp_functions::field_get_value(object, field, result);
                *valueRes = toString(returnType->type, result);
                free(result);
                return nullptr;
            }
        }
    }
    return nullptr;
}

std::string Method::infoString() {
    CRASH_UNLESS(paramNames.size() == paramTypes.size());
    std::stringstream ss;
    if(method) {
        ss << "M: " << typeName(returnType) << " " << name << "(";
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
        ss << "F: " << typeName(returnType) << " " << name;
        if(set)
            ss << " (set)\n";
        else
            ss << " (get)\n";
        return ss.str();
    } else
        return "";
}