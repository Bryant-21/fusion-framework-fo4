#pragma once
// Work a Fusion mod must finish before the player starts or loads a game (F4FS compiling shaders at the title). Owners
// publish it; menus that offer Continue / New / Load ask Active() and wait while it is set. No owner, no wait.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace fusionframework::busy {
struct Task {
    std::string owner;  // the publishing mod ("F4FS")
    std::string label;  // what it is doing, for a status line ("Compiling shaders")
    std::uint32_t done{}, total{};  // progress; total 0 when unknown
};

// Any thread. Set again to update; Clear when done (an unknown owner is ignored).
void Set(std::string_view owner, std::string_view label, std::uint32_t done, std::uint32_t total);
void Clear(std::string_view owner);
// The earliest published task still set.
std::optional<Task> Active();
}
