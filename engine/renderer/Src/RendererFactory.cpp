#include <Engine/Renderer/RendererFactory.h>
#include <Engine/Renderer/RenderContext.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/Core/Passes.h>
#include <Engine/Renderer/Scene.h>
#include <memory>
#include <string>
#include <cstdlib>
#include <cmath>
#include <windows.h>

namespace Engine::Renderer {

namespace {

static Mat4 Identity4()
{
    Mat4 m{}; m.m[0]=1; m.m[5]=1; m.m[10]=1; m.m[15]=1; return m;
}

// Minimal helper to draw a rotating wireframe cube using Win32 GDI
static void DrawWireCube(HWND hwnd, float angle_rad)
{
    if (!hwnd) return;
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) return;
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    // Clear to black
    HBRUSH hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(hdc, &rc, hbr);

    // Setup white pen
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
    HGDIOBJ old_pen = SelectObject(hdc, pen);

    // Simple 3D cube vertices (-1..1)
    struct Vec3f { float x,y,z; };
    Vec3f v[8] = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1}, // back face
        {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}  // front face
    };

    const float c = std::cos(angle_rad);
    const float s = std::sin(angle_rad);

    // Rotate around Y then X to make it interesting
    auto rot_apply = [&](const Vec3f& a){
        // Y rotation
        float x1 = a.x*c + a.z*s;
        float z1 = -a.x*s + a.z*c;
        float y1 = a.y;
        // X rotation (slower)
        float cx = std::cos(angle_rad*0.7f);
        float sx = std::sin(angle_rad*0.7f);
        float y2 = y1*cx - z1*sx;
        float z2 = y1*sx + z1*cx;
        return Vec3f{ x1, y2, z2 };
    };

    // Project to 2D (simple perspective)
    POINT pts[8];
    const float fov = 1.1f; // in radians, approx 63 deg
    const float f = 1.0f / std::tan(fov*0.5f);
    const float aspect = width > 0 ? (float)width / (float)height : 1.0f;
    const float z_cam = 3.0f; // camera distance

    for (int i=0;i<8;++i)
    {
        Vec3f r = rot_apply(v[i]);
        // move cube forward
        r.z += z_cam;
        float px = (r.x * f / aspect) / r.z;
        float py = (r.y * f) / r.z;
        int sx = (int)((px * 0.5f + 0.5f) * width);
        int sy = (int)((-py * 0.5f + 0.5f) * height);
        pts[i].x = rc.left + sx;
        pts[i].y = rc.top + sy;
    }

    auto line = [&](int a, int b){ MoveToEx(hdc, pts[a].x, pts[a].y, nullptr); LineTo(hdc, pts[b].x, pts[b].y); };
    // Draw edges
    line(0,1); line(1,2); line(2,3); line(3,0); // back
    line(4,5); line(5,6); line(6,7); line(7,4); // front
    line(0,4); line(1,5); line(2,6); line(3,7); // sides

    // Cleanup
    SelectObject(hdc, old_pen);
    DeleteObject(pen);
    ReleaseDC(hwnd, hdc);
}

class RendererDX12Stub : public IRenderer {
public:
    bool Initialize(void* windowHandle) override {
        m_hwnd = static_cast<HWND>(windowHandle);
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
        m_angle += 0.02f;
        DrawWireCube(m_hwnd, m_angle);

        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); m_hwnd = nullptr; }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
    HWND m_hwnd{nullptr};
    float m_angle{0.0f};
};

class RendererVulkanStub : public IRenderer {
public:
    bool Initialize(void* windowHandle) override {
        m_hwnd = static_cast<HWND>(windowHandle);
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
        m_angle += 0.02f;
        DrawWireCube(m_hwnd, m_angle);

        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); m_hwnd = nullptr; }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
    HWND m_hwnd{nullptr};
    float m_angle{0.0f};
};

class RendererMethaneStub : public IRenderer {
public:
    bool Initialize(void* windowHandle) override {
        m_hwnd = static_cast<HWND>(windowHandle);
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
        m_angle += 0.02f;
        DrawWireCube(m_hwnd, m_angle);

        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); m_hwnd = nullptr; }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
    HWND m_hwnd{nullptr};
    float m_angle{0.0f};
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
