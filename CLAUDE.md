# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OBody-NG is an SKSE (Skyrim Script Extender) plugin for Skyrim Special Edition that dynamically applies BodySlide presets to NPCs at runtime. It integrates with RaceMenu's BodyMorph system (SKEE) to apply body morphs based on configurable preset distributions.

## Build System

This project uses **xmake** as its primary build system. The build requires:
- xmake 2.8.2+
- MSVC compiler with C++23 support
- vcpkg for some dependencies

### Build Commands

```powershell
# Full build with dependency updates (local development)
./build.ps1

# Quick rebuild
xmake -y OBody

# Update dependencies (when needed)
xmake repo --update
xmake require --upgrade

# Generate CMakeLists.txt for IDE support (CLion, etc.)
xmake project -k cmakelists
```

### Build Output

Built DLL and PDB files are copied to:
- `contrib/Distribution/data/skse/plugins/`
- If `MO2_MODS_DIR` env var is set: `$MO2_MODS_DIR/OBody - releasedbg/skse/plugins/`

## Architecture

### Core Components

**Body::OBody** (`include/Body/Body.h`, `src/Body/Body.cpp`)
- Singleton managing all body morphing operations
- Interfaces with SKEE's `IBodyMorphInterface` for applying RaceMenu morphs
- Handles actor generation, preset application, and ORefit (clothing overlay) functionality
- Manages plugin API readiness state and event dispatching

**PresetManager** (`include/PresetManager/PresetManager.h`)
- Parses BodySlide XML preset files into `Preset` and `Slider` structures
- Maintains separate preset sets for male/female actors, with blacklist support
- Uses sparse preset indexing (20-bit indexes) for memory-efficient cosave storage

**Parser::JSONParser** (`include/JSONParser/JSONParser.h`)
- Parses `OBody_presetDistributionConfig.json` for NPC-specific preset assignments
- Handles blacklists (characters, outfits), faction-based presets, and race-based presets
- Validates config against JSON schema at startup

**ActorTracker::Registry** (`include/ActorTracker/ActorTracker.h`)
- Thread-safe concurrent flat map tracking per-actor state (preset assignments)
- Uses `boost::concurrent_flat_map` for lock-free actor state lookups
- Bit-packed `ActorState` struct (4 bytes) stores preset index and event flags

**SaveFileState** (`include/SaveFileState/SaveFileState.h`)
- SKSE cosave serialization for persistent preset assignments
- Buffered I/O (64KB) for efficient save/load operations
- Stores actor registry and preset name→index mappings

### Plugin API

**OBody::API** (`include/API/API.h`)
- Versioned C++ API for other SKSE plugins to interact with OBody
- Uses SKSE messaging (`RequestPluginInterface` message type `0xc0B0D9cc`)
- Event-driven architecture with `IOBodyReadinessEventListener` and `IActorChangeEventListener`
- ABI-stable virtual interfaces with version negotiation

### Papyrus Integration

**PapyrusBody** (`include/Papyrus/PapyrusBody.h`)
- Exposes OBody functionality to Papyrus scripts via `OBodyNative` script
- Functions: `GenActor`, `ApplyPresetByName`, `SetORefit`, `GetAllPossiblePresets`, etc.
- Papyrus event registration: `OnActorGenerated`, `OnActorNaked`, `OnActorRemovingClothes`

### SKSE Lifecycle

1. **Plugin Load**: Validate JSON config against schema, bind Papyrus functions
2. **kPostPostLoad**: Acquire SKEE BodyMorph interface, register serialization callbacks, set up plugin API message handler
3. **kDataLoaded**: Parse JSON categories, generate presets from BodySlide XMLs
4. **kNewGame/kPostLoadGame**: Assign preset indexes, register event handlers, signal plugin API readiness

## Dependencies

- **CommonLibSSE-NG** (git submodule at `lib/commonlibsse-ng`) - SKSE plugin framework
- **rapidjson** - JSON parsing with schema validation
- **pugixml** - BodySlide XML preset parsing
- **ryml** (vcpkg) - YAML parsing
- **boost-algorithm** (vcpkg) - String algorithms

## Code Style

- Google-based style with 120 column limit, 4-space indentation (see `.clang-format`)
- Singletons use `GetInstance()` pattern with static `instance_` member
- Headers use `#pragma once`
- Precompiled header: `include/PCH.h`

## Configuration Files

- `Data/SKSE/Plugins/OBody_presetDistributionConfig.json` - Runtime NPC preset configuration
- `Data/SKSE/Plugins/OBody_presetDistributionConfig_schema.json` - JSON schema for validation
- JSON schema examples in `json schema/` directory
