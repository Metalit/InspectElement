#include "main.hpp"
#include "classutils.hpp"

using namespace ClassUtils;
using namespace il2cpp_utils;

std::vector<FieldInfo*> getFields(Il2CppClass* klass) {
    std::vector<FieldInfo*> ret;
    FieldInfo** iter;
    while(il2cpp_functions::class_get_fields(klass, &iter)) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<PropertyInfo*> getProperties(Il2CppClass* klass) {
    std::vector<PropertyInfo*> ret;
    PropertyInfo** iter;
    while(il2cpp_functions::class_get_properties(klass, &iter)) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<MethodInfo*> getMethods(Il2CppClass* klass) {
    std::vector<MethodInfo*> ret;
    MethodInfo** iter;
    while(il2cpp_functions::class_get_methods(klass, &iter)) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<Il2CppClass*> getInterfaces(Il2CppClass* klass) {
    std::vector<Il2CppClass*> ret;
    Il2CppClass** iter;
    while(il2cpp_functions::class_get_interfaces(klass, &iter)) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}