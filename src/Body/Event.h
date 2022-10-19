#pragma once

#include "Body/Body.h"

namespace Event {
    using EventResult = RE::BSEventNotifyControl;

    class ObjectLoadedEventHandler : public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
    public:
        static ObjectLoadedEventHandler* GetSingleton() {
            static ObjectLoadedEventHandler singleton;
            return &singleton;
        }

        virtual EventResult ProcessEvent(const RE::TESObjectLoadedEvent* a_event,
                                         RE::BSTEventSource<RE::TESObjectLoadedEvent>*) {
            if (!a_event || !a_event->loaded) return EventResult::kContinue;

            auto actor = RE::TESObjectREFR::LookupByID<RE::Actor>(a_event->formID);

            if (actor) {
                if (actor->HasKeywordString("ActorTypeNPC") && !actor->IsChild()) {
                    logger::info("Obj load {} appeared", actor->GetName());
                    auto obody = Body::OBody::GetInstance();
                    obody->ProcessActor(actor);
                }
            }

            return EventResult::kContinue;
        }

    private:
        ObjectLoadedEventHandler() = default;
        ObjectLoadedEventHandler(const ObjectLoadedEventHandler&) = delete;
        ObjectLoadedEventHandler(ObjectLoadedEventHandler&&) = delete;
        virtual ~ObjectLoadedEventHandler() = default;

        ObjectLoadedEventHandler& operator=(const ObjectLoadedEventHandler&) = delete;
        ObjectLoadedEventHandler& operator=(ObjectLoadedEventHandler&&) = delete;
    };

    class InitScriptEventHandler : public RE::BSTEventSink<RE::TESInitScriptEvent> {
    public:
        static InitScriptEventHandler* GetSingleton() {
            static InitScriptEventHandler singleton;
            return &singleton;
        }

        virtual EventResult ProcessEvent(const RE::TESInitScriptEvent* a_event,
                                         RE::BSTEventSource<RE::TESInitScriptEvent>*) {
            if (!a_event || !a_event->objectInitialized->Is3DLoaded()) return EventResult::kContinue;

            RE::Actor* actor = a_event->objectInitialized->As<RE::Actor>();

            if (actor) {
                if (actor->HasKeywordString("ActorTypeNPC") && !actor->IsChild()) {
                    logger::info("Script init {} appeared", actor->GetName());
                    auto obody = Body::OBody::GetInstance();
                    obody->ProcessActor(actor);
                }
            }

            return EventResult::kContinue;
        }

    private:
        InitScriptEventHandler() = default;
        InitScriptEventHandler(const InitScriptEventHandler&) = delete;
        InitScriptEventHandler(InitScriptEventHandler&&) = delete;
        virtual ~InitScriptEventHandler() = default;

        InitScriptEventHandler& operator=(const InitScriptEventHandler&) = delete;
        InitScriptEventHandler& operator=(InitScriptEventHandler&&) = delete;
    };

    class LoadGameEventHandler : public RE::BSTEventSink<RE::TESLoadGameEvent> {
    public:
        static LoadGameEventHandler* GetSingleton() {
            static LoadGameEventHandler singleton;
            return &singleton;
        }

        virtual EventResult ProcessEvent(const RE::TESLoadGameEvent* a_event,
                                         RE::BSTEventSource<RE::TESLoadGameEvent>*) {
            if (!a_event) return EventResult::kContinue;

            logger::info("GameLoaded");

            auto obody = Body::OBody::GetInstance();
            obody->setGameLoaded = true;

            return EventResult::kContinue;
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

        virtual EventResult ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) {
            auto actor = a_event->actor->As<RE::Actor>();
            if (!a_event || !actor) return EventResult::kContinue;

            auto form = RE::TESForm::LookupByID(a_event->baseObject);
            if (!form) return EventResult::kContinue;

            if (form->Is(RE::FormType::Armor) || form->Is(RE::FormType::Armature)) {
                if (actor->HasKeywordString("ActorTypeNPC") && !actor->IsChild()) {
                    bool removingBody = false;

                    if (!a_event->equipped) {
                        logger::info("Not equipped");
                        auto changes = actor->GetInventoryChanges();
                        auto const armor = changes->GetArmorInSlot(32);

                        if (armor) {
                            removingBody = (armor->formID == form->formID);
                        } else {
                            logger::info("armor not found");
                            removingBody = true;
                        }
                    }

                    auto obody = Body::OBody::GetInstance();
                    obody->ProcessActorEquipEvent(actor, removingBody);
                }
            }

            return EventResult::kContinue;
        }

    private:
        EquipEventHandler() = default;
        EquipEventHandler(const EquipEventHandler&) = delete;
        EquipEventHandler(EquipEventHandler&&) = delete;
        virtual ~EquipEventHandler() = default;

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
