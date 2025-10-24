#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/Core/IRenderPass.h>
#include <Engine/Renderer/RenderContext.h>

namespace Engine { namespace Renderer {

void FrameGraph::AddPass(const PassPtr& pass)
{
    m_passes.push_back(pass);
    m_compiled = false;
}

void FrameGraph::Compile()
{
    if (m_compiled)
        return;

    for (const auto& pass : m_passes)
    {
        if (pass)
            pass->Setup(*this);
    }
    m_compiled = true;
}

void FrameGraph::Execute(RenderContext& ctx)
{
    if (!m_compiled)
        Compile();

    for (const auto& pass : m_passes)
    {
        if (pass)
            pass->Execute(ctx);
    }
}

void FrameGraph::Clear()
{
    m_passes.clear();
    m_resources.clear();
    m_compiled = false;
}

}} // namespace Engine::Renderer
