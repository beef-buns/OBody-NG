#include <stddef.h>

#include "Body/Body.h"
#include "Body/Event.h"
#include "Papyrus/Papyrus.h"
#include "JSONParser/JSONParser.h"
#include "PresetManager/PresetManager.h"
#include "SKEE.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    /**
     * Setup logging.
     *
     * <p>
     * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
     * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
     * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
     * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
     * (or <code>Skyrim VR</code> if you are using VR).
     * </p>
     */
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
        auto obody = Body::OBody::GetInstance();

        switch (a_msg->type) {
            // On kPostPostLoad, we can try to fetch the Racemenu interface
            case SKSE::MessagingInterface::kPostPostLoad: {
                SKEE::InterfaceExchangeMessage msg;
                auto intfc = SKSE::GetMessagingInterface();
                intfc->Dispatch(SKEE::InterfaceExchangeMessage::kExchangeInterface, (void*)&msg,
                                sizeof(SKEE::InterfaceExchangeMessage*), "skee");
                if (!msg.interfaceMap) {
                    logger::critical("Couldn't get interface map!");
                    return;
                }

                auto morphInterface =
                    static_cast<SKEE::IBodyMorphInterface*>(msg.interfaceMap->QueryInterface("BodyMorph"));
                if (!morphInterface) {
                    logger::critical("Couldn't get serialization MorphInterface!");
                    return;
                }

                logger::info("BodyMorph Version {}", morphInterface->GetVersion());
                if (!obody->SetMorphInterface(morphInterface)) logger::info("BodyMorphInterace not provided");

                return;
            }

            // When data is all loaded (this is by the time the Main Menu is visible), we can parse the JSON and the
            // Bodyslide presets
            case SKSE::MessagingInterface::kDataLoaded: {
                auto parser = Parser::JSONParser::GetInstance();

                std::ifstream f(L"Data/SKSE/Plugins/OBody_presetDistributionConfig.json");

                try {
                    parser->presetDistributionConfig = nlohmann::ordered_json::parse(f);
                    parser->ProcessJSONCategories();
                    parser->presetDistributionConfigValid = true;
                } catch (const std::runtime_error& re) {
                    log::info("{} ", re.what());
                    parser->presetDistributionConfigValid = false;
                } catch (const std::exception& ex) {
                    log::info("{} ", ex.what());
                    parser->presetDistributionConfigValid = false;
                } catch (...) {
                    log::info("An unknown error has occurred while parsing the JSON file.");
                    parser->presetDistributionConfigValid = false;
                }

                try {
                    PresetManager::GeneratePresets();
                    parser->bodyslidePresetsParsingValid = true;
                } catch (const std::runtime_error& re) {
                    log::info("{} ", re.what());
                    parser->bodyslidePresetsParsingValid = false;
                } catch (const std::exception& ex) {
                    log::info("{} ", ex.what());
                    parser->bodyslidePresetsParsingValid = false;
                } catch (...) {
                    log::info("An unknown error has occurred while parsing the bodyslide presets files.");
                    parser->bodyslidePresetsParsingValid = false;
                }

                if (parser->presetDistributionConfigValid) {
                    log::info("OBody has finished parsing the JSON config file.");
                } else {
                    log::info("There are errors in the OBody JSON config file! OBody will not work properly.");
                }

                return;
            }

            // We can only register for events after the game is loaded
            // The game doesn't send a Load game event on new game, so we need to listen for this one in specific
            case SKSE::MessagingInterface::kNewGame: {
                log::info("New Game started");
                Event::Register();
                return;
            }

            case SKSE::MessagingInterface::kPostLoadGame: {
                log::info("Game finished loading");
                Event::Register();
                return;
            }
        }
    }
}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);

    Init(a_skse);

    auto message = SKSE::GetMessagingInterface();
    if (!message->RegisterListener(MessageHandler)) return false;

    Papyrus::Bind();

    log::info("{} has finished loading.", plugin->GetName());

    return true;
}
