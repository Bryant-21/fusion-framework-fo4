#include "fusionframework/Game.h"
#include "fusionframework/ActorUpdate.h"
#include "fusionframework/Hooks.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace fusionframework::actors {
namespace {
// Actor::RestoreHealth and Actor::UpdateMagic, {OG 1.10.163, NG, AE}.
constexpr REL::ID kRestoreHealth{ 90360, 2230971, 2230971 };
constexpr REL::ID kUpdateMagic{ 1275301, 2231313, 2231313 };

using UpdateFn = void (*)(RE::Actor*, float);
UpdateFn originalRestore{};
UpdateFn originalUpdateMagic{};
// Filled at load; read-only once installed.
std::vector<Callback> perFrame;
std::vector<Callback> everyRestore;
thread_local RE::Actor* updatingMagic{};

void Add(std::vector<Callback>& list, Callback callback)
{
    if (callback && std::ranges::find(list, callback) == list.end()) list.push_back(callback);
}

// RestoreHealth runs from several callers; only the one inside UpdateMagic is the per-frame tick.
void RestoreHealth(RE::Actor* actor, float seconds)
{
    originalRestore(actor, seconds);
    if (!actor) return;
    for (const auto callback : everyRestore) callback(actor, seconds);
    if (actor == updatingMagic && std::isfinite(seconds) && seconds > 0.0F)
        for (const auto callback : perFrame) callback(actor, seconds);
}

void UpdateMagic(RE::Actor* actor, float seconds)
{
    const auto previous = updatingMagic;
    updatingMagic = actor;
    originalUpdateMagic(actor, seconds);
    updatingMagic = previous;
}
}

void OnUpdate(Callback callback) { Add(perFrame, callback); }

void OnRestoreHealth(Callback callback) { Add(everyRestore, callback); }

bool Install()
{
    if (originalRestore || (perFrame.empty() && everyRestore.empty())) return true;
    std::vector<hooks::Hook> list{ { kRestoreHealth, reinterpret_cast<void*>(&RestoreHealth), reinterpret_cast<void**>(&originalRestore) } };
    if (!perFrame.empty())
        list.push_back({ kUpdateMagic, reinterpret_cast<void*>(&UpdateMagic), reinterpret_cast<void**>(&originalUpdateMagic) });
    return hooks::Install(list, "actor update");
}
}
