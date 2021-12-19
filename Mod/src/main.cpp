#include "main.hpp"
#include "objectdump.hpp"
#include "classutils.hpp"
#include "manager.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "questui/shared/QuestUI.hpp"
#include "config-utils/shared/config-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "VRUIControls/VRPointer.hpp"
#include "GlobalNamespace/OVRInput_Button.hpp"

#include "UnityEngine/EventSystems/PointerEventData.hpp"
#include "UnityEngine/EventSystems/RaycastResult.hpp"

#include "UnityEngine/GameObject.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"

#include <filesystem>

using namespace GlobalNamespace;

static ModInfo modInfo;
DEFINE_CONFIG(ModConfig);

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

std::string GetDataPath() {
    static std::string s(getDataDir(modInfo));
    return s;
}

// Hooks
MAKE_HOOK_MATCH(ControllerLateUpdate, &VRUIControls::VRPointer::LateUpdate, void, VRUIControls::VRPointer* self) {
    ControllerLateUpdate(self);
    // ensure manager exists
    if(!Manager::Instance) return;
    // check for button
    bool lbut = OVRInput::GetDown(getModConfig().Button.GetValue(), OVRInput::Controller::LTouch);
    bool rbut = OVRInput::GetDown(getModConfig().Button.GetValue(), OVRInput::Controller::RTouch);
    bool isRight = self->_get__lastControllerUsedWasRight();
    if((lbut && !isRight) || (rbut && isRight)) {
        LOG_INFO("Setting object");
        // should include ui
        auto hoveredObject = self->pointerData->pointerCurrentRaycast.get_gameObject();
        Manager::Instance->SetObject(hoveredObject);
    }
}

MAKE_HOOK_MATCH(MenuActivate, &MainMenuViewController::DidActivate,
        void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MenuActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    Manager::Instance = new Manager();
    Manager::Instance->Init();
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
    
    auto dataPath = GetDataPath();
    if(!direxists(dataPath))
        mkpath(dataPath);
	
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
    INSTALL_HOOK(logger, ControllerLateUpdate);
    INSTALL_HOOK(logger, MenuActivate);
    getLogger().info("Installed all hooks!");
}