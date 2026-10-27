#include "fusionframework/DllLog.h"
#include "fusionframework/Game.h"
#include "fusionframework/Require.h"

extern "C" F4SE::PluginVersionData F4SEPlugin_Version;

namespace {
const bool earlyLog = fusionframework::dll_log::Start("FusionFramework");
}

extern "C" __declspec(dllexport) std::uint32_t FusionFramework_Version()
{
    return fusionframework::require::kFramework.version;
}

// The pre-NG F4SE on 1.10.163 reads only this classic export; NG/AE F4SE read F4SEPlugin_Version.
F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
    if (!a_f4se || !a_info) return false;
    if (a_f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    a_info->infoVersion = F4SE::PluginInfo::kVersion;
    a_info->name = "FusionFramework";
    a_info->version = F4SEPlugin_Version.pluginVersion;
    return true;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
    F4SE::InitInfo info{};
    info.log = false;
    F4SE::Init(a_f4se, info);
    fusionframework::dll_log::Open("FusionFramework");
    spdlog::info("FusionFramework {}.{}.{} loaded", FUSIONFRAMEWORK_VERSION_MAJOR, FUSIONFRAMEWORK_VERSION_MINOR,
                 FUSIONFRAMEWORK_VERSION_PATCH);
    return true;
}
