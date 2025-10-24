#include <Engine/Renderer/IRenderer.h>

namespace Engine::Renderer {

class RendererVulkan : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override { return true; }
    void RenderFrame() override {}
    void Shutdown() override {}
};

} // namespace Engine::Renderer
