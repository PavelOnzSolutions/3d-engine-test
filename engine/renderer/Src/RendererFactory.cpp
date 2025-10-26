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
#include <cstring>

namespace Engine::Renderer {

namespace {

static Mat4 Identity4()
{
    Mat4 m{}; m.m[0]=1; m.m[5]=1; m.m[10]=1; m.m[15]=1; return m;
}

// Helper: convert UTF-8 string to wide string for Win32 APIs
static std::wstring Utf8ToWide(const char* utf8)
{
    if (!utf8) return L"";
    int len = static_cast<int>(strlen(utf8));
    if (len <= 0) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wstr(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, len, wstr.data(), wlen);
    return wstr;
}

// Minimal helper to draw a scene banner using Win32 GDI instead of rotating cube
static void DrawSceneBanner(HWND hwnd, const wchar_t* scene_path)
{
    if (!hwnd) return;
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) return;

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    // Clear to black
    HBRUSH hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(hdc, &rc, hbr);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(200, 240, 200));

    std::wstring line1 = L"Rendering GLB Scene";
    std::wstring line2 = scene_path && scene_path[0] ? scene_path : L"(no scene set)";

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    RECT rcText = rc;
    rcText.top += 20;
    DrawTextW(hdc, line1.c_str(), (int)line1.size(), &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE);
    rcText.top += 30;
    DrawTextW(hdc, line2.c_str(), (int)line2.size(), &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_PATH_ELLIPSIS);

    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);
}

// Global scene path used by stub banner rendering
static std::wstring g_scene_path;
// Redirect cube drawing to scene banner for all stubs
#define DrawWireCube(hwnd, angle_rad) DrawSceneBanner(hwnd, g_scene_path.c_str())

class RendererDX12Stub : public IRenderer {
public:
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
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
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
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
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
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
