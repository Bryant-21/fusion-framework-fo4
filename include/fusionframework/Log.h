#pragma once
// Inline: spdlog's default logger is per module, so it must be created in the plugin that logs.
#include "fusionframework/CriticalSyncLogger.h"
#include "fusionframework/Paths.h"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

namespace fusionframework::log {
// This launch plus the nine before it.
inline constexpr std::size_t kKeptRuns = 10;

// <F4SE log folder>/<plugin>.log, like every F4SE plugin.
inline std::filesystem::path FilePath(std::string_view plugin)
{
    return paths::LogDir() / (std::string{plugin} + ".log");
}

// Opens <plugin>.log fresh each launch; earlier runs move to <plugin>.1.log ... <plugin>.9.log.
inline std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> FileSink(std::string_view plugin)
{
    constexpr auto maxSize = std::numeric_limits<std::size_t>::max();
    return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(FilePath(plugin).string(), maxSize, kKeptRuns - 1, true);
}

// Default spdlog logger writing FileSink(plugin). Async: the logging thread only enqueues, one worker per plugin
// writes, and a full queue drops its oldest lines rather than stalling the game. syncCritical: critical lines are
// written before the call returns instead (CriticalSyncLogger).
inline void Init(std::string_view plugin, spdlog::level::level_enum level = spdlog::level::info, bool syncCritical = false)
{
    const auto flushLevel = std::min(level, spdlog::level::info);
    // Again (a settings reload): reopening would rotate the file this logger is writing.
    if (const auto current = spdlog::default_logger(); current && current->name() == plugin) {
        current->set_level(level);
        current->flush_on(flushLevel);
        return;
    }
    if (!spdlog::thread_pool()) spdlog::init_thread_pool(8192, 1);
    const auto file = FileSink(plugin);
    auto queue = std::make_shared<spdlog::async_logger>(std::string{plugin}, file, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);
    queue->flush_on(flushLevel);
    std::shared_ptr<spdlog::logger> log = queue;
    if (syncCritical) {
        queue->set_level(spdlog::level::trace);
        log = std::make_shared<CriticalSyncLogger>(std::string{plugin}, file, std::move(queue));
    }
    log->set_level(level);
    log->flush_on(flushLevel);
    spdlog::set_default_logger(std::move(log));
}
}
