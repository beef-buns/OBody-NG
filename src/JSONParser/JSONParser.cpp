#include "Body/Body.h"
#include "JSONParser/JSONParser.h"

#include "STL.h"

namespace Parser {
    JSONParser* JSONParser::GetInstance() {
        static JSONParser instance;
        return std::addressof(instance);
    }

    bool GetHasSourceFileArray(RE::TESForm* form) {
        return form->sourceFiles.array;  // Check if the source files array exists
    }

    std::string GetNthFormLocationName(RE::TESForm* form, uint32_t n) {
        std::string formName;

        if (GetHasSourceFileArray(form) && form->sourceFiles.array->size() > n) {
            RE::TESFile** sourceFiles = form->sourceFiles.array->data();
            formName = sourceFiles[n]->fileName;
        }

        // fix for weird bug where refs first defined in Skyrim.Esm aren't always detected properly
        if (((form->formID & 0xFF000000) ==
             0)                           // Refs from Skyrim.ESM will have 00 for the first two hexidecimal digits
            && formName != "Skyrim.esm")  // And refs from all other mods will have a non zero value, so a bitwise && of
                                          // those two digits with FF will be nonzero for all non Skyrim.ESM mods
        {
            formName = "Skyrim.esm";
        }

        return formName;
    }

    bool IsActorInForm(RE::TESNPC* form, std::string target) {
        if (GetHasSourceFileArray(form) && form->sourceFiles.array->size() > 0) {
            RE::TESFile** sourceFiles = form->sourceFiles.array->data();

            for (int i = 0; i < form->sourceFiles.array->size(); i++) {
                if (sourceFiles[i]->fileName == target) {
                    return true;
                }
            }
        }

        return false;
    }

    bool JSONParser::IsActorInBlacklistedCharacterCategorySet(uint32_t formID) {
        for (categorizedList character : blacklistedCharacterCategorySet) {
            if (character.formID == formID) {
                return true;
            }
        }

        return false;
    }

    bool JSONParser::IsOutfitInBlacklistedOutfitCategorySet(uint32_t formID) {
        for (categorizedList outfit : blacklistedOutfitCategorySet) {
            if (outfit.formID == formID) {
                return true;
            }
        }

        return false;
    }

    bool JSONParser::IsOutfitInForceRefitCategorySet(uint32_t formID) {
        for (categorizedList outfit : forceRefitOutfitCategorySet) {
            if (outfit.formID == formID) {
                return true;
            }
        }

        return false;
    }

    categorizedList JSONParser::GetNPCFromCategorySet(uint32_t formID) {
        categorizedList npc;

        for (categorizedList character : characterCategorySet) {
            if (character.formID == formID) {
                npc = character;
                break;
            }
        }

        return npc;
    }

    std::string DiscardFormDigits(std::string formID) {
        std::string newFormID = formID;

        if (formID.size() == 8) {
            newFormID.erase(0, 2);
        }

        return newFormID;
    }

    void JSONParser::ProcessNPCsFormID() {
        if (presetDistributionConfig.contains("npcFormID")) {
            for (auto& plugin : presetDistributionConfig["npcFormID"].items()) {
                std::string owningMod = plugin.key();

                for (auto& form : plugin.value().items()) {
                    std::string formID = DiscardFormDigits(form.key());

                    uint32_t hexnumber;
                    sscanf_s(formID.c_str(), "%x", &hexnumber);

                    auto datahandler = RE::TESDataHandler::GetSingleton();

                    auto actorform = datahandler->LookupForm(hexnumber, owningMod);

                    if (!actorform) {
                        logger::info("{} is not a valid key!", formID);
                        continue;
                    }

                    // We have to use this full-length ID in order to identify them.
                    auto ID = actorform->GetFormID();

                    categorizedList parsedlist;
                    parsedlist.formID = ID;
                    parsedlist.owningMod = owningMod;
                    parsedlist.bodyslidePresets = form.value();

                    characterCategorySet.push_back(parsedlist);
                }
            }
        }
    }

    void JSONParser::ProcessNPCsFormIDBlacklist() {
        if (presetDistributionConfig.contains("blacklistedNpcsFormID")) {
            for (auto& [plugin, val] : presetDistributionConfig["blacklistedNpcsFormID"].items()) {
                for (std::string formIDRaw : val) {
                    std::string formID = DiscardFormDigits(formIDRaw);
                    uint32_t hexnumber;
                    sscanf_s(formID.c_str(), "%x", &hexnumber);

                    auto datahandler = RE::TESDataHandler::GetSingleton();

                    auto actorform = datahandler->LookupForm(hexnumber, plugin);

                    if (!actorform) {
                        logger::info("{} is not a valid key!", formID);
                        continue;
                    }

                    // We have to use this full-length ID in order to identify them.
                    auto ID = actorform->GetFormID();

                    categorizedList parsedlist;
                    parsedlist.formID = ID;
                    parsedlist.owningMod = plugin;

                    blacklistedCharacterCategorySet.push_back(parsedlist);
                }
            }
        }
    }

    void JSONParser::ProcessOutfitsFormIDBlacklist() {
        if (presetDistributionConfig.contains("blacklistedOutfitsFromORefitFormID")) {
            for (auto& [plugin, val] : presetDistributionConfig["blacklistedOutfitsFromORefitFormID"].items()) {
                for (std::string formIDRaw : val) {
                    std::string formID = DiscardFormDigits(formIDRaw);
                    uint32_t hexnumber;
                    sscanf_s(formID.c_str(), "%x", &hexnumber);

                    auto datahandler = RE::TESDataHandler::GetSingleton();

                    auto outfitform = datahandler->LookupForm(hexnumber, plugin);

                    if (!outfitform) {
                        logger::info("{} is not a valid key!", formID);
                        continue;
                    }

                    // We have to use this full-length ID in order to identify them.
                    auto ID = outfitform->GetFormID();

                    categorizedList parsedlist;
                    parsedlist.formID = ID;
                    parsedlist.owningMod = plugin;

                    blacklistedOutfitCategorySet.push_back(parsedlist);
                }
            }
        }
    }

    void JSONParser::ProcessOutfitsForceRefitFormIDBlacklist() {
        if (presetDistributionConfig.contains("outfitsForceRefitFormID")) {
            for (auto& [plugin, val] : presetDistributionConfig["outfitsForceRefitFormID"].items()) {
                for (std::string formIDRaw : val) {
                    std::string formID = DiscardFormDigits(formIDRaw);
                    uint32_t hexnumber;
                    sscanf_s(formID.c_str(), "%x", &hexnumber);

                    auto datahandler = RE::TESDataHandler::GetSingleton();

                    auto outfitform = datahandler->LookupForm(hexnumber, plugin);

                    if (!outfitform) {
                        logger::info("{} is not a valid key!", formID);
                        continue;
                    }

                    // We have to use this full-length ID in order to identify them.
                    auto ID = outfitform->GetFormID();

                    categorizedList parsedlist;
                    parsedlist.formID = ID;
                    parsedlist.owningMod = plugin;

                    forceRefitOutfitCategorySet.push_back(parsedlist);
                }
            }
        }
    }

    void JSONParser::ProcessJSONCategories() {
        ProcessNPCsFormIDBlacklist();
        ProcessNPCsFormID();
        ProcessOutfitsFormIDBlacklist();
        ProcessOutfitsForceRefitFormIDBlacklist();
    }

    bool JSONParser::IsStringInJsonConfigKey(std::string a_value, std::string key) {
        boost::trim(a_value);

        return presetDistributionConfig.contains(key) &&
               std::find(presetDistributionConfig[key].begin(), presetDistributionConfig[key].end(), a_value) !=
                   presetDistributionConfig[key].end();
    }

    bool JSONParser::IsSubKeyInJsonConfigKey(std::string key, std::string subKey) {
        boost::trim(subKey);

        return presetDistributionConfig.contains(key) && presetDistributionConfig[key].contains(subKey);
    }

    bool JSONParser::IsOutfitBlacklisted(RE::TESObjectARMO& a_outfit) {
        if (a_outfit.HasKeywordByEditorID("OBodyRefitBlacklisted")) {
            return true;
        }

        return IsStringInJsonConfigKey(a_outfit.GetName(), "blacklistedOutfitsFromORefit") ||
               IsOutfitInBlacklistedOutfitCategorySet(a_outfit.GetFormID()) ||
               IsStringInJsonConfigKey(GetNthFormLocationName(RE::TESForm::LookupByID(a_outfit.GetFormID()), 0),
                                       "blacklistedOutfitsFromORefitPlugin");
    }

    bool JSONParser::IsAnyForceRefitItemEquipped(RE::Actor* a_actor, bool a_removingArmor,
                                                 RE::TESForm* a_equippedArmor) {
        auto inventory = a_actor->GetInventory();

        std::vector<std::string> wornItems;

        for (auto const& item : inventory) {
            if (item.second.second->IsWorn()) {
                // Check if the item is being unequipped or not first
                if (a_removingArmor && item.first->GetFormID() == a_equippedArmor->GetFormID()) {
                    continue;
                }

                RE::FormType itemFormType = item.first->GetFormType();

                if ((itemFormType == RE::FormType::Armor || itemFormType == RE::FormType::Armature) &&
                        IsStringInJsonConfigKey(item.second.second->GetDisplayName(), "outfitsForceRefit") ||
                    IsOutfitInForceRefitCategorySet(item.first->GetFormID()) ||
                    (item.first->HasKeywordByEditorID("OBodyForceRefit"))) {
                    logger::info("Outfit {} is in force refit list", item.second.second->GetDisplayName());

                    return true;
                }
            }
        }

        return false;
    }

    bool JSONParser::IsNPCBlacklisted(std::string actorName, uint32_t actorID) {
        if (IsStringInJsonConfigKey(actorName, "blacklistedNpcs")) {
            logger::info("{} is Blacklisted by blacklistedNpcs", actorName);
            return true;
        }

        if (IsActorInBlacklistedCharacterCategorySet(actorID)) {
            logger::info("{} is Blacklisted by character category set", actorName);
            return true;
        }

        return false;
    }

    bool JSONParser::IsNPCBlacklistedGlobally(RE::Actor* a_actor, std::string actorRace, bool female) {
        auto actorOwningMod = GetNthFormLocationName(a_actor, 0);

        if (female) {
            return IsStringInJsonConfigKey(actorOwningMod, "blacklistedNpcsPluginFemale") ||
                   IsStringInJsonConfigKey(actorRace, "blacklistedRacesFemale");
        } else {
            return IsStringInJsonConfigKey(actorOwningMod, "blacklistedNpcsPluginMale") ||
                   IsStringInJsonConfigKey(actorRace, "blacklistedRacesMale");
        }
    }

    PresetManager::Preset JSONParser::GetNPCFactionPreset(RE::TESNPC* a_actor, bool female) {
        PresetManager::Preset preset;
        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        auto actorRanks = a_actor->factions;

        std::vector<RE::TESFaction*> actorFactions;

        for (auto rank : actorRanks) {
            actorFactions.push_back(rank.faction);
        }

        if (actorFactions.empty()) {
            return preset;
        }

        PresetManager::PresetSet presetSet =
            female ? presetContainer->allFemalePresets : presetContainer->allMalePresets;

        std::string factionKey = female ? "factionFemale" : "factionMale";

        for (auto& faction : presetDistributionConfig[factionKey].items()) {
            if (std::find(actorFactions.begin(), actorFactions.end(),
                          RE::TESFaction::LookupByEditorID(faction.key())) != actorFactions.end()) {
                preset = PresetManager::GetRandomPresetByName(presetSet, faction.value(), female);
                break;
            }
        }

        return preset;
    }

    PresetManager::Preset JSONParser::GetNPCPreset(std::string actorName, uint32_t formID, bool female) {
        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        PresetManager::Preset preset;

        PresetManager::PresetSet presetSet =
            female ? presetContainer->allFemalePresets : presetContainer->allMalePresets;

        auto character = GetNPCFromCategorySet(formID);

        if (!character.bodyslidePresets.empty()) {
            preset = PresetManager::GetRandomPresetByName(presetSet, character.bodyslidePresets, female);
        } else if (presetDistributionConfig.contains("npc") && presetDistributionConfig["npc"].contains(actorName)) {
            preset =
                PresetManager::GetRandomPresetByName(presetSet, presetDistributionConfig["npc"][actorName], female);
        }

        return preset;
    }

    PresetManager::Preset JSONParser::GetNPCPluginPreset(RE::TESNPC* a_actor, std::string actorName, bool female) {
        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        // auto actorOwningMod = GetNthFormLocationName(a_actor, 0);

        PresetManager::Preset preset;

        PresetManager::PresetSet presetSet =
            female ? presetContainer->allFemalePresets : presetContainer->allMalePresets;

        if (female) {
            if (presetDistributionConfig.contains("npcPluginFemale")) {
                for (auto& [mod, presetList] : presetDistributionConfig["npcPluginFemale"].items()) {
                    logger::info("Checking if actor {} is in mod {}", actorName, mod);

                    if (IsActorInForm(a_actor, mod)) {
                        preset = PresetManager::GetRandomPresetByName(
                            presetSet, presetDistributionConfig["npcPluginFemale"][mod], female);
                        break;
                    }
                }
            }
        } else {
            if (presetDistributionConfig.contains("npcPluginMale")) {
                for (auto& [mod, presetList] : presetDistributionConfig["npcPluginMale"].items()) {
                    if (IsActorInForm(a_actor, mod)) {
                        preset = PresetManager::GetRandomPresetByName(
                            presetSet, presetDistributionConfig["npcPluginMale"][mod], female);
                        break;
                    }
                }
            }
        }

        return preset;
    }

    PresetManager::Preset JSONParser::GetNPCRacePreset(std::string actorRace, bool female) {
        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        PresetManager::Preset preset;

        PresetManager::PresetSet presetSet =
            female ? presetContainer->allFemalePresets : presetContainer->allMalePresets;

        if (female) {
            if (IsSubKeyInJsonConfigKey("raceFemale", actorRace)) {
                preset = PresetManager::GetRandomPresetByName(
                    presetSet, presetDistributionConfig["raceFemale"][actorRace], female);
            }
        } else {
            if (IsSubKeyInJsonConfigKey("raceMale", actorRace)) {
                preset = PresetManager::GetRandomPresetByName(presetSet,
                                                              presetDistributionConfig["raceMale"][actorRace], female);
            }
        }

        return preset;
    }
}  // namespace Parser
