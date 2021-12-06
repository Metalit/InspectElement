#include "main.hpp"
#include "classutils.hpp"

using namespace ClassUtils;
using namespace il2cpp_utils;
using namespace il2cpp_functions;

std::vector<MethodInfo*> getMethods(Il2CppObject* obj) {
    std::vector<MethodInfo*> ret;
    auto klass = object_get_class(obj);
    MethodInfo** iter;
    while(class_get_methods(klass, &iter)) {
        if(*iter)
            ret.push_back(*iter);
    }
    return ret;
}