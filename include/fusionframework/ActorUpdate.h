#pragma once
// Compiled into each consumer (fusionframework_use()); hooks through hooks::Install with the plugin's own MinHook.

namespace RE { class Actor; }

namespace fusionframework::actors {
using Callback = void (*)(RE::Actor* actor, float seconds);

// Per actor, per frame (game time: paused with the game): Actor::RestoreHealth when Actor::UpdateMagic calls it,
// after the native restore. Calls with no elapsed time are skipped. UpdateMagic runs on parallel actor-update job
// threads, not the main thread; these callbacks are serialized, one at a time across all threads.
void OnUpdate(Callback callback);
// Every Actor::RestoreHealth call, whoever makes it, after the native restore.
void OnRestoreHealth(Callback callback);
// Load time only: register first, then install once. Installs nothing when no callback was registered;
// false when the hooks failed (logged).
bool Install();
}
