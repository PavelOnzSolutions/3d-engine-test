#include <Engine/Renderer/RendererFactory.h>
#include <Engine/Renderer/RenderContext.h>
#include <memory>
#include <string>

namespace Engine::Renderer {

namespace {

class RendererDX12Stub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override { return true; }
    void RenderFrame() override {}
    void Shutdown() override {}
};

class RendererVulkanStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override { return true; }
    void RenderFrame() override {}
    void Shutdown() override {}
};

class RendererMethaneStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        // Create a dummy RenderContext; real integration will pass Methane RHI objects
        m_ctx = std::make_unique<RenderContext>();
        m_ctx->BeginFrame();
        m_ctx->EndFrame();
        return true;
    }
    void RenderFrame() override {
        if (m_ctx && !m_ctx->IsFrameRecording()) {
            m_ctx->BeginFrame();
            // ... record commands when integrated ...
            m_ctx->EndFrame();
        }
    }
    void Shutdown() override { m_ctx.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
};

} // anonymous namespace

std::unique_ptr<IRenderer> CreateRenderer(const std::string& id)
{
    std::string lid;
    lid.reserve(id.size());
    for (char c : id) lid.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));

    if (lid == "direct3d" || lid == "dx12" || lid == "d3d12" || lid == "d3d" || lid == "directx")
        return std::make_unique<RendererDX12Stub>();
    if (lid == "vulkan" || lid == "vulcan" || lid == "vk")
        return std::make_unique<RendererVulkanStub>();
    if (lid == "methane" || lid == "methanekit" || lid == "methane-kit" || lid == "mk")
        return std::make_unique<RendererMethaneStub>();

    return {};
}

} // namespace Engine::Renderer
