#pragma once
// Consumer side of the shared Fusion DLLs. FusionFramework.dll and FusionUI.dll sit in Data/F4SE/Plugins and
// every plugin delay-loads them, so a missing or old DLL is reported to the player instead of failing the
// plugin's own load. fusionframework_use() compiles the implementation into the consumer.
#include "fusionframework/Version.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

// How every Fusion DLL's `<Name>_Version` export packs its semantic version.
#define FUSION_PACK_VERSION(major, minor, patch) \
    ((static_cast<std::uint32_t>(major) << 20) | (static_cast<std::uint32_t>(minor) << 10) | static_cast<std::uint32_t>(patch))

namespace fusionframework::require {
struct Dll {
    const char* file;           // loaded from the plugin's own folder
    const char* versionExport;  // extern "C" std::uint32_t <export>(void)
    const char* download;       // the release the player installs, named in the notice
    std::uint32_t version;      // the version this plugin was built against
};

inline constexpr Dll kFramework{"FusionFramework.dll", "FusionFramework_Version", "Fusion Framework",
    FUSION_PACK_VERSION(FUSIONFRAMEWORK_VERSION_MAJOR, FUSIONFRAMEWORK_VERSION_MINOR, FUSIONFRAMEWORK_VERSION_PATCH)};

// Loads each DLL in order and checks it has the built-against major version and at least its minor.patch.
// Empty when all pass; otherwise one player-facing line naming what to install or update, logged and shown
// once on the HUD at the first kPostLoadGame / kNewGame (Check registers that F4SE listener itself, so call it
// after F4SE::Init). Call it first in F4SEPlugin_Load and, when it fails, call nothing in these DLLs: a call
// into a missing one crashes.
std::string Check(std::string_view plugin, std::initializer_list<Dll> dlls);
}
