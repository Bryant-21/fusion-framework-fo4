#pragma once
#include <toml.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fusionframework::config {
using Value = toml::ordered_value;

// One TOML config file. Tables are addressed by dotted path ("AdvancedLighting.Lamps"); keys are
// case-sensitive. Comments above each key survive edits; blank lines and free-floating comments do not,
// so shipped files keep every comment directly above its key. No engine or Windows includes -- built into
// both the plugin DLL and the Windows-free doctest targets.
class Document {
public:
    // Throws std::runtime_error with toml11's located message on malformed text.
    static Document Parse(std::string_view text, std::string_view sourceName = "config");
    // Parse() that returns an empty document and fills `error` instead of throwing.
    static Document TryParse(std::string_view text, std::string* error = nullptr, std::string_view sourceName = "config");
    std::string Serialize() const;

    // Overlays `other` on top of this document key by key (tables merge, everything else replaces).
    void Overlay(const Document& other);

    const Value* Find(std::string_view table, std::string_view key) const;
    // nullptr when the table is absent or not a table.
    const Value* Table(std::string_view table) const;
    bool Has(std::string_view table, std::string_view key) const { return Find(table, key) != nullptr; }

    // Fallback when the key is absent or holds the wrong type. Ints accept only integers; floats accept
    // integers or floats; nothing is parsed from strings.
    std::string GetString(std::string_view table, std::string_view key, std::string fallback) const;
    bool GetBool(std::string_view table, std::string_view key, bool fallback) const;
    std::int64_t GetInt(std::string_view table, std::string_view key, std::int64_t fallback) const;
    double GetFloat(std::string_view table, std::string_view key, double fallback) const;
    std::optional<std::string> TryGetString(std::string_view table, std::string_view key) const;
    std::optional<bool> TryGetBool(std::string_view table, std::string_view key) const;
    std::optional<std::int64_t> TryGetInt(std::string_view table, std::string_view key) const;
    std::optional<double> TryGetFloat(std::string_view table, std::string_view key) const;

    // Creates missing tables. Keeps the key's comments and, for integers, its base (hex stays hex).
    // Floats are written with the shortest text that reads back exactly (a float overload keeps 0.1f "0.1").
    void Set(std::string_view table, std::string_view key, bool value);
    void Set(std::string_view table, std::string_view key, std::int64_t value);
    void Set(std::string_view table, std::string_view key, int value) { Set(table, key, std::int64_t{ value }); }
    void Set(std::string_view table, std::string_view key, double value);
    void Set(std::string_view table, std::string_view key, float value);
    void Set(std::string_view table, std::string_view key, std::string value);
    void Set(std::string_view table, std::string_view key, const char* value) { Set(table, key, std::string(value)); }
    bool Remove(std::string_view table, std::string_view key);

    Value& Root() { return root_; }
    const Value& Root() const { return root_; }

private:
    Value* MutableTable(std::string_view table);
    Value& Slot(std::string_view table, std::string_view key);

    Value root_ = Value(toml::ordered_table{});
};

// Splits a dotted table path; exposed for callers walking Root() themselves.
std::vector<std::string> SplitPath(std::string_view table);

// One plugin's config file. Readers on any thread share a parsed copy for up to a second; writes replace
// the file atomically. The first access writes `shipped` when the file is missing. A file that fails to
// parse is logged and read as `shipped`; Update() refuses to overwrite it.
class Store {
public:
    Store(std::filesystem::path path, std::string_view shipped);

    const std::filesystem::path& Path() const { return path_; }
    std::string_view Shipped() const { return shipped_; }
    Document Load();
    std::string ReadText();
    // Read-modify-write of the whole document, serialised and replaced atomically.
    bool Update(const std::function<void(Document&)>& edit);

private:
    void EnsureCurrent();

    std::filesystem::path path_;
    std::string_view shipped_;

    std::once_flag ensured_;
    std::mutex fileMutex_;
    std::mutex cacheMutex_;
    std::optional<Document> cached_;
    std::chrono::steady_clock::time_point cachedAt_;
    std::uint64_t cacheGeneration_ = 0;
};

// Reads and atomically replaces a file on disk (temp file, then one rename; parent folders are created).
// ReadFile returns nullopt when missing.
std::optional<std::string> ReadFile(const std::filesystem::path& path);
bool WriteFile(const std::filesystem::path& path, std::string_view text);

inline bool WriteFile(const std::filesystem::path& path, std::span<const std::uint8_t> bytes)
{
    return WriteFile(path, std::string_view{ reinterpret_cast<const char*>(bytes.data()), bytes.size() });
}
}
