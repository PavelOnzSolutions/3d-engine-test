#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <functional>

namespace Engine { namespace Renderer {

// Minimal math types to avoid external dependencies
struct Vec3 { float x{}, y{}, z{}; };
struct Vec4 { float x{}, y{}, z{}, w{}; };
struct Mat4 { // column-major 4x4
    float m[16]{}; // m[col*4 + row]
};

inline Mat4 Mul(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int r0 = 0; r0 < 4; ++r0)
            r.m[c*4 + r0] = a.m[0*4 + r0]*b.m[c*4 + 0] + a.m[1*4 + r0]*b.m[c*4 + 1] + a.m[2*4 + r0]*b.m[c*4 + 2] + a.m[3*4 + r0]*b.m[c*4 + 3];
    return r;
}

struct AABB {
    Vec3 min{ 0,0,0 };
    Vec3 max{ 0,0,0 };
};

struct Camera {
    Mat4 view{};
    Mat4 proj{};
    Mat4 view_proj{};
    // 6 frustum planes in form ax + by + cz + d >= 0 inside; order: left, right, bottom, top, near, far
    std::array<Vec4, 6> planes{};

    void SetViewProjection(const Mat4& v, const Mat4& p)
    {
        view = v; proj = p; view_proj = Mul(p, v); ExtractFrustumPlanes();
    }

    void ExtractFrustumPlanes()
    {
        const float* m = view_proj.m; // column-major
        // Convert to row-major convenience by gathering rows
        auto row = [&](int r){ return std::array<float,4>{ m[0*4+r], m[1*4+r], m[2*4+r], m[3*4+r] }; };
        auto r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
        // Left  = r3 + r0
        planes[0] = { r3[0]+r0[0], r3[1]+r0[1], r3[2]+r0[2], r3[3]+r0[3] };
        // Right = r3 - r0
        planes[1] = { r3[0]-r0[0], r3[1]-r0[1], r3[2]-r0[2], r3[3]-r0[3] };
        // Bottom= r3 + r1
        planes[2] = { r3[0]+r1[0], r3[1]+r1[1], r3[2]+r1[2], r3[3]+r1[3] };
        // Top   = r3 - r1
        planes[3] = { r3[0]-r1[0], r3[1]-r1[1], r3[2]-r1[2], r3[3]-r1[3] };
        // Near  = r3 + r2
        planes[4] = { r3[0]+r2[0], r3[1]+r2[1], r3[2]+r2[2], r3[3]+r2[3] };
        // Far   = r3 - r2
        planes[5] = { r3[0]-r2[0], r3[1]-r2[1], r3[2]-r2[2], r3[3]-r2[3] };

        // Normalize planes
        for (auto& p2 : planes)
        {
            float len = std::sqrt(p2.x*p2.x + p2.y*p2.y + p2.z*p2.z);
            if (len > 0.0f) { p2.x/=len; p2.y/=len; p2.z/=len; p2.w/=len; }
        }
    }
};

struct DrawCall {
    int mesh_id{ -1 };
    int material_id{ -1 };
    Mat4 model{};
};

struct Renderable {
    AABB world_bounds{}; // world-space bounds for simplicity
    Mat4 model{};
    int mesh_id{ -1 };
    int material_id{ -1 };
    bool enabled{ true };
};

struct Scene {
    std::vector<Renderable> renderables;
};

// ECS-friendly traversal; compatible with SoA or external ECS by adapting lambda capture
inline void ForEachRenderable(const Scene& scene, const std::function<void(const Renderable&)>& fn)
{
    for (const auto& r : scene.renderables)
        if (r.enabled)
            fn(r);
}

inline bool AABBInsideFrustum(const AABB& b, const std::array<Vec4,6>& planes)
{
    // Check against each plane using positive vertex test
    for (const auto& p : planes)
    {
        Vec3 vp{ p.x >= 0 ? b.max.x : b.min.x,
                 p.y >= 0 ? b.max.y : b.min.y,
                 p.z >= 0 ? b.max.z : b.min.z };
        float dist = p.x*vp.x + p.y*vp.y + p.z*vp.z + p.w;
        if (dist < 0.0f)
            return false; // completely outside
    }
    return true;
}

inline void CullScene(const Scene& scene, const Camera& cam, std::vector<DrawCall>& out_draws)
{
    out_draws.clear();
    ForEachRenderable(scene, [&](const Renderable& r){
        if (AABBInsideFrustum(r.world_bounds, cam.planes))
        {
            DrawCall dc; dc.mesh_id = r.mesh_id; dc.material_id = r.material_id; dc.model = r.model; out_draws.push_back(dc);
        }
    });
}

}} // namespace Engine::Renderer
