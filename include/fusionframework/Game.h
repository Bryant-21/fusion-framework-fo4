#pragma once

// Engine-side framework sources are compiled inside each consumer's F4SE plugin target, which
// already provides CommonLibF4 and spdlog; this is the subset of a plugin pch they rely on.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <REL/Relocation.h>
#include <spdlog/spdlog.h>

#include <Windows.h>
