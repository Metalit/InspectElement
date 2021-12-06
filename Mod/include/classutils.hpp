#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace ClassUtils {
    std::vector<il2cpp_utils::MethodInfo*> getMethods(il2cpp_utils::Il2CppObject* obj);
}

#define GetMethods(obj) ClassUtils::getMethods(reinterpret_cast<il2cpp_utils::Il2CppObject*>(obj))