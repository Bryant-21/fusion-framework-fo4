#include "fusionframework/Game.h"
#include "fusionframework/Hooks.h"

#include <MinHook.h>

#include <vector>

namespace fusionframework::hooks {
bool Install(std::span<const Hook> hooks, std::string_view feature)
{
    const auto initialized = MH_Initialize();
    if (initialized != MH_OK && initialized != MH_ERROR_ALREADY_INITIALIZED) {
        spdlog::error("{}: MinHook initialize failed ({})", feature, MH_StatusToString(initialized));
        return false;
    }
    std::vector<const Hook*> created;
    // Detours test their trampoline for null, so a rolled-back hook leaves it cleared.
    const auto rollback = [&] {
        for (const auto* hook : created) {
            MH_DisableHook(reinterpret_cast<void*>(hook->address));
            MH_RemoveHook(reinterpret_cast<void*>(hook->address));
            if (hook->original) *hook->original = nullptr;
        }
    };
    for (const auto& hook : hooks) {
        auto* target = reinterpret_cast<void*>(hook.address);
        auto status = MH_CreateHook(target, hook.detour, hook.original);
        if (status == MH_OK) {
            created.push_back(&hook);
            status = MH_QueueEnableHook(target);
        }
        if (status != MH_OK) {
            spdlog::error("{}: hook at {:X} failed ({})", feature, hook.address, MH_StatusToString(status));
            rollback();
            return false;
        }
    }
    if (const auto status = MH_ApplyQueued(); status != MH_OK) {
        spdlog::error("{}: hook activation failed ({})", feature, MH_StatusToString(status));
        rollback();
        return false;
    }
    return true;
}
}
