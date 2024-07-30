#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Note: add new modules here before using
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <random>
#include <boost/algorithm/string.hpp>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace WinAPI = REX::W32;
namespace logger = SKSE::log;
namespace fs = std::filesystem;

using namespace std::literals;

using json = nlohmann::json;

#define DLLEXPORT __declspec(dllexport)
#include "Plugin.h"
