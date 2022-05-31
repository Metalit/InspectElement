#include "main.hpp"
#include "classutils.hpp"

using namespace ClassUtils;
using namespace il2cpp_utils;

// field_get_value, field_set_value
std::vector<const FieldInfo*> ClassUtils::GetFields(const Il2CppClass* klass) {
    std::vector<const FieldInfo*> ret;
    // only a single pointer since fields are stored as values
    FieldInfo* iter = nullptr; // needs to be explicitly set to nullptr
    while(il2cpp_functions::class_get_fields(klass, (void**)(&iter))) {
        if(iter)
            ret.push_back(iter);
    }
    return ret;
}

// I think all the property methods are gotten by getMethods
std::vector<const MethodInfo*> ClassUtils::GetPropMethods(const PropertyInfo* prop) {
    std::vector<const MethodInfo*> ret{};
    // should actually make everything else const, but I'm lazy
    if(auto m = il2cpp_functions::property_get_get_method(prop))
        ret.push_back(m);
    if(auto m = il2cpp_functions::property_get_set_method(prop))
        ret.push_back(m);
    return ret;
}

std::vector<const PropertyInfo*> ClassUtils::GetProperties(const Il2CppClass* klass) {
    std::vector<const PropertyInfo*> ret;
    // only a single pointer since properties are stored as values
    PropertyInfo* iter = nullptr;
    while(il2cpp_functions::class_get_properties(klass, (void**)(&iter))) {
        if(iter)
            ret.push_back(iter);
    }
    return ret;
}

std::vector<const MethodInfo*> ClassUtils::GetMethods(const Il2CppClass* klass) {
    std::vector<const MethodInfo*> ret;
    // double pointer because methods are stored as pointers
    MethodInfo** iter = nullptr;
    while(il2cpp_functions::class_get_methods(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

std::vector<const Il2CppClass*> ClassUtils::GetInterfaces(const Il2CppClass* klass) {
    std::vector<const Il2CppClass*> ret;
    // double pointer because classes are stored as pointers
    Il2CppClass** iter = nullptr;
    while(il2cpp_functions::class_get_interfaces(klass, (void**)(&iter))) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}

// TODO: genericcontext->genericinst->argv = generic type array (argc count)
// requires generally switching to type instead of class, which should be done anyway

const Il2CppClass* ClassUtils::GetParent(const Il2CppClass* klass) {
    return il2cpp_functions::class_get_parent(klass);
}
