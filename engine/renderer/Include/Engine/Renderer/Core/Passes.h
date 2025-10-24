#pragma once

#include <Engine/Renderer/Core/IRenderPass.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/RenderContext.h>
#include <memory>
#include <string>

namespace Engine { namespace Renderer {

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
    void Execute(RenderContext& ctx) override {
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
    void Execute(RenderContext& ctx) override {
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
    void Execute(RenderContext& ctx) override {
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
    void Execute(RenderContext& ctx) override {
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
    void Execute(RenderContext& ctx) override {
        (void)ctx;
        // Placeholder: present swap chain
#if defined(ENGINE_USE_METHANEKIT)
        if (ctx.IsValid()) {
            // TODO: transition LDRColor to present and present the frame
        }
#endif
    }
};

}} // namespace Engine::Renderer
