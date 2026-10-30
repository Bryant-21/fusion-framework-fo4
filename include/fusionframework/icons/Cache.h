#pragma once
#include "fusionframework/icons/Raster.h"
#include <list>
#include <string>
#include <string_view>
#include <utility>

namespace fusionframework::icons {
class IconCache {
public:
    explicit IconCache(std::size_t capacity = 64) : capacity(capacity) {}

    const Icon* Find(std::string_view key)
    {
        const auto entry = std::find_if(entries.begin(), entries.end(),
            [key](const auto& value) { return value.first == key; });
        if (entry == entries.end()) return nullptr;
        entries.splice(entries.begin(), entries, entry);
        return &entry->second;
    }

    void Insert(std::string key, Icon icon)
    {
        if (Find(key)) entries.front().second = std::move(icon);
        else {
            entries.emplace_front(std::move(key), std::move(icon));
            if (entries.size() > capacity) entries.pop_back();
        }
    }

private:
    std::size_t capacity;
    std::list<std::pair<std::string, Icon>> entries;
};
}
