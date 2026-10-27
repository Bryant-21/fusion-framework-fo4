#include "fusionframework/Config.h"
#include <spdlog/spdlog.h>

#include <Windows.h>

#include <fstream>
#include <sstream>
#include <system_error>

namespace fusionframework::config {
std::optional<std::string> ReadFile(const std::filesystem::path& path)
{
    try {
        if (!std::filesystem::exists(path)) return std::nullopt;
        std::ifstream in(path, std::ios::binary);
        std::ostringstream buf;
        buf << in.rdbuf();
        return buf.str();
    } catch (const std::exception& e) {
        spdlog::error("config: failed to read {} ({}); using defaults", path.string(), e.what());
        return std::nullopt;
    }
}

// Other threads parse the file at any moment, so it is replaced in one step rather than rewritten.
bool WriteFile(const std::filesystem::path& path, std::string_view text)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    auto temp = path;
    temp += L".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!out) {
            spdlog::error("config: could not write {}", temp.string());
            return false;
        }
    }
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        spdlog::error("config: could not replace {} (error {})", path.string(), GetLastError());
        std::filesystem::remove(temp, error);
        return false;
    }
    return true;
}

Store::Store(std::filesystem::path path, std::string_view shipped) :
    path_(std::move(path)), shipped_(shipped)
{}

void Store::EnsureCurrent()
{
    std::call_once(ensured_, [this] {
        std::lock_guard lock(fileMutex_);
        if (!std::filesystem::exists(path_) && WriteFile(path_, shipped_))
            spdlog::info("config: wrote default settings to {}", path_.string());
    });
}

std::string Store::ReadText()
{
    EnsureCurrent();
    std::lock_guard lock(fileMutex_);
    return ReadFile(path_).value_or(std::string{ shipped_ });
}

Document Store::Load()
{
    const auto now = std::chrono::steady_clock::now();
    std::uint64_t generation;
    {
        std::lock_guard lock(cacheMutex_);
        if (cached_ && now - cachedAt_ < std::chrono::seconds(1)) return *cached_;
        generation = cacheGeneration_;
    }
    std::string error;
    auto doc = Document::TryParse(ReadText(), &error, path_.filename().string());
    if (!error.empty()) {
        spdlog::error("config: {} is not valid TOML; using defaults\n{}", path_.string(), error);
        doc = Document::Parse(shipped_, "shipped defaults");
    }
    std::lock_guard lock(cacheMutex_);
    // An Update() since the read means `doc` may be stale; don't cache it.
    if (generation == cacheGeneration_) {
        cached_ = doc;
        cachedAt_ = now;
    }
    return doc;
}

bool Store::Update(const std::function<void(Document&)>& edit)
{
    EnsureCurrent();
    std::lock_guard lock(fileMutex_);
    std::string error;
    auto doc = Document::TryParse(ReadFile(path_).value_or(std::string{ shipped_ }), &error, path_.filename().string());
    if (!error.empty()) {
        spdlog::error("config: not saving over {}; fix the TOML error first\n{}", path_.string(), error);
        return false;
    }
    edit(doc);
    const bool written = WriteFile(path_, doc.Serialize());
    std::lock_guard cacheLock(cacheMutex_);
    cached_.reset();
    ++cacheGeneration_;
    return written;
}
}
