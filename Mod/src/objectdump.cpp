#include "main.hpp"
#include "objectdump.hpp"

#include "UnityEngine/Resources.hpp"

void logIndent(std::ofstream& stream, int length) {
    for(int i = 0; i < length; i++)
        stream << "  ";
}

void logChildren(UnityEngine::Transform* t, std::ofstream& stream, int maxDepth, int depth) {
    if(depth > maxDepth) return;
    if(!t) return;
    int num = t->get_childCount();
    logIndent(stream, depth);
    stream << STR(t->get_gameObject()->get_name()) << " has " << num << " child" << (num == 1? "\n" : "ren\n");
    auto arr = t->get_gameObject()->GetComponents<UnityEngine::MonoBehaviour*>();
    for(int i = 0; i < arr->Length(); i++) {
        auto cmpnt = arr->get(i);
        const static MethodInfo* nameMethod = il2cpp_utils::FindMethodUnsafe("UnityEngine", "MonoBehaviour", "GetScriptClassName", 0);
        Il2CppString* name = unwrap_optionals(il2cpp_utils::RunMethod<Il2CppString*, false>(cmpnt, nameMethod));
        if(name) {
            logIndent(stream, depth + 1);
            stream << "Component: " << STR(name) << "\n";
        }
        // const MethodInfo* textMethod = il2cpp_utils::FindMethodUnsafe(cmpnt, "get_text", 0);
        // if(textMethod) {
        //     Il2CppString* text = unwrap_optionals(il2cpp_utils::RunMethod<Il2CppString*, false>(cmpnt, textMethod));
        //     if(text) {
        //         logIndent(stream, depth + 2);
        //         stream << "Has text: " << STR(text) << "\n";
        //     }
        // }
    }
    for(int i = 0; i < num; i++) {
        logChildren(t->GetChild(i), stream, maxDepth, depth + 1);
    }
}

void logHierarchy(std::string path) {
    std::ofstream stream(path);
    if(!stream) {
        LOG_INFO("Couldn't open path %s for writing", path.c_str());
        return;
    }
    // slooow
    auto objects = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::GameObject*>();
    for(int i = 0; i < objects->Length(); i++) {
        auto obj = objects->get(i)->get_transform();
        if(obj->get_parent()) continue;
        stream << "Root object: " << STR(objects->get(i)->get_name()) << "\n";
        logChildren(obj, stream, 8);
    }
}