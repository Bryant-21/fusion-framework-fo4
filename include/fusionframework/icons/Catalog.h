#pragma once
// Item-icon catalogs, format "fusion-item-icons" v2: FusionHUD's shipped FO4/DLC catalog, BACUP's FO76 catalog and
// mod authors' catalogs (Data/F4SE/Plugins/FusionHUD/icons/*.json). Each icon is its 128 x 128 alpha bytes,
// zlib-compressed and base64-encoded; forms are keyed by FormKey (plugin file, 6-digit hex ObjectID), never by a
// runtime FormID. Consumers link zlib and nlohmann_json.
#include "fusionframework/icons/Raster.h"
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fusionframework::icons {
inline constexpr std::string_view kCatalogFormat = "fusion-item-icons";
inline constexpr int kCatalogVersion = 2;
inline constexpr std::size_t kCatalogMaxBytes = 64u << 20;

struct CatalogForm {
    std::string plugin;
    std::uint32_t objectID;
    std::size_t icon;
    bool fallback;  // fallback_forms: weapons the runtime renderer draws with their attachments
};
struct CatalogGlyph {
    std::string plugin;      // a form's plugin, or the keyword's
    std::uint32_t objectID;  // the form, or the keyword
    std::string glyph;       // icon-pack glyph name
    bool keyword;
};
struct Catalog {
    std::vector<std::string> icons;  // encoded alpha; decode on first use
    std::vector<CatalogForm> forms;
    std::vector<CatalogGlyph> glyphs;
};

inline std::vector<std::uint8_t> Base64(std::string_view text)
{
    std::vector<std::uint8_t> bytes;
    std::uint32_t buffer{};
    int bits{};
    for (const char c : text) {
        int value;
        if (c >= 'A' && c <= 'Z') value = c - 'A';
        else if (c >= 'a' && c <= 'z') value = c - 'a' + 26;
        else if (c >= '0' && c <= '9') value = c - '0' + 52;
        else if (c == '+') value = 62;
        else if (c == '/') value = 63;
        else if (c == '=') break;
        else return {};
        buffer = (buffer << 6 | static_cast<std::uint32_t>(value)) & 0xFFFF;
        if ((bits += 6) >= 8) bytes.push_back(static_cast<std::uint8_t>(buffer >> (bits -= 8)));
    }
    return bytes;
}

// Rows of equal alpha, as the runtime raster delivers. Empty when damaged.
inline Icon DecodeIcon(std::string_view text)
{
    const auto packed = Base64(text);
    std::vector<std::uint8_t> alpha(static_cast<std::size_t>(kArmorIconSize) * kArmorIconSize);
    auto size = static_cast<uLongf>(alpha.size());
    if (packed.empty() || uncompress(alpha.data(), &size, packed.data(), static_cast<uLong>(packed.size())) != Z_OK ||
        size != alpha.size()) return {};
    Icon icon;
    for (int y = 0; y < kArmorIconSize; ++y) {
        const auto* row = alpha.data() + static_cast<std::size_t>(y) * kArmorIconSize;
        for (int x = 0, end; x < kArmorIconSize; x = end) {
            for (end = x + 1; end < kArmorIconSize && row[end] == row[x]; ++end) {}
            if (row[x]) icon.push_back({x, y, end - x, row[x]});
        }
    }
    return icon;
}

// A plugin file name as a catalog may name it: no path parts, no "..", an .esm / .esp / .esl extension.
inline bool PluginName(std::string_view name)
{
    if (name.empty() || name.size() > 260 || name.find("..") != std::string_view::npos ||
        name.find_first_of("/\\:") != std::string_view::npos) return false;
    if (name.size() < 5) return false;
    std::string extension(name.substr(name.size() - 4));
    for (auto& c : extension) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return extension == ".esm" || extension == ".esp" || extension == ".esl";
}

// "000801": up to six hex digits, the ObjectID without its load-order byte.
inline std::uint32_t ObjectID(std::string_view text)
{
    if (text.empty() || text.size() > 6) throw std::runtime_error("bad object id: " + std::string(text));
    std::uint32_t value{};
    for (const char c : text) {
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (digit < 0) throw std::runtime_error("bad object id: " + std::string(text));
        value = value << 4 | static_cast<std::uint32_t>(digit);
    }
    return value;
}

// "000805:MyMod.esp"
inline std::pair<std::string, std::uint32_t> FormKey(std::string_view text)
{
    const auto colon = text.find(':');
    if (colon == std::string_view::npos || !PluginName(text.substr(colon + 1)))
        throw std::runtime_error("bad form key: " + std::string(text));
    return {std::string(text.substr(colon + 1)), ObjectID(text.substr(0, colon))};
}

// Throws on anything outside the format, so one bad file is skipped whole.
inline Catalog ParseCatalog(const nlohmann::json& file)
{
    if (!file.is_object() || file.value("format", "") != kCatalogFormat || file.value("version", 0) != kCatalogVersion ||
        file.value("width", 0) != kArmorIconSize || file.value("height", 0) != kArmorIconSize ||
        file.value("encoding", "") != "zlib-alpha8") throw std::runtime_error("not a fusion-item-icons v2 catalog");
    Catalog result;
    for (const auto& icon : file.at("icons")) {
        const auto& alpha = icon.at("alpha");
        if (!alpha.is_string() || alpha.get_ref<const std::string&>().size() > 128 * 128 * 2)
            throw std::runtime_error("bad icon");
        result.icons.push_back(alpha.get<std::string>());
    }
    for (const auto& [section, fallback] : {std::pair{"forms", false}, std::pair{"fallback_forms", true}}) {
        if (!file.contains(section)) continue;
        for (const auto& [plugin, ids] : file.at(section).items()) {
            if (!PluginName(plugin)) throw std::runtime_error("bad plugin name: " + plugin);
            for (const auto& [id, index] : ids.items()) {
                if (!index.is_number_integer() || index.get<std::int64_t>() < 0 || index.get<std::size_t>() >= result.icons.size())
                    throw std::runtime_error("icon index out of range");
                result.forms.push_back({plugin, ObjectID(id), index.get<std::size_t>(), fallback});
            }
        }
    }
    if (const auto glyphs = file.find("glyphs"); glyphs != file.end()) {
        if (const auto forms = glyphs->find("forms"); forms != glyphs->end())
            for (const auto& [plugin, ids] : forms->items()) {
                if (!PluginName(plugin)) throw std::runtime_error("bad plugin name: " + plugin);
                for (const auto& [id, glyph] : ids.items()) result.glyphs.push_back({plugin, ObjectID(id), glyph.get<std::string>(), false});
            }
        if (const auto keywords = glyphs->find("keywords"); keywords != glyphs->end())
            for (const auto& [key, glyph] : keywords->items()) {
                auto [plugin, id] = FormKey(key);
                result.glyphs.push_back({std::move(plugin), id, glyph.get<std::string>(), true});
            }
    }
    return result;
}
}
