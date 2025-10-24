#pragma once

#include <memory>
#include <cstdint>

// This header provides a small cross-backend RenderContext abstraction that can
// wrap MethaneKit's command buffers (command lists) and descriptor sets (program bindings)
// when MethaneKit is available. Otherwise it compiles to a no-op placeholder so other
// backends are not affected.

#if defined(ENGINE_USE_METHANEKIT)
// Forward-declare Methane RHI interface types to avoid pulling heavy headers here
namespace Methane { namespace Graphics { namespace Rhi {
    class IRenderContext;
    class IRenderCommandList;
    class IProgramBindings;
} } }
#endif

namespace Engine::Renderer {

class RenderContext {
public:
    RenderContext() = default;

#if defined(ENGINE_USE_METHANEKIT)
    using MethaneRenderContext = Methane::Graphics::Rhi::IRenderContext;
    using CommandList = Methane::Graphics::Rhi::IRenderCommandList;
    using ProgramBindings = Methane::Graphics::Rhi::IProgramBindings;

    // Construct wrapper around existing Methane RHI objects
    RenderContext(std::shared_ptr<MethaneRenderContext> render_ctx,
                  std::shared_ptr<CommandList> cmd_list = {},
                  std::shared_ptr<ProgramBindings> program_bindings = {})
        : m_methane_render_ctx(std::move(render_ctx))
        , m_cmd_list(std::move(cmd_list))
        , m_program_bindings(std::move(program_bindings))
    {}

    void SetCommandList(std::shared_ptr<CommandList> cmd_list) { m_cmd_list = std::move(cmd_list); }
    void SetProgramBindings(std::shared_ptr<ProgramBindings> bindings) { m_program_bindings = std::move(bindings); }

    const std::shared_ptr<MethaneRenderContext>& GetMethaneRenderContext() const { return m_methane_render_ctx; }
    const std::shared_ptr<CommandList>& GetCommandList() const { return m_cmd_list; }
    const std::shared_ptr<ProgramBindings>& GetProgramBindings() const { return m_program_bindings; }

    // Minimal frame lifecycle helpers (do not throw, safe to call when pointers are null)
    void BeginFrame() noexcept { m_frame_started = true; }
    void EndFrame() noexcept { m_frame_started = false; }

    bool IsValid() const noexcept { return static_cast<bool>(m_methane_render_ctx); }
    bool IsFrameRecording() const noexcept { return m_frame_started; }
#else
    // Stubs when MethaneKit is not enabled in the build
    void BeginFrame() noexcept { m_frame_started = true; }
    void EndFrame() noexcept { m_frame_started = false; }
    bool IsValid() const noexcept { return false; }
    bool IsFrameRecording() const noexcept { return m_frame_started; }
#endif

private:
#if defined(ENGINE_USE_METHANEKIT)
    std::shared_ptr<MethaneRenderContext> m_methane_render_ctx;
    std::shared_ptr<CommandList>          m_cmd_list;
    std::shared_ptr<ProgramBindings>      m_program_bindings;
#endif
    bool m_frame_started{false};
};

} // namespace Engine::Renderer
