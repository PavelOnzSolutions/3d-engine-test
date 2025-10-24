#include <Engine/Renderer/RendererFactory.h>
#include <Engine/Renderer/RenderContext.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/Core/Passes.h>
#include <Engine/Renderer/Scene.h>
#include <memory>
#include <string>
#include <cstdlib>

namespace Engine::Renderer {

namespace {

static Mat4 Identity4()
{
    Mat4 m{}; m.m[0]=1; m.m[5]=1; m.m[10]=1; m.m[15]=1; return m;
}

class RendererDX12Stub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();

        // Create a trivial scene, camera and output draw list owned by renderer
        m_scene = std::make_unique<Scene>();
        m_camera = std::make_unique<Camera>();
        m_culled = std::make_unique<std::vector<DrawCall>>();
        m_camera->SetViewProjection(Identity4(), Identity4());
        // Add one dummy renderable so culling produces a draw
        Renderable r; r.mesh_id = 0; r.material_id = 0; r.model = Identity4();
        r.world_bounds = { { -1,-1,-1 }, { 1,1,1 } };
        m_scene->renderables.push_back(r);
        // Bind resources for passes
        m_frame_graph->SetResource(SceneResources::ScenePtr, m_scene.get());
        m_frame_graph->SetResource(SceneResources::CameraPtr, m_camera.get());
        m_frame_graph->SetResource(SceneResources::CulledDrawsPtr, m_culled.get());

        // Choose pipeline based on environment variable ENGINE_FORWARD_PLUS (1/true/yes to enable)
        const bool use_forward_plus = []{
            if (const char* env = std::getenv("ENGINE_FORWARD_PLUS"))
                return env[0] == '1' || env[0] == 't' || env[0] == 'T' || env[0] == 'y' || env[0] == 'Y';
            return false;
        }();
        if (use_forward_plus)
            BuildForwardPlusPipeline(*m_frame_graph);
        else
            BuildDeferredPipeline(*m_frame_graph);
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
};

class RendererVulkanStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();

        m_scene = std::make_unique<Scene>();
        m_camera = std::make_unique<Camera>();
        m_culled = std::make_unique<std::vector<DrawCall>>();
        m_camera->SetViewProjection(Identity4(), Identity4());
        Renderable r; r.mesh_id = 0; r.material_id = 0; r.model = Identity4();
        r.world_bounds = { { -1,-1,-1 }, { 1,1,1 } };
        m_scene->renderables.push_back(r);
        m_frame_graph->SetResource(SceneResources::ScenePtr, m_scene.get());
        m_frame_graph->SetResource(SceneResources::CameraPtr, m_camera.get());
        m_frame_graph->SetResource(SceneResources::CulledDrawsPtr, m_culled.get());

        // Choose pipeline based on environment variable ENGINE_FORWARD_PLUS (1/true/yes to enable)
        const bool use_forward_plus = []{
            if (const char* env = std::getenv("ENGINE_FORWARD_PLUS"))
                return env[0] == '1' || env[0] == 't' || env[0] == 'T' || env[0] == 'y' || env[0] == 'Y';
            return false;
        }();
        if (use_forward_plus)
            BuildForwardPlusPipeline(*m_frame_graph);
        else
            BuildDeferredPipeline(*m_frame_graph);
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
};

class RendererMethaneStub : public IRenderer {
public:
    bool Initialize(void* /*windowHandle*/) override {
        // Create a dummy RenderContext; real integration will pass Methane RHI objects
        m_ctx = std::make_unique<RenderContext>();
        m_frame_graph = std::make_unique<FrameGraph>();

        m_scene = std::make_unique<Scene>();
        m_camera = std::make_unique<Camera>();
        m_culled = std::make_unique<std::vector<DrawCall>>();
        m_camera->SetViewProjection(Identity4(), Identity4());
        Renderable r; r.mesh_id = 0; r.material_id = 0; r.model = Identity4();
        r.world_bounds = { { -1,-1,-1 }, { 1,1,1 } };
        m_scene->renderables.push_back(r);
        m_frame_graph->SetResource(SceneResources::ScenePtr, m_scene.get());
        m_frame_graph->SetResource(SceneResources::CameraPtr, m_camera.get());
        m_frame_graph->SetResource(SceneResources::CulledDrawsPtr, m_culled.get());

        // Choose pipeline based on environment variable ENGINE_FORWARD_PLUS (1/true/yes to enable)
        const bool use_forward_plus = []{
            if (const char* env = std::getenv("ENGINE_FORWARD_PLUS"))
                return env[0] == '1' || env[0] == 't' || env[0] == 'T' || env[0] == 'y' || env[0] == 'Y';
            return false;
        }();
        if (use_forward_plus)
            BuildForwardPlusPipeline(*m_frame_graph);
        else
            BuildDeferredPipeline(*m_frame_graph);
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
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
