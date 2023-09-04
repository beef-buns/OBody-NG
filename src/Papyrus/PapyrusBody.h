#pragma once

#include "Body/Body.h"
#include "JSONParser/JSONParser.h"
#include "PresetManager/PresetManager.h"

namespace PapyrusBody {
    using VM = RE::BSScript::IVirtualMachine;

    void GenActor(RE::StaticFunctionTag*, RE::Actor* a_actor) {
        auto obody = Body::OBody::GetInstance();
        obody->GenerateActorBody(a_actor);
    }

    void SetORefit(RE::StaticFunctionTag*, bool a_enabled) {
        auto obody = Body::OBody::GetInstance();
        obody->setRefit = a_enabled;
    }

    void SetNippleRand(RE::StaticFunctionTag*, bool a_enabled) {
        auto obody = Body::OBody::GetInstance();
        obody->setNippleRand = a_enabled;
    }

    void SetGenitalRand(RE::StaticFunctionTag*, bool a_enabled) {
        auto obody = Body::OBody::GetInstance();
        obody->setGenitalRand = a_enabled;
    }

    void SetPerformanceMode(RE::StaticFunctionTag*, bool a_enabled) {
        auto obody = Body::OBody::GetInstance();
        obody->setPerformanceMode = a_enabled;
    }

    void SetDistributionKey(RE::StaticFunctionTag*, std::string a_distributionKey) {
        auto obody = Body::OBody::GetInstance();
        obody->distributionKey = a_distributionKey;
    }

    int GetFemaleDatabaseSize(RE::StaticFunctionTag*) {
        auto presetContainer = PresetManager::PresetContainer::GetInstance();
        return static_cast<int>(presetContainer->femalePresets.size());
    }

    int GetMaleDatabaseSize(RE::StaticFunctionTag*) {
        auto presetContainer = PresetManager::PresetContainer::GetInstance();
        return static_cast<int>(presetContainer->malePresets.size());
    }

    void RegisterForOBodyEvent(RE::StaticFunctionTag*, RE::TESQuest* a_quest) {
        Body::OnActorGenerated.Register(a_quest);
    }

    void RegisterForOBodyNakedEvent(RE::StaticFunctionTag*, RE::TESQuest* a_quest) {
        Body::OnActorNaked.Register(a_quest);
    }

    void RegisterForOBodyRemovingClothesEvent(RE::StaticFunctionTag*, RE::TESQuest* a_quest) {
        Body::OnActorRemovingClothes.Register(a_quest);
    }

    void ApplyPresetByName(RE::StaticFunctionTag*, RE::Actor* a_actor, std::string a_name) {
        auto obody = Body::OBody::GetInstance();
        obody->GenerateBodyByName(a_actor, a_name);
    }

    void AddClothesOverlay(RE::StaticFunctionTag*, RE::Actor* a_actor) {
        auto obody = Body::OBody::GetInstance();
        obody->ApplyClothePreset(a_actor);
        obody->ApplyMorphs(a_actor, true);
    }

    void ResetActorOBodyMorphs(RE::StaticFunctionTag*, RE::Actor* a_actor) {
        auto obody = Body::OBody::GetInstance();
        obody->ClearActorMorphs(a_actor);
    }

	bool presetNameComparison(std::string a, std::string b) {
        return boost::algorithm::to_lower_copy(a) < boost::algorithm::to_lower_copy(b);
    } 

    auto GetAllPossiblePresets(RE::StaticFunctionTag*, RE::Actor* a_actor) {
        std::vector<std::string> ret;
        auto presetContainer = PresetManager::PresetContainer::GetInstance();
        auto obody = Body::OBody::GetInstance();
        auto presetDistributionConfig = Parser::JSONParser::GetInstance()->presetDistributionConfig;

        bool showBlacklistedPresets = false;

        // For some reason, some users get CTDs when reading this value, even when everything else seems correct...
        // Also happens when used with Immersive Equipment displays. I have no idea why.
        // Catch the error, default to True in case something fails
        try {
            showBlacklistedPresets = presetDistributionConfig.contains("blacklistedPresetsShowInOBodyMenu") &&
                                     presetDistributionConfig["blacklistedPresetsShowInOBodyMenu"];
        } catch (json::type_error) {
            logger::info(
                "Failed to read blacklistedPresetsShowInOBodyMenu key. Defaulting to showing the blacklisted presets "
                "in OBody menu.");
            showBlacklistedPresets = true;
        }

        if (obody->IsFemale(a_actor)) {
            if (showBlacklistedPresets) {
                for (auto& preset : presetContainer->allFemalePresets) ret.push_back(preset.name);
            } else {
                for (auto& preset : presetContainer->femalePresets) ret.push_back(preset.name);
            }
        } else {
            if (showBlacklistedPresets) {
                for (auto& preset : presetContainer->allMalePresets) ret.push_back(preset.name);
            } else {
                for (auto& preset : presetContainer->malePresets) ret.push_back(preset.name);
            }
        }

		std::sort(ret.begin(), ret.end(), presetNameComparison);

        return ret;
    }

    bool Bind(VM* a_vm) {
        const auto obj = "OBodyNative"sv;

        BIND(GenActor);
        BIND(ApplyPresetByName);
        BIND(GetAllPossiblePresets);
        BIND(AddClothesOverlay);
        BIND(RegisterForOBodyEvent);
        BIND(RegisterForOBodyNakedEvent);
        BIND(RegisterForOBodyRemovingClothesEvent);
        BIND(GetFemaleDatabaseSize);
        BIND(GetMaleDatabaseSize);
        BIND(ResetActorOBodyMorphs);

        BIND(SetORefit);
        BIND(SetNippleRand);
        BIND(SetGenitalRand);
        BIND(SetPerformanceMode);
        BIND(SetDistributionKey);

        return true;
    }
}  // namespace PapyrusBody
