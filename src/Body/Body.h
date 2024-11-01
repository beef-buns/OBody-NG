#pragma once

#include "PresetManager/PresetManager.h"
#include "SKEE.h"

namespace Body {
    inline SKSE::RegistrationSet<RE::Actor*, std::string> OnActorGenerated("OnActorGenerated"sv);
    inline SKSE::RegistrationSet<RE::Actor*> OnActorNaked("OnActorNaked"sv);
    inline SKSE::RegistrationSet<RE::Actor*> OnActorRemovingClothes("OnActorRemovingClothes"sv);

    class OBody {
    public:
        OBody() = default;
        static OBody* GetInstance();

        bool SetMorphInterface(SKEE::IBodyMorphInterface* a_morphInterface);

        void SetMorph(RE::Actor* a_actor, const char* a_morphName, const char* a_key, float a_value);
        float GetMorph(RE::Actor* a_actor, const char* a_morphName);
        void ApplyMorphs(RE::Actor* a_actor, bool updateMorphsWithoutTimer, bool applyProcessedMorph = true);

        void ProcessActorEquipEvent(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor);

        void GenerateActorBody(RE::Actor* a_actor);
        void GenerateBodyByName(RE::Actor* a_actor, std::string a_name);
        void GenerateBodyByPreset(RE::Actor* a_actor, PresetManager::Preset& a_preset, bool updateMorphsWithoutTimer);

        void ApplySlider(RE::Actor* a_actor, PresetManager::Slider& a_slider, const char* a_key, float a_weight);
        void ApplySliderSet(RE::Actor* a_actor, PresetManager::SliderSet& a_sliders, const char* a_key);
        void ApplyClothePreset(RE::Actor* a_actor);
        void RemoveClothePreset(RE::Actor* a_actor);
        void ClearActorMorphs(RE::Actor* a_actor);

        float GetWeight(RE::Actor* a_actor);

        bool IsOutfitBlacklisted(RE::TESObjectARMO* armor);
        bool IsClotheActive(RE::Actor* a_actor);
        bool IsNaked(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor);
        bool IsRemovingClothes(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor);
        bool IsFemale(RE::Actor* a_actor);
        bool IsProcessed(RE::Actor* a_actor);
        bool IsBlacklisted(RE::Actor* a_actor);
        bool ShouldBlacklist(RE::Actor* a_actor);

        PresetManager::SliderSet GenerateRandomNippleSliders();
        PresetManager::SliderSet GenerateRandomGenitalSliders();
        PresetManager::SliderSet GenerateClotheSliders(RE::Actor* a_actor);

        PresetManager::Slider DeriveSlider(RE::Actor* a_actor, const char* a_morph, float a_target);

        bool synthesisInstalled = false;

        bool setRefit = true;
        bool setNippleSlidersRefitEnabled = true;
        bool setNippleRand = true;
        bool setGenitalRand = true;
        bool setPerformanceMode = true;

        std::string distributionKey;

        SKEE::IBodyMorphInterface* morphInterface;
    };
}  // namespace Body
