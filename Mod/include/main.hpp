#pragma once

#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

#include "modconfig.hpp"

Logger& getLogger();

#define LOG_INFO(...) getLogger().info(__VA_ARGS__)
#define STR(string) to_utf8(csstrtostr(string))
#define CSTR(string) il2cpp_utils::newcsstr(string)

#include "HMUI/ViewController.hpp"

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);