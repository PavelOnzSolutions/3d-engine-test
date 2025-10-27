#include <Engine/Renderer/RenderContext.h>

#if defined(ENGINE_USE_METHANEKIT)
#include <Methane/Graphics/RHI/ISystem.h>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Platform/Windows/AppEnvironment.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Memory.hpp>
#include <taskflow/taskflow.hpp>
#include <windows.h>

namespace Engine::Renderer {

// Keep single executor alive across contexts in this module
static std::unique_ptr<tf::Executor> g_executor;

bool RenderContext::InitializeFromWindowHandle(void* hwnd, uint32_t width, uint32_t height) noexcept
{
    try
    {
        if (!g_executor)
            g_executor = std::make_unique<tf::Executor>();

        Methane::Platform::AppEnvironment app_env{};
        app_env.window_handle = reinterpret_cast<HWND>(hwnd);

        auto& system = Methane::Graphics::Rhi::ISystem::Get();
        const auto& devices = system.UpdateGpuDevices(app_env);
        if (devices.empty())
            return false;

        Methane::Graphics::Rhi::IRenderContext::Settings settings{};
        settings.frame_size = Methane::Graphics::Rhi::FrameSize{ width, height };
        settings.color_format = Methane::Graphics::Rhi::PixelFormat::BGRA8Unorm;
        settings.vsync_enabled = true;
        settings.is_full_screen = false;
        settings.clear_color = Methane::Opt<Methane::Graphics::Color4F>(Methane::Graphics::Color4F(0.10f, 0.10f, 0.12f, 1.0f));

        auto render_ctx_ptr = Methane::Graphics::Rhi::IRenderContext::Create(app_env, *devices.front(), *g_executor, settings);
        m_methane_render_ctx = std::shared_ptr<MethaneRenderContext>(std::move(render_ctx_ptr));
        return static_cast<bool>(m_methane_render_ctx);
    }
    catch(...)
    {
        return false;
    }
}

void RenderContext::Present() noexcept
{
    if (m_methane_render_ctx)
    {
        try { m_methane_render_ctx->Present(); }
        catch(...) { /* swallow in noexcept */ }
    }
}

} // namespace Engine::Renderer

#else
// ENGINE_USE_METHANEKIT not defined: provide empty translation unit to satisfy build
namespace Engine::Renderer {}
#endif
