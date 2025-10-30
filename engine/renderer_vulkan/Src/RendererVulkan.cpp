#include <Engine/Renderer/IRenderer.h>

namespace Engine::Renderer {

class RendererVulkan : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override { return true; }
    void RenderFrame() override {}
    void Shutdown() override {}
    void ClearOverlayTexts() override { /* no-op for Vulkan stub */ }
    void AddOverlayText(const OverlayText& /*text*/) override { /* no-op for Vulkan stub */ }
};

} // namespace Engine::Renderer
