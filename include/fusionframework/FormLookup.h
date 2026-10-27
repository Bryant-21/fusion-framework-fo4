#pragma once
#include <cstring>
#include <string_view>
#include <vector>

namespace fusionframework::forms {
// Resolve plugins from the compiled load order, never TESDataHandler::files. The engine rebuilds
// that list on another thread just after kGameDataReady, and CommonLib's LookupModByName walked
// into a null TESFile there (CTD in ucrtbase strlen during the condition catalog bind).
inline const RE::TESFile* LoadedFile(std::string_view name)
{
    const auto matches = [&](const RE::TESFile* file) {
        return file && name.size() == std::strlen(file->filename) &&
               _strnicmp(file->filename, name.data(), name.size()) == 0;
    };
    // Item-list hooks resolve forms per item per frame; scanning the whole load order each time
    // made big inventories hitch. The load order is fixed for the session, so found files are kept.
    thread_local std::vector<const RE::TESFile*> found;
    for (const auto* file : found)
        if (matches(file)) return file;
    auto* data = RE::TESDataHandler::GetSingleton();
    if (!data) return nullptr;
    for (const auto* list : { &data->compiledFileCollection.files, &data->compiledFileCollection.smallFiles })
        for (const auto* file : *list)
            if (matches(file)) return found.push_back(file), file;
    return nullptr;
}

inline RE::TESFormID LookupFormID(RE::TESFormID rawFormID, std::string_view plugin)
{
    const auto* file = LoadedFile(plugin);
    if (!file || file->compileIndex == 0xFF) return 0;
    if (file->compileIndex == 0xFE)
        return 0xFE000000u | (static_cast<RE::TESFormID>(file->smallFileCompileIndex) << 12) | (rawFormID & 0xFFF);
    return (static_cast<RE::TESFormID>(file->compileIndex) << 24) | (rawFormID & 0xFFFFFF);
}

inline RE::TESForm* LookupForm(RE::TESFormID rawFormID, std::string_view plugin)
{
    const auto id = LookupFormID(rawFormID, plugin);
    return id ? RE::TESForm::GetFormByID(id) : nullptr;
}

template <class T>
T* LookupForm(RE::TESFormID rawFormID, std::string_view plugin)
{
    auto* form = LookupForm(rawFormID, plugin);
    if (!form) return nullptr;
    // Abstract bases (TESForm, TESBoundObject, MagicItem, ...) have FORM_ID kNONE, which no loaded
    // form's type matches; As<T>() dispatches on the concrete type and accepts any derived form.
    if constexpr (T::FORM_ID == RE::ENUM_FORM_ID::kNONE) return form->As<T>();
    else return form->Is(T::FORM_ID) ? form->As<T>() : nullptr;
}
}
