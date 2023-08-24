#pragma once

#include "Body/Body.h"
#include "JSONParser/JSONParser.h"

namespace Event {
    class InitScriptEventHandler : public RE::BSTEventSink<RE::TESInitScriptEvent> {
    public:
        static InitScriptEventHandler* GetSingleton() {
            static InitScriptEventHandler singleton;
            return &singleton;
        }

        virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESInitScriptEvent* a_event,
                                                      RE::BSTEventSource<RE::TESInitScriptEvent>*) {
            if (!a_event || !a_event->objectInitialized->Is3DLoaded()) return RE::BSEventNotifyControl::kContinue;

            RE::Actor* actor = a_event->objectInitialized->As<RE::Actor>();

            if (actor && actor->HasKeywordString("ActorTypeNPC") && !actor->IsChild()) {
                obody->GenerateActorBody(actor);
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        InitScriptEventHandler() = default;
        InitScriptEventHandler(const InitScriptEventHandler&) = delete;
        InitScriptEventHandler(InitScriptEventHandler&&) = delete;
        virtual ~InitScriptEventHandler() = default;

        Body::OBody* obody = Body::OBody::GetInstance();

        InitScriptEventHandler& operator=(const InitScriptEventHandler&) = delete;
        InitScriptEventHandler& operator=(InitScriptEventHandler&&) = delete;
    };

    class LoadGameEventHandler : public RE::BSTEventSink<RE::TESLoadGameEvent> {
    public:
        static LoadGameEventHandler* GetSingleton() {
            static LoadGameEventHandler singleton;
            return &singleton;
        }

        virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent* a_event,
                                                      RE::BSTEventSource<RE::TESLoadGameEvent>*) {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            auto parser = Parser::JSONParser::GetInstance();

            if (!parser->presetDistributionConfigValid) {
                RE::DebugMessageBox(
                    "The OBody NG JSON configuration file contains errors! OBody NG will not work properly. Please "
                    "exit the game now and refer to the OBody NG JSON Configuration Guide to validate your "
                    "configuration file.");
            }

            if (!parser->bodyslidePresetsParsingValid) {
                RE::DebugMessageBox(
                    "A critical error has occurred while parsing the Bodyslide presets files. This most likely means "
                    "you have a corrupt bodyslide preset, or a bodyslide preset where the name contains "
                    "special/incompatible characters. As a result, the presets list in the OBody menu will be empty. "
                    "Please exit the game now and refer to the OBody NG mod page for more information.");
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        LoadGameEventHandler() = default;
        LoadGameEventHandler(const LoadGameEventHandler&) = delete;
        LoadGameEventHandler(LoadGameEventHandler&&) = delete;
        virtual ~LoadGameEventHandler() = default;

        LoadGameEventHandler& operator=(const LoadGameEventHandler&) = delete;
        LoadGameEventHandler& operator=(LoadGameEventHandler&&) = delete;
    };

    class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent> {
    public:
        static EquipEventHandler* GetSingleton() {
            static EquipEventHandler singleton;
            return &singleton;
        }

        virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event,
                                                      RE::BSTEventSource<RE::TESEquipEvent>*) {
            auto actor = a_event->actor->As<RE::Actor>();

            if (!a_event || !actor) return RE::BSEventNotifyControl::kContinue;

            auto form = RE::TESForm::LookupByID(a_event->baseObject);

            if (!form) return RE::BSEventNotifyControl::kContinue;

            if (form->Is(RE::FormType::Armor) || form->Is(RE::FormType::Armature)) {
                if (actor->HasKeywordString("ActorTypeNPC") && !actor->IsChild()) {
                    obody->ProcessActorEquipEvent(actor, !a_event->equipped, form);
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        EquipEventHandler() = default;
        EquipEventHandler(const EquipEventHandler&) = delete;
        EquipEventHandler(EquipEventHandler&&) = delete;
        virtual ~EquipEventHandler() = default;

        Body::OBody* obody = Body::OBody::GetInstance();

        EquipEventHandler& operator=(const EquipEventHandler&) = delete;
        EquipEventHandler& operator=(EquipEventHandler&&) = delete;
    };

    void Register() {
        auto events = RE::ScriptEventSourceHolder::GetSingleton();
        if (events) {
            events->AddEventSink(EquipEventHandler::GetSingleton());
            events->AddEventSink(LoadGameEventHandler::GetSingleton());
            events->AddEventSink(InitScriptEventHandler::GetSingleton());
        }
    }
}  // namespace Event
