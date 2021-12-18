#include "main.hpp"
#include "classutils.hpp"

using namespace ClassUtils;
using namespace il2cpp_utils;

// field_get_value, field_set_value
std::vector<FieldInfo*> ClassUtils::getFields(Il2CppClass* klass) {
    std::vector<FieldInfo*> ret;
    FieldInfo** iter;
    while(il2cpp_functions::class_get_fields(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

// property_get_get_method, property_get_set_method
std::vector<PropertyInfo*> ClassUtils::getProperties(Il2CppClass* klass) {
    std::vector<PropertyInfo*> ret;
    PropertyInfo** iter;
    while(il2cpp_functions::class_get_properties(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<MethodInfo*> ClassUtils::getMethods(Il2CppClass* klass) {
    std::vector<MethodInfo*> ret;
    MethodInfo** iter;
    while(il2cpp_functions::class_get_methods(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<Il2CppClass*> ClassUtils::getInterfaces(Il2CppClass* klass) {
    std::vector<Il2CppClass*> ret;
    Il2CppClass** iter;
    while(il2cpp_functions::class_get_interfaces(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}