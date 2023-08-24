#pragma once

namespace Parser {
    struct categorizedList {
        std::string owningMod = "";
        uint32_t formID = 0;
        std::vector<std::string> bodyslidePresets;
    };

    class JSONParser {
    public:
        JSONParser() = default;
        static JSONParser* GetInstance();

        void ProcessNPCsFormID();
        void ProcessNPCsFormIDBlacklist();
        void ProcessOutfitsFormIDBlacklist();
        void ProcessOutfitsForceRefitFormIDBlacklist();

        void ProcessJSONCategories();

        bool IsActorInBlacklistedCharacterCategorySet(uint32_t formID);
        bool IsOutfitInBlacklistedOutfitCategorySet(uint32_t formID);
        bool IsOutfitInForceRefitCategorySet(uint32_t formID);

        categorizedList GetNPCFromCategorySet(uint32_t formID);
        bool IsStringInJsonConfigKey(std::string a_value, std::string key);
        bool IsSubKeyInJsonConfigKey(std::string key, std::string subKey);

        bool IsOutfitBlacklisted(RE::TESObjectARMO& a_outfit);
        bool IsAnyForceRefitItemEquipped(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor);
        bool IsNPCBlacklisted(std::string actorName, uint32_t actorID);
        bool IsNPCBlacklistedGlobally(RE::Actor* a_actor, std::string actorRace, bool female);

        PresetManager::Preset GetNPCFactionPreset(RE::TESNPC* a_actor, bool female);

        PresetManager::Preset GetNPCPreset(std::string actorName, uint32_t formID, bool female);
        PresetManager::Preset GetNPCPluginPreset(RE::TESNPC* a_actor, std::string actorName, bool female);
        PresetManager::Preset GetNPCRacePreset(std::string actorRace, bool female);

        nlohmann::ordered_json presetDistributionConfig;
        bool presetDistributionConfigValid;
        bool bodyslidePresetsParsingValid;

        std::vector<categorizedList> blacklistedCharacterCategorySet;
        std::vector<categorizedList> characterCategorySet;

        std::vector<categorizedList> blacklistedOutfitCategorySet;
        std::vector<categorizedList> forceRefitOutfitCategorySet;
    };
}  // namespace Parser
