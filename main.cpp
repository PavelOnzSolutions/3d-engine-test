#include <windows.h>
#include <string>
#include <memory>
#include <Engine/Core/Config.h>
#include <Engine/Core/Logger.h>
#include <Engine/Renderer/RendererFactory.h>

using Engine::Core::Config;
using Engine::Core::VideoConfig;
using Engine::Renderer::IRenderer;
using Engine::Renderer::CreateRenderer;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
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

    const wchar_t CLASS_NAME[] = L"EngineWindowClass";

    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

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

    // Title reflects renderer choice
    std::wstring title = L"3D Engine - ";
    std::wstring wrenderer(videoCfg.renderer.begin(), videoCfg.renderer.end());
    title += wrenderer + L" renderer";

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
        return -1;

    ShowWindow(hWnd, videoCfg.fullscreen ? SW_SHOWMAXIMIZED : nCmdShow);
    UpdateWindow(hWnd);

    // Create renderer based on INI setting
    std::unique_ptr<IRenderer> renderer = CreateRenderer(videoCfg.renderer);
    if (!renderer)
    {
        // Fallback to direct3d if unknown
        Engine::Core::Logger::Error("Unknown renderer requested. Falling back to direct3d.");
        renderer = CreateRenderer("direct3d");
    }

    if (renderer)
    {
        Engine::Core::Logger::Info("Initializing renderer...");
        renderer->Initialize(static_cast<void*>(hWnd));
    }

    // Basic message loop
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (renderer)
            renderer->RenderFrame();
    }

    if (renderer)
    {
        renderer->Shutdown();
        renderer.reset();
    }

    return static_cast<int>(msg.wParam);
}