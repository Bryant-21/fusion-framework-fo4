#pragma once
// Inline, so each plugin resolves them with its own F4SE state (usable before FusionFramework.dll is checked).
// REX::W32 rather than <Windows.h>: this header reaches plugin code that must not see Windows macros.
#include <F4SE/F4SE.h>
#include <REX/W32/KERNEL32.h>
#include <REX/W32/OLE32.h>
#include <REX/W32/SHELL32.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace fusionframework::paths {
// The directory containing the running Fallout4.exe, independent of the process working
// directory: mod managers and alternate launchers don't reliably start the game in its root.
// Falls back to the working directory, then to an empty path, logging as it degrades.
inline std::filesystem::path GameRoot()
{
    namespace fs = std::filesystem;

    std::array<wchar_t, 4096> buffer{};
    const auto length = REX::W32::GetModuleFileNameW(nullptr, buffer.data(), static_cast<std::uint32_t>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        spdlog::warn("could not resolve game executable path; falling back to current working directory");
        std::error_code ec;
        auto cwd = fs::current_path(ec);
        if (ec) {
            spdlog::error("could not resolve current working directory either; game root is unknown");
            return fs::path{};
        }
        return cwd;
    }
    return fs::path(buffer.data(), buffer.data() + length).parent_path();
}

// Documents/My Games/<F4SE save folder, else Fallout4>/F4SE: where F4SE and its plugins write their logs.
inline std::filesystem::path LogDir()
{
    wchar_t* raw = nullptr;
    const auto hr = REX::W32::SHGetKnownFolderPath(REX::W32::FOLDERID_Documents, REX::W32::KF_FLAG_DEFAULT, nullptr, &raw);
    std::filesystem::path docs;
    if (hr >= 0 && raw) {
        docs = raw;
    }
    if (raw) {
        REX::W32::CoTaskMemFree(raw);
    }
    if (docs.empty()) {
        docs = std::filesystem::current_path();
    }

    // Empty until F4SE::Init has run (and on OG F4SE before 0.7.1).
    const auto saveFolder = F4SE::GetSaveFolderName();
    auto dir = docs / L"My Games" / std::filesystem::path(saveFolder.empty() ? "Fallout4" : saveFolder) / L"F4SE";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

// The plugin's own folder beside the logs (settings, caches, captures), created on first use.
inline std::filesystem::path DataDir(std::string_view plugin)
{
    auto dir = LogDir() / std::filesystem::path(plugin);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}
}
