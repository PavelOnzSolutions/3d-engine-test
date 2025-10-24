#pragma once

#include <Engine/Renderer/Core/IRenderPass.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/RenderContext.h>
#include <Engine/Renderer/Scene.h>
#include <memory>
#include <string>
#include <vector>

namespace Engine { namespace Renderer {

// Scene-related resource keys
namespace SceneResources {
    inline constexpr const char* ScenePtr       = "Scene.Active";
    inline constexpr const char* CameraPtr      = "Scene.Camera";
    inline constexpr const char* CulledDrawsPtr = "Scene.CulledDraws"; // std::vector<DrawCall>*
}

// A CPU-side scene culling pass producing a vector of DrawCalls from Scene+Camera
class SceneCullingPass : public IRenderPass {
public:
    const char* GetName() const override { return "SceneCullingPass"; }
    void Setup(FrameGraph& fg) override {
        // Ensure output resource key exists
        if (!fg.GetResource(SceneResources::CulledDrawsPtr))
            fg.SetResource(SceneResources::CulledDrawsPtr, nullptr);
    }
    void Execute(RenderContext& /*ctx*/, FrameGraph& fg) override {
        auto* scene  = static_cast<const Scene*>(fg.GetResource(SceneResources::ScenePtr));
        auto* camera = static_cast<const Camera*>(fg.GetResource(SceneResources::CameraPtr));
        auto* draws  = static_cast<std::vector<DrawCall>*>(fg.GetResource(SceneResources::CulledDrawsPtr));
        if (!scene || !camera || !draws)
            return; // resources not bound yet
        CullScene(*scene, *camera, *draws);
    }
};

// Resource keys used by deferred renderer
namespace DeferredResources {
    inline constexpr const char* ShadowMap   = "Shadow.Map";
    inline constexpr const char* GAlbedo     = "GBuffer.Albedo";
    inline constexpr const char* GNormals    = "GBuffer.Normals";
    inline constexpr const char* GMaterial   = "GBuffer.Material";
    inline constexpr const char* GDepth      = "GBuffer.Depth";
    inline constexpr const char* HDRLighting = "Lighting.HDR";
    inline constexpr const char* LDRColor    = "Post.LDR";
}

class ShadowPass : public IRenderPass {
public:
    const char* GetName() const override { return "ShadowPass"; }
    void Setup(FrameGraph& fg) override {
        // Register produced shadow map resource placeholder
        fg.SetResource(DeferredResources::ShadowMap, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
        // Placeholder: render shadow map for scene lights
        // When MethaneKit is available, here we'd record commands into the command list.
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: create or bind shadow map pipeline and draw depth-only
        }
#endif
    }
};

class GBufferPass : public IRenderPass {
public:
    const char* GetName() const override { return "GBufferPass"; }
    void Setup(FrameGraph& fg) override {
        // Declare G-buffer attachments
        fg.SetResource(DeferredResources::GAlbedo, nullptr);
        fg.SetResource(DeferredResources::GNormals, nullptr);
        fg.SetResource(DeferredResources::GMaterial, nullptr);
        fg.SetResource(DeferredResources::GDepth, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: bind G-buffer render state and draw scene geometry
        }
#endif
    }
};

class DeferredLightingPass : public IRenderPass {
public:
    const char* GetName() const override { return "DeferredLightingPass"; }
    void Setup(FrameGraph& fg) override {
        // Declare HDR lighting target produced from G-buffer + shadows
        fg.SetResource(DeferredResources::HDRLighting, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: fullscreen quad combining G-buffer and shadow map
        }
#endif
    }
};

class PostProcessPass : public IRenderPass {
public:
    const char* GetName() const override { return "PostProcessPass"; }
    void Setup(FrameGraph& fg) override {
        // Post-processing produces LDR color for present
        fg.SetResource(DeferredResources::LDRColor, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
        // Placeholder effects: bloom, tone mapping, TAA (stub)
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: apply simple tone-mapping on HDRLighting to LDRColor
        }
#endif
    }
};

class PresentPass : public IRenderPass {
public:
    const char* GetName() const override { return "PresentPass"; }
    void Setup(FrameGraph& /*fg*/) override {}
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
        // Placeholder: present swap chain
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: transition LDRColor to present and present the frame
        }
#endif
    }
};

// Resource keys used by optional Forward+ renderer with clustered light culling
namespace ForwardPlusResources {
    inline constexpr const char* Depth          = "ForwardPlus.Depth";        // optional depth pre-pass
    inline constexpr const char* ClusterAABBs   = "ForwardPlus.ClusterAABBs"; // cluster bounding boxes/grid
    inline constexpr const char* LightIndices   = "ForwardPlus.LightIndices"; // flattened per-cluster light index list
    inline constexpr const char* LightGrid      = "ForwardPlus.LightGrid";    // per-cluster offset/count into LightIndices
    inline constexpr const char* HDRLighting    = "ForwardPlus.HDR";          // shaded HDR color
}

// Optional depth-only pre-pass for Forward+ to get a stable depth buffer
class DepthPrepass : public IRenderPass {
public:
    const char* GetName() const override { return "DepthPrepass"; }
    void Setup(FrameGraph& fg) override {
        fg.SetResource(ForwardPlusResources::Depth, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: bind depth-only pipeline and render opaque geometry to populate depth buffer
        }
#endif
    }
};

// Cluster generation and light culling pass (CPU/GPU compute placeholder)
class ClusterBuildPass : public IRenderPass {
public:
    const char* GetName() const override { return "ClusterBuildPass"; }
    void Setup(FrameGraph& fg) override {
        // Declare cluster data products
        fg.SetResource(ForwardPlusResources::ClusterAABBs, nullptr);
        fg.SetResource(ForwardPlusResources::LightGrid, nullptr);
        fg.SetResource(ForwardPlusResources::LightIndices, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: compute dispatch to build screen-space clusters and cull lights per cluster
            // Inputs: camera params, optionally depth (ForwardPlusResources::Depth)
            // Outputs: LightGrid (offset,count) and LightIndices array
        }
#endif
    }
};

// Forward+ lighting pass that shades visible geometry using clustered light lists
class ForwardPlusLightingPass : public IRenderPass {
public:
    const char* GetName() const override { return "ForwardPlusLightingPass"; }
    void Setup(FrameGraph& fg) override {
        fg.SetResource(ForwardPlusResources::HDRLighting, nullptr);
    }
    void Execute(RenderContext& ctx, FrameGraph& /*fg*/) override {
        (void)ctx;
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: bind forward material shaders using cluster buffers to shade lights per-fragment
        }
#endif
    }
};

// Convenience builders to assemble typical pipelines. These are optional helpers
// allowing the application to switch between Deferred and Forward+ pipelines.
inline void BuildDeferredPipeline(FrameGraph& fg)
{
    using std::make_shared;
    // Insert culling at the beginning
    fg.AddPass(make_shared<SceneCullingPass>());
    fg.AddPass(make_shared<ShadowPass>());
    fg.AddPass(make_shared<GBufferPass>());
    fg.AddPass(make_shared<DeferredLightingPass>());
    fg.AddPass(make_shared<PostProcessPass>());
    fg.AddPass(make_shared<PresentPass>());
}

inline void BuildForwardPlusPipeline(FrameGraph& fg, bool use_depth_prepass = true)
{
    using std::make_shared;
    fg.AddPass(make_shared<SceneCullingPass>());
    if (use_depth_prepass)
        fg.AddPass(make_shared<DepthPrepass>());
    fg.AddPass(make_shared<ClusterBuildPass>());
    fg.AddPass(make_shared<ForwardPlusLightingPass>());
    fg.AddPass(make_shared<PostProcessPass>());
    fg.AddPass(make_shared<PresentPass>());
}

}} // namespace Engine::Renderer
