#pragma once
// Inline, like Paths.h: the path resolves with the plugin's own F4SE state.
#include "fusionframework/Config.h"
#include "fusionframework/Paths.h"

#include <string>
#include <string_view>

namespace fusionframework::config {
// Data/F4SE/Plugins/<plugin>.toml, the read-only settings a plugin ships beside its DLL.
inline std::filesystem::path PluginTomlPath(std::string_view plugin)
{
    return paths::GameRoot() / L"Data" / L"F4SE" / L"Plugins" / (std::string{plugin} + ".toml");
}

// Parses PluginTomlPath(plugin); an empty document (every Get* returns its fallback) when it is missing or
// malformed, logged either way.
inline Document LoadPluginToml(std::string_view plugin)
{
    const auto path = PluginTomlPath(plugin);
    const auto text = ReadFile(path);
    if (!text) {
        spdlog::warn("{} missing; using defaults", path.string());
        return {};
    }
    std::string error;
    auto document = Document::TryParse(*text, &error, path.filename().string());
    if (!error.empty()) spdlog::error("{}; using defaults", error);
    return document;
}
}
