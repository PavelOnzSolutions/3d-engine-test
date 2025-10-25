#include <windows.h>
#include <string>
#include <memory>
#include <Engine/Core/Config.h>
#include <Engine/Core/Logger.h>
#include <Engine/Renderer/RendererFactory.h>
#include <sstream>
#include <chrono>
#include <iomanip>

using Engine::Core::Config;
using Engine::Core::VideoConfig;
using Engine::Renderer::IRenderer;
using Engine::Renderer::CreateRenderer;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HBRUSH hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &ps.rcPaint, hbr);
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

    // Initial window title
    std::wstring title = L"3DEngineTest - FPS: 0";

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

    // Force-set initial title in case the system or frameworks override it
    if (!SetWindowTextW(hWnd, L"3DEngineTest - FPS: 0"))
    {
        DWORD err = GetLastError();
        Engine::Core::Logger::Info(std::string("SetWindowTextW initial title failed with error ") + std::to_string(static_cast<unsigned long>(err)));
    }

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
    int frame_count = 0;
    double current_fps = 0.0;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (renderer)
            renderer->RenderFrame();

        // FPS accounting
        ++frame_count;
        auto now = Clock::now();
        std::chrono::duration<double> elapsed = now - last_time;
        if (elapsed.count() >= 1.0)
        {
            current_fps = frame_count / elapsed.count();
            std::wstringstream wss;
            wss.setf(std::ios::fixed);
            wss << L"3DEngineTest - FPS: " << static_cast<int>(current_fps + 0.5);
            SetWindowTextW(hWnd, wss.str().c_str());

            // reset
            frame_count = 0;
            last_time = now;
        }
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