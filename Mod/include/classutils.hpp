#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace ClassUtils {
    std::vector<FieldInfo*> getFields(Il2CppClass* klass);

    std::vector<MethodInfo*> getPropMethods(PropertyInfo* prop);
    std::vector<PropertyInfo*> getProperties(Il2CppClass* klass);

    std::vector<MethodInfo*> getMethods(Il2CppClass* klass);

    std::vector<Il2CppClass*> getInterfaces(Il2CppClass* klass);

    Il2CppClass* getParent(Il2CppClass* klass);
}

#define classofinst(instance) il2cpp_functions::object_get_class(instance)