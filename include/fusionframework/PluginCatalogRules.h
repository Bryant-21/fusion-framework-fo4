#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Per-plugin JSON catalog contract (B21_TalesFromAppalachia/docs/2026-10-12-plugin-catalogs-plan.md, sections 1-2).
// Pure logic: no RE:: types. Mirrored by creation_lib/catalog/plugin_catalog.py; both run
// creation_lib/resources/catalog_schemas/plugin_catalog_cases.json.
namespace fusionframework::catalog {

struct FormKey {
    std::string plugin;  // lowercased
    std::uint32_t id{};
    bool operator==(const FormKey&) const = default;
};

inline std::string LowerAscii(std::string_view text)
{
    std::string out(text);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// "Plugin.ext:HEX" only. Hex is 1-8 digits, either case, no 0x, non-zero.
inline std::optional<FormKey> ParseFormKey(std::string_view text)
{
    const auto colon = text.find(':');
    if (colon == std::string_view::npos || text.find(':', colon + 1) != std::string_view::npos) return std::nullopt;
    const auto plugin = text.substr(0, colon);
    const auto hex = text.substr(colon + 1);
    if (plugin.empty() || hex.empty() || hex.size() > 8) return std::nullopt;
    const auto dot = plugin.rfind('.');
    if (dot == std::string_view::npos || dot == 0 || dot + 1 == plugin.size()) return std::nullopt;
    for (const char c : plugin)
        if (std::isspace(static_cast<unsigned char>(c))) return std::nullopt;
    std::uint32_t id = 0;
    for (const char c : hex) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
        id = (id << 4) | static_cast<std::uint32_t>(c <= '9' ? c - '0' : (std::tolower(static_cast<unsigned char>(c)) - 'a' + 10));
    }
    if (id == 0) return std::nullopt;
    return FormKey{LowerAscii(plugin), id};
}

// Names of the ".json" files whose plugin is active, in load order. Returned names are the
// originals; the plugin is the name without its ".json" suffix.
inline std::vector<std::string> OrderFiles(const std::vector<std::string>& fileNames,
                                           const std::vector<std::string>& activePluginsInLoadOrder)
{
    std::unordered_map<std::string, std::size_t> loadIndex;
    for (std::size_t i = 0; i < activePluginsInLoadOrder.size(); ++i)
        loadIndex.emplace(LowerAscii(activePluginsInLoadOrder[i]), i);

    std::vector<std::pair<std::size_t, std::string>> kept;
    for (const auto& name : fileNames) {
        constexpr std::string_view suffix = ".json";
        if (name.size() <= suffix.size() || LowerAscii(std::string_view(name).substr(name.size() - suffix.size())) != suffix) continue;
        const auto found = loadIndex.find(LowerAscii(std::string_view(name).substr(0, name.size() - suffix.size())));
        if (found != loadIndex.end()) kept.emplace_back(found->second, name);
    }
    std::stable_sort(kept.begin(), kept.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::string> out;
    out.reserve(kept.size());
    for (auto& entry : kept) out.push_back(std::move(entry.second));
    return out;
}

// Applies one document's section. Array sections: each object entry is keyed by
// keyOf(entry, ""); non-object array items are skipped. Object sections: each value, of any JSON
// type (map sections such as delegates hold plain strings or arrays), is keyed by
// keyOf(value, mapKey). keyOf returns std::optional<Key>; entries it declines are skipped.
// An object entry with "remove": true erases the key; any other entry replaces the whole earlier
// entry. Anything that is not an array or object is ignored.
template <class Key, class KeyOf>
void MergeSection(std::map<Key, nlohmann::json>& into, const nlohmann::json& section, KeyOf&& keyOf)
{
    const auto apply = [&](const nlohmann::json& entry, const std::string& mapKey) {
        const std::optional<Key> key = keyOf(entry, mapKey);
        if (!key) return;
        const auto removeFlag = entry.is_object() ? entry.find("remove") : entry.end();
        if (removeFlag != entry.end() && removeFlag->is_boolean() && removeFlag->get<bool>())
            into.erase(*key);
        else
            into.insert_or_assign(*key, entry);
    };
    if (section.is_array()) {
        for (const auto& entry : section)
            if (entry.is_object()) apply(entry, std::string{});
    } else if (section.is_object()) {
        for (const auto& [mapKey, entry] : section.items()) apply(entry, mapKey);
    }
}

// Per top-level field, later wins. No deep merge. A non-object `from` is ignored.
inline void MergeSettings(nlohmann::json& into, const nlohmann::json& from)
{
    if (!from.is_object()) return;
    if (!into.is_object()) into = nlohmann::json::object();
    for (const auto& [name, value] : from.items()) into[name] = value;
}

}  // namespace fusionframework::catalog
