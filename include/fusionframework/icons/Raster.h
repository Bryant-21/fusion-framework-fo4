#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace fusionframework::icons {
inline constexpr int kWidth = 384, kHeight = 384, kSupersampling = 2;
inline constexpr int kArmorIconSize = 128;
inline constexpr int kListIconSize = 64, kListAlphaLevels = 8;
inline constexpr int kRenderWidth = kWidth*kSupersampling, kRenderHeight = kHeight*kSupersampling;
// Detail lines are cut into an opaque silhouette; a faint fill under bright lines was hard to see.
inline constexpr int kLineAlpha = 40;
// Lists show the same detailed icons as the wheel. True restores the catalog's list_runs
// silhouettes (helmet eyes) and plain silhouettes for fan-made power armor.
inline constexpr bool kListSilhouettes = false;
struct Point { float x, y, z; };
using Triangle = std::array<Point, 3>;
struct Run {
    int x, y, length, alpha;
    bool operator==(const Run&) const = default;
};
using Icon = std::vector<Run>;

inline int Pixel(const Icon& icon, int x, int y)
{
    for (const auto& run : icon)
        if (run.y == y && x >= run.x && x < run.x + run.length) return run.alpha;
    return 0;
}

// Inventory rows show icons at about text height. Full-size runs cost Flash thousands of
// fills per row, so lists receive a box-filtered square grid with quantized alpha.
inline Icon Downsample(const Icon& icon, int size, int target = kListIconSize)
{
    if (size <= 0 || target <= 0 || size % target) return {};
    const int factor = size / target;
    std::vector<int> coverage(static_cast<std::size_t>(target) * target);
    for (const auto& run : icon) {
        if (run.y < 0 || run.y >= size) continue;
        const int row = run.y / factor * target;
        for (int x = std::max(run.x, 0), end = std::min(run.x + run.length, size); x < end;) {
            const int next = std::min(end, (x / factor + 1) * factor);
            coverage[row + x / factor] += run.alpha * (next - x);
            x = next;
        }
    }
    Icon result;
    for (int y = 0; y < target; ++y) {
        int start = 0, previous = 0;
        for (int x = 0; x <= target; ++x) {
            const int average = x < target ? coverage[y*target+x] / (factor*factor) : 0;
            const int alpha = (average * kListAlphaLevels + 127) / 255 * 255 / kListAlphaLevels;
            if (alpha == previous) continue;
            if (previous) result.push_back({start, y, x - start, previous});
            start = x;
            previous = alpha;
        }
    }
    return result;
}

// List rows show power armor as a plain silhouette; fan-made sets have no catalog list artwork.
inline Icon Solid(const Icon& icon)
{
    Icon result;
    for (const auto& run : icon) {
        if (!result.empty() && result.back().y == run.y && result.back().x + result.back().length == run.x)
            result.back().length += run.length;
        else result.push_back({run.x, run.y, run.length, 255});
    }
    return result;
}

// Weapons: the side view turned 45 degrees in the square. Power armor: the front, which faces
// model +Y, projected like the offline catalog so generated and catalogued sets match. A camera in
// front sees the wearer's right (+X) on its left; drawing +X rightward showed left pieces as right ones.
enum class View { Side, Diagonal, Front };

inline Icon Rasterize(const std::vector<Triangle>& triangles, View view = View::Diagonal)
{
    if (triangles.empty()) return {};
    auto low = triangles.front()[0], high = low;
    for (const auto& triangle : triangles) for (const auto& p : triangle) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return {};
        low = {std::min(low.x,p.x), std::min(low.y,p.y), std::min(low.z,p.z)};
        high = {std::max(high.x,p.x), std::max(high.y,p.y), std::max(high.z,p.z)};
    }
    const bool alongX = high.x - low.x >= high.y - low.y;
    const bool diagonal = view == View::Diagonal;
    const auto project = [alongX, diagonal, view](Point p) {
        if (view == View::Front) return Point{-p.x, -p.z, p.y};
        const float horizontal = alongX ? p.x : p.y;
        const float vertical = -p.z;
        constexpr float cosine = 0.7071067811865475F;
        return Point{diagonal ? (horizontal + vertical)*cosine : horizontal,
            diagonal ? (vertical - horizontal)*cosine : vertical, alongX ? p.y : p.x};
    };
    low = high = project(triangles.front()[0]);
    for (const auto& triangle : triangles) for (const auto& p : triangle) {
        const auto v = project(p);
        low = {std::min(low.x,v.x), std::min(low.y,v.y), std::min(low.z,v.z)};
        high = {std::max(high.x,v.x), std::max(high.y,v.y), std::max(high.z,v.z)};
    }
    const float width = high.x - low.x;
    const float height = high.y - low.y;
    if (!(width > 0) || !(height > 0) || !std::isfinite(width) || !std::isfinite(height)) return {};
    const int renderHeight = view == View::Side ? kRenderWidth/2 : kRenderHeight;
    constexpr int rasterScale = kRenderWidth/192;
    const float scale = std::min((kRenderWidth - 8*rasterScale) / width, (renderHeight - 8*rasterScale) / height);
    if (!std::isfinite(scale)) return {};
    const float left = (kRenderWidth - width * scale) / 2, top = (renderHeight - height * scale) / 2;
    struct Surface { Point normal; float depth; bool covered; };
    std::vector<Surface> surfaces(kRenderWidth * renderHeight);
    std::size_t samples = 0;
    const auto edge = [](Point a, Point b, Point p) { return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x); };
    for (const auto& triangle : triangles) {
        std::array<Point,3> v;
        for (int i = 0; i < 3; ++i) {
            const auto p = project(triangle[i]);
            v[i] = {left + (p.x-low.x)*scale, top + (p.y-low.y)*scale, (p.z-low.z)*scale};
        }
        const float area = edge(v[0],v[1],v[2]);
        if (std::abs(area) < 0.0001F) continue;
        const Point u{v[1].x-v[0].x, v[1].y-v[0].y, v[1].z-v[0].z};
        const Point w{v[2].x-v[0].x, v[2].y-v[0].y, v[2].z-v[0].z};
        Point normal{u.y*w.z-u.z*w.y, u.z*w.x-u.x*w.z, area};
        const float inverseLength = (area < 0 ? -1.0F : 1.0F) /
            std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        normal = {normal.x*inverseLength, normal.y*inverseLength, normal.z*inverseLength};
        const int x0 = std::clamp(static_cast<int>(std::floor(std::min({v[0].x,v[1].x,v[2].x}))), 0, kRenderWidth-1);
        const int x1 = std::clamp(static_cast<int>(std::ceil(std::max({v[0].x,v[1].x,v[2].x}))), 0, kRenderWidth-1);
        const int y0 = std::clamp(static_cast<int>(std::floor(std::min({v[0].y,v[1].y,v[2].y}))), 0, renderHeight-1);
        const int y1 = std::clamp(static_cast<int>(std::ceil(std::max({v[0].y,v[1].y,v[2].y}))), 0, renderHeight-1);
        samples += static_cast<std::size_t>(x1-x0+1) * (y1-y0+1);
        if (samples > 4000000*rasterScale*rasterScale) return {};
        for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
            const Point p{x+0.5F,y+0.5F,0};
            const float a = edge(v[0],v[1],p), b = edge(v[1],v[2],p), c = edge(v[2],v[0],p);
            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0)) {
                const float depth = (b*v[0].z + c*v[1].z + a*v[2].z) / area;
                auto& surface = surfaces[y*kRenderWidth+x];
                if (!surface.covered || depth > surface.depth) surface = {normal,depth,true};
            }
        }
    }
    const auto crease = [&](int i, int dx, int dy) {
        const auto& a = surfaces[i];
        const auto& b = surfaces[i+dy*kRenderWidth+dx];
        if (!b.covered) return false;
        const float dot = a.normal.x*b.normal.x + a.normal.y*b.normal.y + a.normal.z*b.normal.z;
        if (dot < 0.85F) return true;
        const float depth = b.depth-a.depth;
        // Plane distance keeps an ordinary sloping surface from becoming a band of false edges.
        return std::abs(depth*a.normal.z + dx*a.normal.x + dy*a.normal.y) > rasterScale &&
            std::abs(depth*b.normal.z + dx*b.normal.x + dy*b.normal.y) > rasterScale;
    };
    Icon result;
    for (int y = 1; y < renderHeight/kSupersampling-1; ++y) {
        int start = 0, previous = 0;
        for (int x = 0; x <= kWidth; ++x) {
            int coverage = 0;
            if (x < kWidth) for (int dy = 0; dy < kSupersampling; ++dy) for (int dx = 0; dx < kSupersampling; ++dx) {
                const auto i = (y*kSupersampling+dy)*kRenderWidth+x*kSupersampling+dx;
                if (!surfaces[i].covered) continue;
                coverage += crease(i,-kSupersampling,0) || crease(i,kSupersampling,0) ||
                    crease(i,0,-kSupersampling) || crease(i,0,kSupersampling) ? kLineAlpha : 255;
            }
            const int alpha = (coverage + kSupersampling*kSupersampling/2) / (kSupersampling*kSupersampling);
            if (alpha == previous) continue;
            if (previous) result.push_back({start,y,x-start,previous});
            start = x;
            previous = alpha;
        }
    }
    return result;
}
}
