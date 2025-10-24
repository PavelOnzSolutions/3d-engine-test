#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Engine { namespace Renderer {

class RenderContext; // fwd
class FrameGraph;    // fwd

// Basic engine-level render pass interface
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    // Human-readable name for debugging/logging
    virtual const char* GetName() const = 0;

    // Called once when the pass is added to the frame-graph (or when graph is compiled)
    // Use to declare required and produced resources via FrameGraph API.
    virtual void Setup(FrameGraph& fg) = 0;

    // Called every frame in the graph execution order
    virtual void Execute(RenderContext& ctx, FrameGraph& fg) = 0;
};

using IRenderPassPtr = std::shared_ptr<IRenderPass>;

}} // namespace Engine::Renderer
