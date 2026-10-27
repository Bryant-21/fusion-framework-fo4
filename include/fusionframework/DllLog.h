#pragma once
// Logging for the Fusion DLLs themselves. Plugins earlier in the load order call into a DLL before F4SE runs its
// F4SEPlugin_Load, so messages wait in memory until Open() creates the DLL's own log file.
#include "fusionframework/Log.h"

#include <spdlog/sinks/ringbuffer_sink.h>

namespace fusionframework::dll_log {
inline std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt>& Early()
{
    static auto sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(512);
    return sink;
}

// From a static initializer of the DLL.
inline bool Start(std::string_view name)
{
    auto logger = std::make_shared<spdlog::logger>(std::string{name}, Early());
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(std::move(logger));
    return true;
}

// From the DLL's F4SEPlugin_Load, after F4SE::Init.
inline void Open(std::string_view name)
{
    const auto early = Early()->last_formatted();
    log::Init(name);
    for (auto line : early) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        spdlog::info("(before load) {}", line);
    }
    Early().reset();
}
}
