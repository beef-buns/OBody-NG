#include "Body/Body.h"

#include "JSONParser/JSONParser.h"
#include "STL.h"

using namespace PresetManager;

const auto OBODY_REFIT_BLACKLISTED = "OBodyRefitBlacklisted";
const auto OBODY_FORCE_REFIT = "OBodyForceRefit";

namespace Body {
    OBody* OBody::GetInstance() {
        static OBody instance;
        return std::addressof(instance);
    }

    bool OBody::SetMorphInterface(SKEE::IBodyMorphInterface* a_morphInterface) {
        return a_morphInterface->GetVersion() ? morphInterface = a_morphInterface : false;
    }

    void OBody::SetMorph(RE::Actor* a_actor, const char* a_morphName, const char* a_key, float a_value) {
        morphInterface->SetMorph(a_actor, a_morphName, a_key, a_value);
    }

    float OBody::GetMorph(RE::Actor* a_actor, const char* a_morphName) {
        return morphInterface->GetMorph(a_actor, a_morphName, "OBody");
    }

    void OBody::ApplyMorphs(RE::Actor* a_actor, bool updateMorphsWithoutTimer, bool applyProcessedMorph) {
        // If updateMorphsWithoutTimer is true, OBody NG will call the ApplyBodyMorphs function without waiting a random
        // amount of time. That is useful for undressing/redressing.
        // If performance mode is turned off, we also apply morphs randomly immediately no matter the context.

        RE::ActorHandle actorHandle = a_actor->GetHandle();

        if (updateMorphsWithoutTimer || !setPerformanceMode) {
            RE::Actor* actor = actorHandle.get().get();

            if (actor != nullptr) {
                if (applyProcessedMorph) {
                    SetMorph(actor, distributionKey.c_str(), "OBody", 1.0f);
                }

                if (actor && actor->Is3DLoaded()) {
                    morphInterface->ApplyBodyMorphs(actor, true);
                    morphInterface->UpdateModelWeight(actor, false);
                }
            }
        } else {
            auto actorName = a_actor->GetActorBase()->GetName();

            unsigned long seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::mt19937 rng(seed);
            std::uniform_int_distribution<int> gen(3, 7);

            int sleepFor = gen(rng);

            // We do this to prevent stutters due to Racemenu attempting to update morphs for too many NPCs
            std::thread([this, actorHandle, actorName, sleepFor] {
                std::this_thread::sleep_for(std::chrono::seconds(sleepFor));

                RE::Actor* actor = actorHandle.get().get();

                if (actor != nullptr) {
                    logger::info("Actor {} is valid, updating morphs now", actorName);

                    SetMorph(actor, distributionKey.c_str(), "OBody", 1.0f);

                    if (actor && actor->Is3DLoaded() &&
                        !morphInterface->HasBodyMorph(actor, "obody_synthebd", "OBody")) {
                        morphInterface->ApplyBodyMorphs(actor, true);
                        morphInterface->UpdateModelWeight(actor, false);
                    }
                } else {
                    logger::info("Actor {} is no longer valid, not updating morphs", actorName);
                }
            }).detach();
        }
    }

    void OBody::ProcessActorEquipEvent(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor) {
        if (!IsProcessed(a_actor) || IsBlacklisted(a_actor)) return;

        if (IsRemovingClothes(a_actor, a_removingArmor, a_equippedArmor)) {
            OnActorRemovingClothes.SendEvent(a_actor);
        }

        // if ORefit is disabled and actor has ORefit morphs, clear them right away.
        if (!setRefit && IsClotheActive(a_actor)) {
            RemoveClothePreset(a_actor);
            ApplyMorphs(a_actor, true);
            return;
        }

        bool female = IsFemale(a_actor);

        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        if ((female && presetContainer->femalePresets.size() < 1) ||
            !female && presetContainer->malePresets.size() < 1) {
            return;
        }

        bool naked = IsNaked(a_actor, a_removingArmor, a_equippedArmor);
        bool clotheActive = IsClotheActive(a_actor);

        if (!(naked) && (a_removingArmor)) {
            // Fires when removing their armor
            OnActorNaked.SendEvent(a_actor);
        }

        if (clotheActive && naked) {
            logger::info("Removing clothed preset to actor {}", a_actor->GetName());
            RemoveClothePreset(a_actor);
            ApplyMorphs(a_actor, true);
        } else if (!clotheActive && !naked && setRefit) {
            logger::info("Applying clothed preset to actor {}", a_actor->GetName());
            ApplyClothePreset(a_actor);
            ApplyMorphs(a_actor, true);
        }
    }

    void OBody::GenerateActorBody(RE::Actor* a_actor) {
        // The main function of OBody NG

        // If actor is already processed, no need to do anything
        if (IsProcessed(a_actor)) {
            return;
        }

        bool female = IsFemale(a_actor);

        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        // If we have no presets at all for the actor's sex, then don't do anything
        if ((female && presetContainer->femalePresets.size() < 1) ||
            !female && presetContainer->malePresets.size() < 1) {
            return;
        }

        PresetManager::Preset preset;

        auto jsonParser = Parser::JSONParser::GetInstance();

        auto actorBase = a_actor->GetActorBase();
        auto actorName = actorBase->GetName();
        auto actorID = actorBase->GetFormID();

        logger::info("Trying to find and apply preset to {}", actorName);

        // If NPC is blacklisted, set him as processed
        if (a_actor->HasKeywordString(OBODY_REFIT_BLACKLISTED) || jsonParser->IsNPCBlacklisted(actorName, actorID)) {
            SetMorph(a_actor, distributionKey.c_str(), "OBody", 1.0f);
            SetMorph(a_actor, "obody_blacklisted", "OBody", 1.0f);
            return;
        }

        // First, we attempt to get the NPC's preset from the keys npcFormID and npc from the JSON
        preset = jsonParser->GetNPCPreset(actorName, actorID, female);

        if (preset.name.size() == 0) {
            auto actorRace = actorBase->GetRace()->GetFormEditorID();

            // if we can't find it, we check if the NPC is blacklisted by plugin name or by race
            if (jsonParser->IsNPCBlacklistedGlobally(a_actor, actorRace, female)) {
                SetMorph(a_actor, distributionKey.c_str(), "OBody", 1.0f);
                SetMorph(a_actor, "obody_blacklisted", "OBody", 1.0f);
                return;
            }

            // Next up, we check if we have a preset defined in one of the NPC's factions
            preset = jsonParser->GetNPCFactionPreset(actorBase, female);

            // If that also fails, we check if we have a preset in the NPC's plugin
            if (preset.name.size() == 0) {
                preset = jsonParser->GetNPCPluginPreset(actorBase, actorName, female);
            }

            // And if that also fails, we check if we have a preset in the NPC's race
            if (preset.name.size() == 0) {
                preset = jsonParser->GetNPCRacePreset(actorRace, female);
            }
        }

        // If we got here without a preset, then we just fetch one randomly
        if (preset.name.size() == 0) {
            logger::info("No preset defined for this actor, getting it randomly");
            if (female) {
                preset = PresetManager::GetRandomPreset(presetContainer->femalePresets);
            } else {
                preset = PresetManager::GetRandomPreset(presetContainer->malePresets);
            }
        }

        logger::info("Preset {} will be applied to {}", preset.name, actorName);

        GenerateBodyByPreset(a_actor, preset, false);
    }

    void OBody::GenerateBodyByName(RE::Actor* a_actor, std::string a_name) {
        PresetManager::Preset preset;
        auto presetContainer = PresetManager::PresetContainer::GetInstance();

        // This is needed to prevent a crash with SynthEBD/Synthesis
        if (synthesisInstalled && a_actor != nullptr) {
            SetMorph(a_actor, "obody_synthebd", "OBody", 1.0f);
        }

        if (IsFemale(a_actor))
            preset = PresetManager::GetPresetByName(presetContainer->allFemalePresets, a_name, true);
        else
            preset = PresetManager::GetPresetByName(presetContainer->allMalePresets, a_name, false);

        GenerateBodyByPreset(a_actor, preset, true);
    }

    void OBody::GenerateBodyByPreset(RE::Actor* a_actor, PresetManager::Preset& a_preset,
                                     bool updateMorphsWithoutTimer) {
        // Start by clearing any previous OBody morphs
        morphInterface->ClearMorphs(a_actor);

        // Apply the preset's sliders
        ApplySliderSet(a_actor, a_preset.sliders, "OBody");

        logger::info("Applying preset: {}", a_preset.name);

        if (IsFemale(a_actor)) {
            // Generate random nipple sliders if needed
            if (setNippleRand) {
                PresetManager::SliderSet set = GenerateRandomNippleSliders();
                ApplySliderSet(a_actor, set, "OBody");
            }

            if (setGenitalRand) {
                // Generate random genital sliders if needed
                PresetManager::SliderSet set = GenerateRandomGenitalSliders();
                ApplySliderSet(a_actor, set, "OBody");
            }
        }

        // If not naked and if ORefit is turned on, apply ORefit morphing
        if (!IsNaked(a_actor, false, nullptr)) {
            if (setRefit) {
                logger::info("Not naked, adding cloth preset");
                ApplyClothePreset(a_actor);
            }
        } else {
            logger::info("Actor is naked, not applying cloth preset");
            OnActorNaked.SendEvent(a_actor);
        }

        ApplyMorphs(a_actor, updateMorphsWithoutTimer);
        OnActorGenerated.SendEvent(a_actor, a_preset.name);
    }

    void OBody::ApplySlider(RE::Actor* a_actor, PresetManager::Slider& a_slider, const char* a_key, float a_weight) {
        float val = ((a_slider.max - a_slider.min) * a_weight) + a_slider.min;
        morphInterface->SetMorph(a_actor, a_slider.name.c_str(), a_key, val);
    }

    void OBody::ApplySliderSet(RE::Actor* a_actor, PresetManager::SliderSet& a_sliders, const char* a_key) {
        float weight = GetWeight(a_actor);
        for (auto& [name, slider] : a_sliders) ApplySlider(a_actor, slider, a_key, weight);
    }

    void OBody::ApplyClothePreset(RE::Actor* a_actor) {
        auto set = GenerateClotheSliders(a_actor);
        ApplySliderSet(a_actor, set, "OClothe");
    }

    void OBody::ClearActorMorphs(RE::Actor* a_actor) {
        morphInterface->ClearBodyMorphKeys(a_actor, "OBody");
        morphInterface->ClearBodyMorphKeys(a_actor, "OClothe");
        ApplyMorphs(a_actor, true, false);
    }

    void OBody::RemoveClothePreset(RE::Actor* a_actor) { morphInterface->ClearBodyMorphKeys(a_actor, "OClothe"); }

    float OBody::GetWeight(RE::Actor* a_actor) { return a_actor->GetActorBase()->GetWeight() / 100.0f; }

    bool OBody::IsClotheActive(RE::Actor* a_actor) { return morphInterface->HasBodyMorphKey(a_actor, "OClothe"); }

    bool OBody::IsOutfitBlacklisted(RE::TESObjectARMO* armor) {
        auto jsonParser = Parser::JSONParser::GetInstance();

        return armor->HasKeywordString(OBODY_REFIT_BLACKLISTED) || jsonParser->IsOutfitBlacklisted(*armor);
    }

    bool OBody::IsNaked(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor) {
        auto jsonParser = Parser::JSONParser::GetInstance();

        auto outfitBody = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kBody);
        auto outergarmentChest = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModChestPrimary);
        auto undergarmentChest = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModChestSecondary);

        // When the TES EquipEvent is sent, the inventory isn't updated yet
        // So we have to check if any of these armors is being removed...
        if (a_removingArmor) {
            if (outfitBody == a_equippedArmor) {
                outfitBody = nullptr;
            } else if (outergarmentChest == a_equippedArmor) {
                outergarmentChest = nullptr;
            } else if (undergarmentChest == a_equippedArmor) {
                undergarmentChest = nullptr;
            }
        }

        // if outfit is blacklisted from ORefit, we assume as not having the outfit so ORefit is not applied
        bool hasBodyOutfit = outfitBody == nullptr ? false : !IsOutfitBlacklisted(outfitBody);
        bool hasOutergarment = outergarmentChest == nullptr ? false : !IsOutfitBlacklisted(outergarmentChest);
        bool hasUndergarment = undergarmentChest == nullptr ? false : !IsOutfitBlacklisted(undergarmentChest);

        // Actor counts as naked if:
        // he has no clothing in the slots defined above / they are blacklisted from ORefit
        // if the items in the outfitsForceRefit key are not equipped
        return !hasBodyOutfit && !hasOutergarment && !hasUndergarment &&
               !jsonParser->IsAnyForceRefitItemEquipped(a_actor, a_removingArmor, a_equippedArmor);
    }

    bool OBody::IsRemovingClothes(RE::Actor* a_actor, bool a_removingArmor, RE::TESForm* a_equippedArmor) {
        auto outfitBody = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kBody);
        auto outergarmentChest = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModChestPrimary);
        auto undergarmentChest = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModChestSecondary);
        auto outergarmentPelvis = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModPelvisPrimary);
        auto undergarmentPelvis = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kModPelvisSecondary);

        bool isRemovingBody = false;
        bool isRemovingOuterGarmentChest = false;
        bool isRemovingUnderGarmentChest = false;
        bool isRemovingOuterGarmentPelvis = false;
        bool isRemovingUnderGarmentPelvis = false;

        // When the TES EquipEvent is sent, the inventory isn't updated yet
        // So we have to check if any of these armors is being removed...
        if (a_removingArmor) {
            if (outfitBody == a_equippedArmor) {
                isRemovingBody = true;
            } else if (outergarmentChest == a_equippedArmor) {
                isRemovingOuterGarmentChest = true;
            } else if (undergarmentChest == a_equippedArmor) {
                isRemovingUnderGarmentChest = true;
            } else if (outergarmentPelvis == a_equippedArmor) {
                isRemovingOuterGarmentPelvis = true;
            } else if (undergarmentPelvis == a_equippedArmor) {
                isRemovingUnderGarmentPelvis = true;
            }
        }

        return isRemovingBody || isRemovingOuterGarmentChest || isRemovingUnderGarmentChest ||
               isRemovingOuterGarmentPelvis || isRemovingUnderGarmentPelvis;
    }

    bool OBody::IsFemale(RE::Actor* a_actor) { return a_actor->GetActorBase()->GetSex() == 1; }

    bool OBody::IsProcessed(RE::Actor* a_actor) {
        return morphInterface->HasBodyMorph(a_actor, distributionKey.c_str(), "OBody");
    }

    bool OBody::IsBlacklisted(RE::Actor* a_actor) {
        return morphInterface->HasBodyMorph(a_actor, "obody_blacklisted", "OBody");
    }

    PresetManager::SliderSet OBody::GenerateRandomNippleSliders() {
        PresetManager::SliderSet set;

        if (stl::chance(15))
            AddSliderToSet(set, Slider{"AreolaSize", stl::random(-1.0f, 0.0f)});
        else
            AddSliderToSet(set, Slider{"AreolaSize", stl::random(0.0f, 1.0f)});

        if (stl::chance(75)) AddSliderToSet(set, Slider{"AreolaPull_v2", stl::random(-0.25f, 1.0f)});

        if (stl::chance(15))
            AddSliderToSet(set, Slider{"NippleLength", stl::random(0.2f, 0.3f)});
        else
            AddSliderToSet(set, Slider{"NippleLength", stl::random(0.0f, 0.1f)});

        AddSliderToSet(set, Slider{"NippleManga", stl::random(-0.3f, 0.8f)});

        if (stl::chance(25)) AddSliderToSet(set, Slider{"NipplePerkManga", stl::random(-0.3f, 1.2f)});

        if (stl::chance(15)) AddSliderToSet(set, Slider{"NipBGone", stl::random(0.6f, 1.0f)});

        AddSliderToSet(set, Slider{"NippleSize", stl::random(-0.5f, 0.3f)});
        AddSliderToSet(set, Slider{"NippleDip", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"NippleCrease_v2", stl::random(-0.4f, 1.0f)});

        if (stl::chance(6)) AddSliderToSet(set, Slider{"NipplePuffy_v2", stl::random(0.4f, 0.7f)});

        if (stl::chance(35)) AddSliderToSet(set, Slider{"NippleThicc_v2", stl::random(0.0f, 0.9f)});

        if (stl::chance(2)) {
            if (stl::chance(50))
                AddSliderToSet(set, Slider{"NippleInvert_v2", 1.0f});
            else
                AddSliderToSet(set, Slider{"NippleInvert_v2", stl::random(0.65f, 0.8f)});
        }

        return set;
    }

    PresetManager::SliderSet OBody::GenerateRandomGenitalSliders() {
        PresetManager::SliderSet set;

        if (stl::chance(20)) {
            // innie
            AddSliderToSet(set, Slider{"Innieoutie", stl::random(0.95f, 1.1f)});

            if (stl::chance(50)) AddSliderToSet(set, Slider{"Labiapuffyness", stl::random(0.75f, 1.25f)});

            if (stl::chance(40)) AddSliderToSet(set, Slider{"LabiaMorePuffyness_v2", stl::random(0.0f, 1.0f)});

            AddSliderToSet(set, Slider{"Labiaprotrude", stl::random(0.0f, 0.5f)});
            AddSliderToSet(set, Slider{"Labiaprotrude2", stl::random(0.0f, 0.1f)});
            AddSliderToSet(set, Slider{"Labiaprotrudeback", stl::random(0.0f, 0.1f)});
            AddSliderToSet(set, Slider{"Labiaspread", 0.0f});
            AddSliderToSet(set, Slider{"LabiaCrumpled_v2", stl::random(0.0f, 0.3f)});
            AddSliderToSet(set, Slider{"LabiaBulgogi_v2", 0.0f});
            AddSliderToSet(set, Slider{"LabiaNeat_v2", 0.0f});
            AddSliderToSet(set, Slider{"VaginaHole", stl::random(-0.2f, 0.05f)});
            AddSliderToSet(set, Slider{"Clit", stl::random(-0.4f, 0.25f)});
        } else if (stl::chance(75)) {
            // average
            AddSliderToSet(set, Slider{"Innieoutie", stl::random(0.4f, 0.75f)});

            if (stl::chance(40)) AddSliderToSet(set, Slider{"Labiapuffyness", stl::random(0.5f, 1.0f)});

            if (stl::chance(30)) AddSliderToSet(set, Slider{"LabiaMorePuffyness_v2", stl::random(0.0f, 0.75f)});

            AddSliderToSet(set, Slider{"Labiaprotrude", stl::random(0.0f, 0.5f)});
            AddSliderToSet(set, Slider{"Labiaprotrude2", stl::random(0.0f, 0.75f)});
            AddSliderToSet(set, Slider{"Labiaprotrudeback", stl::random(0.0f, 1.0f)});

            if (stl::chance(50)) {
                AddSliderToSet(set, Slider{"Labiaspread", stl::random(0.0f, 1.0f)});
                AddSliderToSet(set, Slider{"LabiaCrumpled_v2", stl::random(0.0f, 0.7f)});

                if (stl::chance(60)) AddSliderToSet(set, Slider{"LabiaBulgogi_v2", stl::random(0.0f, 0.1f)});
            } else {
                AddSliderToSet(set, Slider{"Labiaspread", 0.0f});
                AddSliderToSet(set, Slider{"LabiaCrumpled_v2", stl::random(0.0f, 0.2f)});

                if (stl::chance(45)) AddSliderToSet(set, Slider{"LabiaBulgogi_v2", stl::random(0.0f, 0.3f)});
            }

            AddSliderToSet(set, Slider{"LabiaNeat_v2", 0.0f});
            AddSliderToSet(set, Slider{"VaginaHole", stl::random(-0.2f, 0.40f)});
            AddSliderToSet(set, Slider{"Clit", stl::random(-0.2f, 0.25f)});
        } else {
            // outie
            AddSliderToSet(set, Slider{"Innieoutie", stl::random(-0.25f, 0.30f)});

            if (stl::chance(30)) AddSliderToSet(set, Slider{"Labiapuffyness", stl::random(0.20f, 0.50f)});

            if (stl::chance(10)) AddSliderToSet(set, Slider{"LabiaMorePuffyness_v2", stl::random(0.0f, 0.35f)});

            AddSliderToSet(set, Slider{"Labiaprotrude", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"Labiaprotrude2", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"Labiaprotrudeback", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"Labiaspread", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"LabiaCrumpled_v2", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"LabiaBulgogi_v2", stl::random(0.0f, 1.0f)});

            if (stl::chance(40)) AddSliderToSet(set, Slider{"LabiaNeat_v2", stl::random(0.0f, 0.25f)});

            AddSliderToSet(set, Slider{"VaginaHole", stl::random(0.0f, 1.0f)});
            AddSliderToSet(set, Slider{"Clit", stl::random(-0.4f, 0.25f)});
        }

        AddSliderToSet(set, Slider{"Vaginasize", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"ClitSwell_v2", stl::random(-0.3f, 1.1f)});
        AddSliderToSet(set, Slider{"Cutepuffyness", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"LabiaTightUp", stl::random(0.0f, 1.0f)});

        if (stl::chance(60))
            AddSliderToSet(set, Slider{"CBPC", stl::random(-0.25f, 0.25f)});
        else
            AddSliderToSet(set, Slider{"CBPC", stl::random(0.6f, 1.0f)});

        AddSliderToSet(set, Slider{"AnalPosition_v2", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"AnalTexPos_v2", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"AnalTexPosRe_v2", stl::random(0.0f, 1.0f)});
        AddSliderToSet(set, Slider{"AnalLoose_v2", -0.1f});

        return set;
    }

    PresetManager::SliderSet OBody::GenerateClotheSliders(RE::Actor* a_actor) {
        PresetManager::SliderSet set;
        // breasts
        // make area on sides behind breasts not sink in
        AddSliderToSet(set, DeriveSlider(a_actor, "BreastSideShape", 0.0f));
        // make area under breasts not sink in
        AddSliderToSet(set, DeriveSlider(a_actor, "BreastUnderDepth", 0.0f));
        // push breasts together
        AddSliderToSet(set, DeriveSlider(a_actor, "BreastCleavage", 1.0f));
        // push up smaller breasts more
        AddSliderToSet(set, Slider{"BreastGravity2", -0.1f, -0.05f});
        // Make top of breast rise higher
        AddSliderToSet(set, Slider{"BreastTopSlope", -0.2f, -0.35f});
        // push breasts together
        AddSliderToSet(set, Slider{"BreastsTogether", 0.3f, 0.35f});
        // push breasts up
        // AddSliderToSet(set, Slider{ "PushUp", 0.6f, 0.4f });
        // Shrink breasts slightly
        AddSliderToSet(set, Slider{"Breasts", -0.05f});
        // Move breasts up on body slightly
        AddSliderToSet(set, Slider{"BreastHeight", 0.15f});

        // butt
        // remove butt impressions
        AddSliderToSet(set, DeriveSlider(a_actor, "ButtDimples", 0.0f));
        AddSliderToSet(set, DeriveSlider(a_actor, "ButtUnderFold", 0.0f));
        // shrink ass slightly
        AddSliderToSet(set, Slider{"AppleCheeks", -0.05f});
        AddSliderToSet(set, Slider{"Butt", -0.05f});

        // Torso
        // remove definition on clavical bone
        AddSliderToSet(set, DeriveSlider(a_actor, "Clavicle_v2", 0.0f));
        // Push out navel
        AddSliderToSet(set, DeriveSlider(a_actor, "NavelEven", 1.0f));

        // hip
        // remove defintion on hip bone
        AddSliderToSet(set, DeriveSlider(a_actor, "HipCarved", 0.0f));

        if (setNippleSlidersRefitEnabled) {
            // nipple
            // sublte change to tip shape
            AddSliderToSet(set, DeriveSlider(a_actor, "NippleDip", 0.0f));
            AddSliderToSet(set, DeriveSlider(a_actor, "NippleTip", 0.0f));
            // flatten areola
            AddSliderToSet(set, DeriveSlider(a_actor, "NipplePuffy_v2", 0.0f));
            // shrink areola
            AddSliderToSet(set, DeriveSlider(a_actor, "AreolaSize", -0.3f));
            // flatten nipple
            AddSliderToSet(set, DeriveSlider(a_actor, "NipBGone", 1.0f));
            // AddSliderToSet(set, DeriveSlider(a_actor, "NippleManga", -0.75f));
            //  push nipples together
            AddSliderToSet(set, Slider{"NippleDistance", 0.05f, 0.08f});
            // Lift large breasts up
            AddSliderToSet(set, Slider{"NippleDown", 0.0f, -0.1f});
            // Flatten nipple + areola
            AddSliderToSet(set, DeriveSlider(a_actor, "NipplePerkManga", -0.25f));
            // Flatten nipple
            // AddSliderToSet(set, DeriveSlider(a_actor, "NipplePerkiness", 0.0f));
        }

        return set;
    }

    Slider OBody::DeriveSlider(RE::Actor* a_actor, const char* a_morph, float a_target) {
        return Slider{a_morph, a_target - GetMorph(a_actor, a_morph)};
    }
}  // namespace Body
