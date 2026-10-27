#pragma once
// Compiled into each consumer (fusionframework_use()): AddTask goes through the plugin's own F4SE task interface.
#include <chrono>
#include <functional>

namespace fusionframework::task {
// Runs `tick` on the game thread every `interval` (read again before each wait) while `gate` (empty: always)
// passes. Coalesced: gate is asked on the ticker thread only while no tick is queued, so a slow game thread
// never builds a backlog, and a passing gate is always followed by its tick. Each call starts a new ticker
// that lives until the plugin unloads: call it once per feature.
void Every(std::function<std::chrono::milliseconds()> interval, std::function<bool()> gate, std::function<void()> tick);

inline void Every(std::chrono::milliseconds interval, std::function<bool()> gate, std::function<void()> tick)
{
    Every([interval] { return interval; }, std::move(gate), std::move(tick));
}
}
