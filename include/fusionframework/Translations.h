#pragma once
#include "fusionframework/Game.h"
#include "fusionframework/Paths.h"

#include <format>
#include <string>
#include <string_view>

namespace fusionframework::translations {

// Adds Data/F4SE/Plugins/<table>_en.txt (required) and <table>_<sLanguage>.txt (optional) to the
// Scaleform translator. Loose tables, so the file can sit outside any archive. Call at kGameDataReady.
inline void Load(std::string_view table)
{
    auto* manager = RE::BSScaleformManager::GetSingleton();
    auto* translator = manager ? manager->GetTranslator() : nullptr;
    if (!translator) {
        spdlog::error("translations: Scaleform translator unavailable at GameDataReady");
        return;
    }
    const auto load = [&](std::string_view language, bool required) {
        const auto relative = std::format("F4SE/Plugins/{}_{}.txt", table, language);
        const auto path = paths::GameRoot() / "Data" / relative;
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) {
            if (required) spdlog::error("translations: missing loose table {}", path.string());
            return;
        }
        RE::BSResourceStreamParser stream(relative.c_str());
        if (!stream.Begin()) {
            spdlog::error("translations: cannot open loose table {}", relative);
            return;
        }
        RE::BSStreamParser<wchar_t> parser(&stream);
        const auto before = translator->translator.translationMap.size();
        translator->AddTranslations(&parser);
        spdlog::info("translations: loaded {}; table entries {} -> {}", relative,
            before, translator->translator.translationMap.size());
    };
    load("en", true);
    if (const auto* setting = RE::GetINISetting("sLanguage:General")) {
        const std::string language{setting->GetString()};
        if (!language.empty() && _stricmp(language.c_str(), "en") != 0) load(language, false);
    }
    translator->Release();
}

// Movies that set text in fontconfig aliases ($MAIN_Font...) draw a $ key GFx translates inside
// such a field as missing glyphs in game, so movies receive the translated text instead.
inline std::string Translate(std::string_view key)
{
    std::string result{key};
    auto* manager = RE::BSScaleformManager::GetSingleton();
    auto* translator = manager ? manager->GetTranslator() : nullptr;
    if (!translator) return result;
    const std::wstring wideKey(key.begin(), key.end());
    const auto& map = translator->translator.translationMap;
    if (const auto found = map.find(RE::BSFixedStringWCS{wideKey.c_str()}); found != map.end()) {
        const std::wstring_view value{found->second.c_str(), found->second.size()};
        const auto count = ::WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (count > 0) {
            result.assign(static_cast<std::size_t>(count), '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
        }
    }
    translator->Release();
    return result;
}

}
