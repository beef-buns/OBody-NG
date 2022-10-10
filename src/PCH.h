#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Note: add new modules here before using
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <pugixml.hpp>
#include <random>

namespace WinAPI = SKSE::WinAPI;
namespace logger = SKSE::log;
namespace fs = std::filesystem;

using namespace std::literals;

#define DLLEXPORT __declspec(dllexport)
