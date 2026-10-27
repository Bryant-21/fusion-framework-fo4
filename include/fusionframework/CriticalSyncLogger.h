#pragma once
#include <spdlog/async_logger.h>
#include <spdlog/sinks/sink.h>

#include <memory>
#include <string>

namespace fusionframework::log {
// Writes critical lines to the file and flushes before returning, so they survive a crash that follows; hands
// every other line to the async logger. A critical line can land ahead of lines still queued before it.
class CriticalSyncLogger final : public spdlog::logger
{
public:
    CriticalSyncLogger(std::string name, spdlog::sink_ptr file, std::shared_ptr<spdlog::async_logger> queue) :
        logger(std::move(name), std::move(file)), queue_(std::move(queue))
    {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (msg.level < spdlog::level::critical) {
            queue_->log(msg.time, msg.source, msg.level, msg.payload);
            return;
        }
        for (auto& sink : sinks_) {
            sink->log(msg);
            sink->flush();
        }
    }
    void flush_() override { queue_->flush(); }

private:
    std::shared_ptr<spdlog::async_logger> queue_;
};
}
