#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace ClassUtils {
    std::vector<const FieldInfo*> GetFields(const Il2CppClass* klass);

    std::vector<const MethodInfo*> GetPropMethods(const PropertyInfo* prop);
    std::vector<const PropertyInfo*> GetProperties(const Il2CppClass* klass);

    std::vector<const MethodInfo*> GetMethods(const Il2CppClass* klass);

    std::vector<const Il2CppClass*> GetInterfaces(const Il2CppClass* klass);

    const Il2CppClass* GetParent(const Il2CppClass* klass);
}

#define classofinst(instance) il2cpp_functions::object_get_class(instance)