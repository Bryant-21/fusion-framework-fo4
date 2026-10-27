#pragma once
// Compiled into each consumer (fusionframework_use()); needs RE/Fallout.h from the pch.
#include <cstddef>
#include <functional>

namespace fusionframework::forms {
// ACTOR_BASE_DATA template-use bits (and templateForms slots) for the categories catalogs key on.
enum class TemplateCategory : std::size_t { Traits = 0, Stats = 1, Spells = 3, Inventory = 8 };

struct TemplateKey {
    const RE::TESNPC* npc{};
    const RE::TESRace* race{};
    bool operator==(const TemplateKey&) const = default;
};

struct TemplateKeyHash {
    std::size_t operator()(const TemplateKey& key) const noexcept
    {
        return std::hash<const void*>{}(key.npc) ^ (std::hash<const void*>{}(key.race) << 1);
    }
};

// The NPC that owns `category` for this actor (following leveled selection and template chains) with the
// actor's race: catalogs key actors by it, not by the placed base. Empty when the chain does not resolve.
TemplateKey TemplateSource(RE::Actor* actor, TemplateCategory category);
}
