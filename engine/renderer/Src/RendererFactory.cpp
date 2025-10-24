#include <Engine/Renderer/RendererFactory.h>
#include <Engine/Renderer/RenderContext.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/Core/Passes.h>
#include <memory>
#include <string>

namespace Engine::Renderer {

namespace {

class RendererDX12Stub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();
        // Build deferred rendering pipeline: Shadows -> G-Buffer -> Deferred Lighting -> Post -> Present
        m_frame_graph->AddPass(std::make_shared<ShadowPass>());
        m_frame_graph->AddPass(std::make_shared<GBufferPass>());
        m_frame_graph->AddPass(std::make_shared<DeferredLightingPass>());
        m_frame_graph->AddPass(std::make_shared<PostProcessPass>());
        m_frame_graph->AddPass(std::make_shared<PresentPass>());
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
};

class RendererVulkanStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();
        // Build deferred rendering pipeline: Shadows -> G-Buffer -> Deferred Lighting -> Post -> Present
        m_frame_graph->AddPass(std::make_shared<ShadowPass>());
        m_frame_graph->AddPass(std::make_shared<GBufferPass>());
        m_frame_graph->AddPass(std::make_shared<DeferredLightingPass>());
        m_frame_graph->AddPass(std::make_shared<PostProcessPass>());
        m_frame_graph->AddPass(std::make_shared<PresentPass>());
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
};

class RendererMethaneStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        // Create a dummy RenderContext; real integration will pass Methane RHI objects
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();
        // Build deferred rendering pipeline: Shadows -> G-Buffer -> Deferred Lighting -> Post -> Present
        m_frame_graph->AddPass(std::make_shared<ShadowPass>());
        m_frame_graph->AddPass(std::make_shared<GBufferPass>());
        m_frame_graph->AddPass(std::make_shared<DeferredLightingPass>());
        m_frame_graph->AddPass(std::make_shared<PostProcessPass>());
        m_frame_graph->AddPass(std::make_shared<PresentPass>());
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
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
