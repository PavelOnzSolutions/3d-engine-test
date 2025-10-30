#define NOMINMAX
#include <windows.h>
#include <string>
#include <memory>
#include <Engine/Core/Config.h>
#include <Engine/Core/Logger.h>
#include <Engine/Renderer/RendererFactory.h>
#include <sstream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <entt/entt.hpp>
#include <Engine/Assets/SceneLoader.h>
#include "engine/ecs/Include/Components.h"
#include "engine/ecs/Include/Systems.h"

using Engine::Core::Config;
using Engine::Core::VideoConfig;
using Engine::Core::EngineConfig;
using Engine::Renderer::IRenderer;
using Engine::Renderer::CreateRenderer;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        // Prevent background erasing to avoid flicker and black clears between frames
        return 1;
    case WM_PAINT:
    {
        // Validate the paint without clearing the background; the renderer draws the frame
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        Engine::Core::Logger::Info("Window destroy requested. Posting quit message.");
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Load configuration from INI
    VideoConfig videoCfg; // defaults applied
    Config::LoadFromIni(videoCfg); // ignore return; defaults remain if file missing

    // Initialize logger from INI (with defaults if not set)
    Engine::Core::LoggingConfig logCfg; // defaults in struct
    Config::LoadLoggingFromIni(logCfg);
    Engine::Core::Logger::Initialize(logCfg);
    Engine::Core::Logger::Info("Application starting...");
    // Log config summary
    Engine::Core::Logger::Info(
        std::string("Video config: renderer=") + videoCfg.renderer +
        ", fullscreen=" + (videoCfg.fullscreen ? "true" : "false") +
        ", resolution=" + std::to_string(videoCfg.width) + "x" + std::to_string(videoCfg.height)
    );
    Engine::Core::Logger::Info(
        std::string("Logging config: MaxFileSizeMB=") + std::to_string(logCfg.max_file_size_mb) +
        ", MaxFileCount=" + std::to_string(logCfg.max_file_count)
    );

    // Load engine directories and startup scene
    EngineConfig engineCfg;
    Config::LoadEngineFromIni(engineCfg);
    Engine::Core::Logger::Info(std::string("Engine directories: models=") + engineCfg.dirModels +
                               ", textures=" + engineCfg.dirTextures +
                               ", sounds=" + engineCfg.dirSounds +
                               ", scenes=" + engineCfg.dirScenes);
    if (!engineCfg.startupScene.empty())
        Engine::Core::Logger::Info(std::string("StartupScene=") + engineCfg.startupScene);
    else
        Engine::Core::Logger::Info("StartupScene not specified; will continue with an empty scene");

    // Attempt to load startup scene using stub loader
    Engine::Assets::LoadedScene loadedScene{};
    std::vector<std::string> loadWarnings;
    if (!engineCfg.startupScene.empty())
    {
        Engine::Assets::LoadScene(engineCfg, engineCfg.startupScene, loadedScene, loadWarnings);
        for (const auto& w : loadWarnings)
            Engine::Core::Logger::Info(std::string("[SceneLoader] ") + w);
    }

    const wchar_t CLASS_NAME[] = L"EngineWindowClass";

    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));

    RegisterClassW(&wc);

    // Window style based on fullscreen
    DWORD style = videoCfg.fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;

    int width = videoCfg.width;
    int height = videoCfg.height;
    if (videoCfg.fullscreen)
    {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    // Initial window title (static; FPS moved into viewport overlay)
    std::wstring title = L"3DEngineTest";

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
    {
        DWORD err = GetLastError();
        Engine::Core::Logger::Error(std::string("CreateWindowExW failed with error ") + std::to_string(static_cast<unsigned long>(err)));
        return -1;
    }

    ShowWindow(hWnd, videoCfg.fullscreen ? SW_SHOWMAXIMIZED : nCmdShow);
    UpdateWindow(hWnd);

    // Ensure static title is set
    SetWindowTextW(hWnd, L"3DEngineTest");

    // Create renderer based on INI setting
    Engine::Core::Logger::Info(std::string("Requested renderer: ") + videoCfg.renderer);
    std::unique_ptr<IRenderer> renderer = CreateRenderer(videoCfg.renderer);
    std::string activeRenderer = videoCfg.renderer;
    if (!renderer)
    {
        // Fallback to direct3d if unknown
        Engine::Core::Logger::Error("Unknown renderer requested. Falling back to direct3d.");
        renderer = CreateRenderer("direct3d");
        activeRenderer = "direct3d";
    }

    if (renderer)
    {
        // Provide scene path (if any) to the renderer prior to initialization.
        if (!loadedScene.scene_path.empty())
            renderer->SetScenePath(loadedScene.scene_path.c_str());

        Engine::Core::Logger::Info(std::string("Initializing renderer backend: ") + activeRenderer + "...");
        bool init_ok = renderer->Initialize(static_cast<void*>(hWnd));
        if (!init_ok)
        {
            Engine::Core::Logger::Error(std::string("Renderer initialization failed for backend: ") + activeRenderer);
            renderer.reset();
        }
        else
        {
            Engine::Core::Logger::Info(std::string("Renderer initialized successfully: ") + activeRenderer);
        }
    }

    // Basic message loop
    Engine::Core::Logger::Info("Entering message loop...");

    using Clock = std::chrono::steady_clock;
    auto last_time = Clock::now();
    auto last_frame_time = Clock::now();
    int frame_count = 0;
    double current_fps = 0.0;
    std::wstring fps_text = L"FPS: 0";

    // ECS world setup
    entt::registry registry;
    const auto entity = registry.create();
    registry.emplace<Transform>(entity, Transform{});
    registry.emplace<Velocity>(entity, Velocity{ glm::vec3(1.f, 0.f, 0.f) }); // move along +X at 1 unit/sec

    // Instantiate additional entities based on loaded scene stub (excluding the first one we already created)
    if (loadedScene.nodes_count > 1)
    {
        entt::entity parent = entity;
        for (int i = 1; i < loadedScene.nodes_count; ++i)
        {
            auto e = registry.create();
            registry.emplace<Transform>(e, Transform{});
            registry.emplace<Parent>(e, Parent{ parent });
            // Attach a MeshRenderer with placeholder indices
            registry.emplace<MeshRenderer>(e, MeshRenderer{ i - 1, i - 1 });
            parent = e; // chain as a simple linear hierarchy for demo
        }
    }

    MSG msg{};
    bool running = true;
    while (running)
    {
        // Process all pending window messages without blocking
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running)
            break;

        // Per-frame delta time
        auto frame_now = Clock::now();
        std::chrono::duration<float> frame_dt = frame_now - last_frame_time;
        last_frame_time = frame_now;

        // Update ECS systems
        UpdateMovement(registry, std::max(0.0f, frame_dt.count()));

        // Render a frame
        if (renderer)
        {
            // Provide model matrix from ECS (for primary entity)
            if (registry.valid(entity))
            {
                const auto &t = registry.get<Transform>(entity);
                float m[16] = {
                    1,0,0,0,
                    0,1,0,0,
                    0,0,1,0,
                    t.position.x, t.position.y, t.position.z, 1
                };
                renderer->SetModelMatrix(m);
            }

            // Prepare overlays via renderer API (must be rendered by renderer before Present)
            RECT rc{}; GetClientRect(hWnd, &rc);
            constexpr int padding = 10;
            renderer->ClearOverlayTexts();

            // FPS top-right (right-aligned)
            Engine::Renderer::OverlayText fpsOverlay;
            fpsOverlay.text = fps_text;
            fpsOverlay.fontSize = 16;
            fpsOverlay.color = 0x00FFFFFF; // white (alpha handled by renderer)
            fpsOverlay.rightAlign = true;
            // place near top-right; renderer should respect rightAlign
            fpsOverlay.x = rc.right - padding - 200; // x baseline region
            fpsOverlay.y = rc.top + padding;
            renderer->AddOverlayText(fpsOverlay);

            // Scene-not-found at top-left (if applicable)
            if (loadedScene.nodes_count == 0 && !loadedScene.scene_path.empty())
            {
                std::wstring ws;
                int wlen = MultiByteToWideChar(CP_UTF8, 0, loadedScene.scene_path.c_str(), static_cast<int>(loadedScene.scene_path.size()), nullptr, 0);
                ws.resize(static_cast<size_t>(wlen));
                MultiByteToWideChar(CP_UTF8, 0, loadedScene.scene_path.c_str(), static_cast<int>(loadedScene.scene_path.size()), ws.data(), wlen);

                Engine::Renderer::OverlayText sceneOverlay;
                sceneOverlay.text = std::wstring(L"Scene not found: ") + ws;
                sceneOverlay.fontSize = 14;
                sceneOverlay.color = 0x00FFFFFF;
                sceneOverlay.rightAlign = false;
                sceneOverlay.x = rc.left + padding;
                sceneOverlay.y = rc.top + padding;
                renderer->AddOverlayText(sceneOverlay);
            }

            // Now the renderer will draw scene + overlay and present
            renderer->RenderFrame();
        }

        // FPS accounting
        ++frame_count;
        auto now = Clock::now();
        std::chrono::duration<double> elapsed = now - last_time;
        if (elapsed.count() >= 1.0)
        {
            current_fps = frame_count / elapsed.count();
            // Update on-screen FPS text once per second
            std::wstringstream wss;
            wss.setf(std::ios::fixed);
            wss << L"FPS: " << static_cast<int>(lround(current_fps + 0.5));
            fps_text = wss.str();

            // Debug: report entity position to log
#ifdef _DEBUG
            if (registry.valid(entity))
            {
                const auto &t = registry.get<Transform>(entity);
                std::ostringstream oss;
                oss.setf(std::ios::fixed);
                oss.precision(2);
                oss << "ECS Transform: x=" << t.position.x << ", y=" << t.position.y << ", z=" << t.position.z;
                Engine::Core::Logger::Info(oss.str());
            }
#endif

            // reset
            frame_count = 0;
            last_time = now;
        }

        // Small sleep to avoid 100% CPU on very fast loops
        Sleep(1);
    }
    Engine::Core::Logger::Info(std::string("Message loop exited with code ") + std::to_string(static_cast<int>(msg.wParam)));

    if (renderer)
    {
        Engine::Core::Logger::Info("Shutting down renderer...");
        renderer->Shutdown();
        renderer.reset();
        Engine::Core::Logger::Info("Renderer shut down.");
    }

    Engine::Core::Logger::Info("Application exiting.");
    return static_cast<int>(msg.wParam);
}