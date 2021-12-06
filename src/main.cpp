#include "main.hpp"
#include "classutils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "questui/shared/QuestUI.hpp"
#include "config-utils/shared/config-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "VRUIControls/VRPointer.hpp"
#include "UnityEngine/EventSystems/PointerEventData.hpp"
#include "UnityEngine/EventSystems/RaycastResult.hpp"

#include "UnityEngine/GameObject.hpp"

using namespace GlobalNamespace;

static ModInfo modInfo;
DEFINE_CONFIG(ModConfig);

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Hooks

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().info("Installing hooks...");
    il2cpp_functions::Init();

    getModConfig().Init(modInfo);
    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);

    LoggerContextObject logger = getLogger().WithContext("load");
    // Install hooks

    getLogger().info("Installed all hooks!");
}