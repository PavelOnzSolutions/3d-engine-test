#include <Engine/Renderer/IRenderer.h>

namespace Engine::Renderer {
    class RendererDX12 : public IRenderer {
    public:
        bool Initialize(void * /*windowHandle*/) override { return true; }

        void RenderFrame() override {
        }

        void Shutdown() override {
        }

        void ClearOverlayTexts() override {
            /* no-op for DX12 stub */
        }

        void AddOverlayText(const OverlayText & /*text*/) override {
            /* no-op for DX12 stub */
        }
    };
} // namespace Engine::Renderer
