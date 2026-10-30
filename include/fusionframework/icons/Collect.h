#pragma once
// The runtime half of the item-icon renderer: a loaded model's visible triangles, in model space, for Rasterize.
// Weapons render with their attachments (the catalogs only know base forms); fan-made power armor renders its own model.
#include "fusionframework/icons/Raster.h"
#include "fusionframework/icons/Transform.h"
#include <RE/B/BSGraphics.h>
#include <RE/B/BSShaderProperty.h>
#include <RE/B/BSTriShape.h>
#include <RE/N/NiNode.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace fusionframework::icons {
inline float Half(std::uint16_t bits)
{
    const int exponent = (bits >> 10) & 31, mantissa = bits & 1023;
    const float value = exponent == 31 ? std::numeric_limits<float>::infinity() :
        std::ldexp(static_cast<float>(mantissa + (exponent ? 1024 : 0)), exponent ? exponent-25 : -24);
    return bits & 0x8000 ? -value : value;
}

// Skinned ground models (power armor, FO76's bows) draw in bind pose, like the offline catalog.
// loading: a shape's geometry had no CPU copy yet, which a model polled the frame it loads can show.
inline bool Collect(RE::NiAVObject& object, const RE::NiTransform& transform, std::vector<Triangle>& triangles, bool& loading,
    unsigned depth = 0)
{
    if (depth > 64) return false;
    if (object.GetAppCulled()) return true;
    if (auto* node = object.IsNode()) {
        for (auto& child : node->children)
            if (child && !Collect(*child, child->GetFlags() & kUsesParentWorld ? transform : Compose(transform, child->local),
                    triangles, loading, depth+1)) return false;
        return true;
    }
    auto* shape = object.IsTriShape();
    if (!shape) return true;
    // Heat haze (the minigun barrel's BaseRefractionMesh) only distorts the scene while firing.
    if (const auto* shader = static_cast<const RE::BSShaderProperty*>(shape->properties[1].get());
        shader && shader->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kRefraction)) return true;
    if (shape->IsDynamicTriShape() || triangles.size() + shape->numTriangles > 200000) return false;
    if (!shape->rendererData) { loading = true; return false; }
    const auto* geometry = static_cast<const RE::BSGraphics::TriShape*>(shape->rendererData);
    auto* vb = geometry->vertexBuffer;
    auto* ib = geometry->indexBuffer;
    auto desc = geometry->vertexDesc;
    const auto stride = desc.GetSize();
    const bool full = desc.HasFlag(RE::BSGraphics::Vertex::VF_FULLPREC);
    if (!desc.HasFlag(RE::BSGraphics::Vertex::VF_VERTEX) || stride < (full ? 12u : 6u)) return false;
    if (!vb || !ib || !vb->data || !ib->data || vb->invalidCpuData || ib->invalidCpuData ||
        vb->pendingCopy || ib->pendingCopy ||
        vb->dataSize < static_cast<std::size_t>(shape->numVertices)*stride ||
        ib->dataSize < static_cast<std::size_t>(shape->numTriangles)*6) { loading = true; return false; }
    std::vector<Point> vertices;
    vertices.reserve(shape->numVertices);
    for (unsigned i = 0; i < shape->numVertices; ++i) {
        const auto* source = static_cast<const std::byte*>(vb->data) + i*stride;
        RE::NiPoint3 p;
        if (full) std::memcpy(&p, source, 12);
        else {
            std::uint16_t xyz[3];
            std::memcpy(xyz, source, sizeof(xyz));
            p = {Half(xyz[0]), Half(xyz[1]), Half(xyz[2])};
        }
        p = Apply(transform, p);
        vertices.push_back({p.x,p.y,p.z});
    }
    for (unsigned i = 0; i < shape->numTriangles; ++i) {
        std::uint16_t indices[3];
        std::memcpy(indices, static_cast<const std::byte*>(ib->data) + i*6, sizeof(indices));
        if (indices[0] >= vertices.size() || indices[1] >= vertices.size() || indices[2] >= vertices.size()) return false;
        triangles.push_back({vertices[indices[0]],vertices[indices[1]],vertices[indices[2]]});
    }
    return true;
}
}
