#include "fusionframework/Busy.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace fusionframework::busy {
namespace {
std::mutex mutex;
std::vector<Task> tasks;
}

void Set(std::string_view owner, std::string_view label, std::uint32_t done, std::uint32_t total)
{
    const std::scoped_lock lock(mutex);
    const auto found = std::ranges::find(tasks, owner, &Task::owner);
    auto& task = found != tasks.end() ? *found : tasks.emplace_back(Task{std::string(owner)});
    task.label = label;
    task.done = done;
    task.total = total;
}

void Clear(std::string_view owner)
{
    const std::scoped_lock lock(mutex);
    std::erase_if(tasks, [&](const Task& task) { return task.owner == owner; });
}

std::optional<Task> Active()
{
    const std::scoped_lock lock(mutex);
    if (tasks.empty()) return std::nullopt;
    return tasks.front();
}
}
