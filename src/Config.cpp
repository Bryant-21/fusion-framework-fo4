#include "fusionframework/Config.h"

#include <charconv>
#include <stdexcept>

namespace fusionframework::config {
namespace {
    // toml11 prints a replaced float with the old value's precision (1.25 -> 0.125 became "0.12") and its
    // default format keeps 6 significant digits, so the precision is set from the shortest exact text.
    toml::floating_format_info ShortestFormat(std::string_view text)
    {
        toml::floating_format_info fmt{};
        const auto exp = text.find_first_of("eE");
        const auto mantissa = text.substr(0, exp);
        const auto dot = mantissa.find('.');
        const auto decimals = dot == std::string_view::npos ? 0 : mantissa.size() - dot - 1;
        if (exp == std::string_view::npos) {
            fmt.fmt = toml::floating_format::fixed;
            fmt.prec = decimals == 0 ? 1 : decimals;
        } else {
            fmt.fmt = toml::floating_format::scientific;
            fmt.prec = decimals;
        }
        return fmt;
    }

    template <class F>
    std::string Shortest(F value)
    {
        char buf[64];
        const auto r = std::to_chars(buf, buf + sizeof(buf), value);
        return std::string(buf, r.ptr);
    }

    void Merge(Value& base, const Value& top)
    {
        if (!base.is_table() || !top.is_table()) {
            base = top;
            return;
        }
        auto& table = base.as_table();
        for (const auto& [key, value] : top.as_table()) {
            const auto it = table.find(key);
            if (it == table.end())
                table.emplace_back(key, value);
            else
                Merge(it->second, value);
        }
    }

    const Value* Child(const Value& v, std::string_view key)
    {
        if (!v.is_table()) return nullptr;
        const auto& table = v.as_table();
        const auto it = table.find(std::string(key));
        return it == table.end() ? nullptr : &it->second;
    }

    void Assign(Value& slot, Value value)
    {
        auto comments = slot.comments();
        slot = std::move(value);
        slot.comments() = std::move(comments);
    }
}

std::vector<std::string> SplitPath(std::string_view table)
{
    std::vector<std::string> parts;
    while (!table.empty()) {
        const auto dot = table.find('.');
        parts.emplace_back(table.substr(0, dot));
        if (dot == std::string_view::npos) break;
        table.remove_prefix(dot + 1);
    }
    return parts;
}

Document Document::Parse(std::string_view text, std::string_view sourceName)
{
    std::vector<unsigned char> bytes(text.begin(), text.end());
    auto result = toml::try_parse<toml::ordered_type_config>(bytes, std::string(sourceName));
    if (result.is_err()) {
        std::string message;
        for (const auto& e : result.unwrap_err()) message += toml::format_error(e);
        throw std::runtime_error(message);
    }
    Document doc;
    doc.root_ = std::move(result.unwrap());
    return doc;
}

Document Document::TryParse(std::string_view text, std::string* error, std::string_view sourceName)
{
    try {
        return Parse(text, sourceName);
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return {};
    }
}

std::string Document::Serialize() const
{
    return toml::format(root_);
}

void Document::Overlay(const Document& other)
{
    Merge(root_, other.root_);
}

const Value* Document::Table(std::string_view table) const
{
    const Value* v = &root_;
    for (const auto& part : SplitPath(table))
        if (!(v = Child(*v, part))) return nullptr;
    return v->is_table() ? v : nullptr;
}

const Value* Document::Find(std::string_view table, std::string_view key) const
{
    const auto* t = Table(table);
    return t ? Child(*t, key) : nullptr;
}

std::optional<std::string> Document::TryGetString(std::string_view table, std::string_view key) const
{
    const auto* v = Find(table, key);
    return v && v->is_string() ? std::optional(v->as_string()) : std::nullopt;
}

std::optional<bool> Document::TryGetBool(std::string_view table, std::string_view key) const
{
    const auto* v = Find(table, key);
    return v && v->is_boolean() ? std::optional(v->as_boolean()) : std::nullopt;
}

std::optional<std::int64_t> Document::TryGetInt(std::string_view table, std::string_view key) const
{
    const auto* v = Find(table, key);
    return v && v->is_integer() ? std::optional<std::int64_t>(v->as_integer()) : std::nullopt;
}

std::optional<double> Document::TryGetFloat(std::string_view table, std::string_view key) const
{
    const auto* v = Find(table, key);
    if (v && v->is_floating()) return v->as_floating();
    if (v && v->is_integer()) return static_cast<double>(v->as_integer());
    return std::nullopt;
}

std::string Document::GetString(std::string_view table, std::string_view key, std::string fallback) const
{
    return TryGetString(table, key).value_or(std::move(fallback));
}

bool Document::GetBool(std::string_view table, std::string_view key, bool fallback) const
{
    return TryGetBool(table, key).value_or(fallback);
}

std::int64_t Document::GetInt(std::string_view table, std::string_view key, std::int64_t fallback) const
{
    return TryGetInt(table, key).value_or(fallback);
}

double Document::GetFloat(std::string_view table, std::string_view key, double fallback) const
{
    return TryGetFloat(table, key).value_or(fallback);
}

Value* Document::MutableTable(std::string_view table)
{
    Value* v = &root_;
    for (const auto& part : SplitPath(table)) {
        auto& t = v->as_table();
        if (t.find(part) == t.end()) t.emplace_back(part, Value(toml::ordered_table{}));
        v = &t[part];
        if (!v->is_table()) *v = Value(toml::ordered_table{});
    }
    return v;
}

Value& Document::Slot(std::string_view table, std::string_view key)
{
    auto* t = MutableTable(table);
    // A table only named by a sub-table header ([A.B] with no [A]) is implicit; the serializer throws on its keys.
    if (t != &root_ && t->as_table_fmt().fmt == toml::table_format::implicit)
        t->as_table_fmt().fmt = toml::table_format::multiline;
    return t->as_table()[std::string(key)];
}

void Document::Set(std::string_view table, std::string_view key, bool value)
{
    Assign(Slot(table, key), Value(value));
}

void Document::Set(std::string_view table, std::string_view key, std::int64_t value)
{
    auto& slot = Slot(table, key);
    const auto fmt = slot.is_integer() ? std::optional(slot.as_integer_fmt()) : std::nullopt;
    Assign(slot, Value(value));
    if (fmt) slot.as_integer_fmt() = *fmt;
}

void Document::Set(std::string_view table, std::string_view key, double value)
{
    auto& slot = Slot(table, key);
    Assign(slot, Value(value));
    slot.as_floating_fmt() = ShortestFormat(Shortest(value));
}

void Document::Set(std::string_view table, std::string_view key, float value)
{
    const auto text = Shortest(value);
    auto& slot = Slot(table, key);
    Assign(slot, Value(std::stod(text)));
    slot.as_floating_fmt() = ShortestFormat(text);
}

void Document::Set(std::string_view table, std::string_view key, std::string value)
{
    Assign(Slot(table, key), Value(std::move(value)));
}

bool Document::Remove(std::string_view table, std::string_view key)
{
    if (!Table(table)) return false;
    auto& t = MutableTable(table)->as_table();
    const auto it = t.find(std::string(key));
    if (it == t.end()) return false;
    t.erase(it);
    return true;
}
}
