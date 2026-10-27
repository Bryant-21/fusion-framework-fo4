#include "fusionframework/Game.h"
#include "fusionframework/Task.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace fusionframework::task {
void Every(std::function<std::chrono::milliseconds()> interval, std::function<bool()> gate, std::function<void()> tick)
{
    static std::mutex mutex;
    static std::vector<std::jthread> tickers;
    auto queued = std::make_shared<std::atomic_bool>(false);
    auto shared = std::make_shared<std::function<void()>>(std::move(tick));
    std::lock_guard lock(mutex);
    tickers.emplace_back([interval = std::move(interval), gate = std::move(gate), queued, shared](std::stop_token stop) {
        std::mutex waitMutex;
        std::condition_variable_any wake;
        std::unique_lock waitLock(waitMutex);
        // Woken by the stop request, so unloading never waits out an interval.
        while (!wake.wait_for(waitLock, stop, interval(), [&] { return stop.stop_requested(); })) {
            auto* tasks = F4SE::GetTaskInterface();
            // A passing gate is always followed by its tick, so a gate may claim state the tick releases.
            if (!tasks || queued->load() || (gate && !gate())) continue;
            queued->store(true);
            tasks->AddTask([queued, shared] {
                queued->store(false);
                (*shared)();
            });
        }
    });
}
}
