#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine { namespace Renderer {

class IRenderPass;
class RenderContext;

// Minimal frame graph that manages a sequence of render passes and a
// trivial resource registry placeholder for future extensions.
class FrameGraph {
public:
    using PassPtr = std::shared_ptr<IRenderPass>;

    // Add a pass to the graph; Setup will be called during Compile().
    void AddPass(const PassPtr& pass);

    // Call Setup() on all passes; build dependencies in future.
    void Compile();

    // Execute all passes in the order they were added.
    void Execute(RenderContext& ctx);

    // Remove all passes and resources.
    void Clear();

    // Simple string-keyed resource registry for future wiring; currently unused but kept for API shape.
    void SetResource(const std::string& name, void* ptr) { m_resources[name] = ptr; }
    void* GetResource(const std::string& name) const {
        auto it = m_resources.find(name);
        return it == m_resources.end() ? nullptr : it->second;
    }

    const std::vector<PassPtr>& GetPasses() const { return m_passes; }

private:
    std::vector<PassPtr> m_passes;
    std::unordered_map<std::string, void*> m_resources;
    bool m_compiled{false};
};

}} // namespace Engine::Renderer
