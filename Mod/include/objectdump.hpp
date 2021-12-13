#pragma once

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"

#include <fstream>

void logChildren(UnityEngine::Transform* t, std::ofstream& stream, int maxDepth, int depth=0);

void logHierarchy(std::string path);