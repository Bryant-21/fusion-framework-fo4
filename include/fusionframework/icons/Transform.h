#pragma once
#include <cassert>
#include <cmath>
#include <format>
#include <RE/N/NiTransform.h>

namespace fusionframework::icons {
inline RE::NiTransform ModelSpace()
{
    // The pinned CommonLib IDENTITY constant repeats its first row on all three axes.
    return {{1,0,0,0, 0,1,0,0, 0,0,1,0}, {0,0,0}, 1};
}

// FO4 rotates row vectors: NiTransform::operator* maps p to (p R) s + T and a child's world is
// local.R parent.R (1.10.155 PDB). The pinned CommonLib operators use column vectors, which turns
// every rotated node the opposite way.
inline RE::NiPoint3 Apply(const RE::NiTransform& t, const RE::NiPoint3& p)
{
    const auto& r = t.rotate;
    return RE::NiPoint3{p.x*r[0][0] + p.y*r[1][0] + p.z*r[2][0],
        p.x*r[0][1] + p.y*r[1][1] + p.z*r[2][1],
        p.x*r[0][2] + p.y*r[1][2] + p.z*r[2][2]} * t.scale + t.translate;
}

inline RE::NiTransform Compose(const RE::NiTransform& parent, const RE::NiTransform& local)
{
    auto world = ModelSpace();
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j)
        world.rotate[i][j] = local.rotate[i][0]*parent.rotate[0][j] + local.rotate[i][1]*parent.rotate[1][j] +
            local.rotate[i][2]*parent.rotate[2][j];
    world.translate = Apply(parent, local.translate);
    world.scale = parent.scale * local.scale;
    return world;
}

// NiAVObject::UpdateWorldData copies the parent's world transform, ignoring local, for this flag.
inline constexpr std::uint64_t kUsesParentWorld = 0x200000000000;
}
