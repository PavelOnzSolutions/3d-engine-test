#pragma once

#include <Engine/Renderer/Core/IRenderPass.h>
#include <memory>

namespace Engine { namespace Renderer {

// Forward decls
class FrameGraph;
class RenderContext;

class GBufferPass : public IRenderPass {
public:
    const char* GetName() const override { return "GBufferPass"; }
    void Setup(FrameGraph& /*fg*/) override {}
    void Execute(RenderContext& /*ctx*/) override {}
};

class LightingPass : public IRenderPass {
public:
    const char* GetName() const override { return "LightingPass"; }
    void Setup(FrameGraph& /*fg*/) override {}
    void Execute(RenderContext& /*ctx*/) override {}
};

class PostProcessPass : public IRenderPass {
public:
    const char* GetName() const override { return "PostProcessPass"; }
    void Setup(FrameGraph& /*fg*/) override {}
    void Execute(RenderContext& /*ctx*/) override {}
};

class PresentPass : public IRenderPass {
public:
    const char* GetName() const override { return "PresentPass"; }
    void Setup(FrameGraph& /*fg*/) override {}
    void Execute(RenderContext& /*ctx*/) override {}
};

}} // namespace Engine::Renderer
