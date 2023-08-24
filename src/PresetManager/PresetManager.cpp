#include "PresetManager/PresetManager.h"

#include "JSONParser/JSONParser.h"
#include "STL.h"

namespace PresetManager {
    const std::set<std::string> DefaultSliders = {"Breasts",   "BreastsSmall", "NippleDistance", "NippleSize",
                                                  "ButtCrack", "Butt",         "ButtSmall",      "Legs",
                                                  "Arms",      "ShoulderWidth"};

    PresetContainer* PresetContainer::GetInstance() {
        static PresetContainer instance;
        return std::addressof(instance);
    }

    void GeneratePresets() {
        const fs::path root_path("Data\\CalienteTools\\BodySlide\\SliderPresets");

        auto container = PresetManager::PresetContainer::GetInstance();

        auto& femalePresets = container->femalePresets;
        auto& malePresets = container->malePresets;

        auto& allFemalePresets = container->allFemalePresets;
        auto& allMalePresets = container->allMalePresets;

        auto& blacklistedFemalePresets = container->blacklistedFemalePresets;
        auto& blacklistedMalePresets = container->blacklistedMalePresets;

        std::vector<std::string> files;
        stl::files(root_path, files, ".xml");

        auto presetDistributionConfig = Parser::JSONParser::GetInstance()->presetDistributionConfig;

        auto blacklistedPresetsBegin = presetDistributionConfig["blacklistedPresetsFromRandomDistribution"].begin();
        auto blacklistedPresetsEnd = presetDistributionConfig["blacklistedPresetsFromRandomDistribution"].end();

        for (auto& entry : files) {
            if (IsClothedSet(entry)) continue;

            pugi::xml_document doc;
            auto result = doc.load_file(entry.c_str(), pugi::parse_default, pugi::encoding_auto);
            if (!result) {
                logger::warn("load failed: {} [{}]", entry, result.description());
                continue;
            }

            auto presets = doc.child("SliderPresets");
            for (auto& node : presets) {
                auto preset = GeneratePreset(node);
                if (!preset) continue;

                if (IsFemalePreset(*preset)) {
                    if (std::find(blacklistedPresetsBegin, blacklistedPresetsEnd, preset.value().name) !=
                        blacklistedPresetsEnd) {
                        blacklistedFemalePresets.push_back(*preset);
                    } else {
                        femalePresets.push_back(*preset);
                    }
                } else {
                    if (std::find(blacklistedPresetsBegin, blacklistedPresetsEnd, preset.value().name) !=
                        blacklistedPresetsEnd) {
                        blacklistedMalePresets.push_back(*preset);
                    } else {
                        malePresets.push_back(*preset);
                    }
                }
            }
        }

        allFemalePresets = femalePresets;
        allFemalePresets.insert(allFemalePresets.end(), blacklistedFemalePresets.begin(),
                                blacklistedFemalePresets.end());

        allMalePresets = malePresets;
        allMalePresets.insert(allMalePresets.end(), blacklistedMalePresets.begin(), blacklistedMalePresets.end());

        logger::info("Female presets: {}", femalePresets.size());
        logger::info("Male presets: {}", malePresets.size());
        logger::info("Blacklisted Female presets: {}", blacklistedFemalePresets.size());
    }

    std::optional<Preset> GeneratePreset(pugi::xml_node a_node) {
        std::string name = a_node.attribute("name").value();

        // Some preset names have dumb trailing whitespaces which may mess up the configuration file...
        boost::trim(name);

        if (IsClothedSet(name)) return std::nullopt;

        std::string body = a_node.attribute("set").value();
        auto sliderSet = SliderSetFromNode(a_node, GetBodyType(body));

        Preset preset{name, body};
        preset.sliders = sliderSet;
        return preset;
    }

    Preset GetPresetByName(PresetSet a_presetSet, std::string a_name, bool female) {
        logger::info("Looking for preset: {}", a_name);

        boost::trim(a_name);
        boost::to_upper(a_name);

        for (auto& preset : a_presetSet) {
            if (stl::cmp(boost::to_upper_copy(preset.name), a_name)) return preset;
        }

        logger::info("Preset not found, choosing a random one.");
        auto container = PresetManager::PresetContainer::GetInstance();
        return GetRandomPreset(female ? container->femalePresets : container->malePresets);
    }

    Preset GetRandomPreset(PresetSet a_presetSet) {
        std::random_device seed;
        // generator
        std::mt19937 engine(seed());
        // number distribution
        std::uniform_int_distribution<int> choose(0, a_presetSet.size() - 1);
        return a_presetSet[choose(engine)];
    }

    Preset GetPresetByNameForRandom(PresetSet a_presetSet, std::string a_name, bool female) {
        logger::info("Looking for preset: {}", a_name);

        boost::trim(a_name);
        boost::to_upper(a_name);

        Preset presetRet;

        for (auto& preset : a_presetSet) {
            if (stl::cmp(boost::to_upper_copy(preset.name), a_name)) {
                presetRet = preset;
                break;
            }
        }

        return presetRet;
    }

    Preset GetRandomPresetByName(PresetSet a_presetSet, std::vector<std::string> a_presetNames, bool female) {
        if (!a_presetNames.size()) {
            logger::info("Preset names size is empty, returning a random one");
            auto container = PresetManager::PresetContainer::GetInstance();
            return GetRandomPreset(female ? container->femalePresets : container->malePresets);
        }

        std::random_device seed;
        // generator
        std::mt19937 engine(seed());
        // number distribution
        auto size = static_cast<int>(a_presetNames.size());
        std::uniform_int_distribution<int> choose(0, size - 1);

        std::string chosenPreset = a_presetNames[choose(engine)];

        Preset preset = GetPresetByNameForRandom(a_presetSet, chosenPreset, female);

        if (preset.name.empty()) {
            auto iterator = std::find(a_presetNames.begin(), a_presetNames.end(), chosenPreset);

            if (iterator != a_presetNames.end()) a_presetNames.erase(iterator);

            return GetRandomPresetByName(a_presetSet, a_presetNames, female);
        }

        return preset;
    }

    bool IsFemalePreset(Preset a_preset) {
        std::vector<std::string> body{"himbo", "talos", "sam", "sos", "savren"};
        return !stl::contains(a_preset.body, body);
    }

    bool IsClothedSet(std::string a_set) {
        std::string presetName = a_set;

        std::vector<std::string> clothed{"cloth", "outfit", "nevernude", "bikini", "feet",
                                         "hands", "push",   "cleavage",  "armor"};

        std::transform(presetName.begin(), presetName.end(), presetName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        return stl::contains(presetName, clothed);
    }

    SliderSet SliderSetFromNode(pugi::xml_node& a_node, BodyType a_body) {
        SliderSet ret;

        for (auto& node : a_node) {
            if (!stl::cmp(node.name(), "SetSlider")) continue;

            std::string name = node.attribute("name").value();

            bool inverted = false;
            if (a_body == BodyType::UNP) {
                if (DefaultSliders.contains(name)) inverted = true;
            }

            float min{0}, max{0};
            float val = node.attribute("value").as_float() / 100.0f;
            auto size = node.attribute("size").value();
            if (stl::cmp(size, "big"))
                max = inverted ? 1.0f - val : val;
            else
                min = inverted ? 1.0f - val : val;

            Slider slider{name, min, max};
            AddSliderToSet(ret, slider, inverted);
        }

        return ret;
    }

    void AddSliderToSet(SliderSet& a_sliderSet, Slider a_slider, [[maybe_unused]] bool a_inverted) {
        float val = 0;

        auto it = a_sliderSet.find(a_slider.name);

        if (it != a_sliderSet.end()) {
            auto& current = it->second;
            if ((current.min == val) && (a_slider.min != val)) current.min = a_slider.min;
            if ((current.max == val) && (a_slider.max != val)) current.max = a_slider.max;
        } else {
            a_sliderSet[a_slider.name] = std::move(a_slider);
        }
    }

    BodyType GetBodyType(std::string a_body) {
        std::vector<std::string> unp{"unp", "coco", "bhunp", "uunp"};
        return stl::contains(a_body, unp) ? BodyType::UNP : BodyType::CBBE;
    }
}  // namespace PresetManager
