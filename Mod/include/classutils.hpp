#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace ClassUtils {
    std::vector<FieldInfo*> GetFields(Il2CppClass* klass);

    std::vector<MethodInfo*> GetPropMethods(PropertyInfo* prop);
    std::vector<PropertyInfo*> GetProperties(Il2CppClass* klass);

    std::vector<MethodInfo*> GetMethods(Il2CppClass* klass);

    std::vector<Il2CppClass*> GetInterfaces(Il2CppClass* klass);

    Il2CppClass* GetParent(Il2CppClass* klass);
}

#define classofinst(instance) il2cpp_functions::object_get_class(instance)