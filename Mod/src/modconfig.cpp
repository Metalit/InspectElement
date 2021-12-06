#include "main.hpp"

#include "HMUI/Touchable.hpp"
#include "questui/shared/BeatSaberUI.hpp"

using namespace QuestUI;

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        
        AddConfigValueToggle(container->get_transform(), getModConfig().Active);
    }
}