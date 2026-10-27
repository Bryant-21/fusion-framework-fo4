#pragma once
// Compiled into each consumer (fusionframework_use()): ids resolve against the plugin's own Address Library
// and every plugin links its own MinHook.
#include <REL/Relocation.h>

#include <cstdint>
#include <span>
#include <string_view>

namespace fusionframework::hooks {
// The target is resolved on construction, so build hooks where they are installed, not as globals.
// Constructors rather than a std::variant: VariantID's variadic constructor hijacks copies from a lvalue.
struct Hook {
    Hook(REL::ID id, void* detour, void** original) :
        Hook(REL::Relocation<std::uintptr_t>{ id }.address(), detour, original) {}
    Hook(const REL::VariantID& id, void* detour, void** original) :
        Hook(REL::Relocation<std::uintptr_t>{ id }.address(), detour, original) {}
    Hook(std::uintptr_t address, void* detour, void** original) :
        address(address), detour(detour), original(original) {}

    std::uintptr_t address;
    void* detour;
    void** original;
};

// All-or-nothing MinHook install: either every hook is enabled or none is left behind. Failures are logged
// under `feature`.
bool Install(std::span<const Hook> hooks, std::string_view feature);

inline bool Install(const Hook& hook, std::string_view feature)
{
    return Install(std::span{ &hook, 1 }, feature);
}
}
