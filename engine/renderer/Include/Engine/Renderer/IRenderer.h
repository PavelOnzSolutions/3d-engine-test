#pragma once

namespace Engine::Renderer {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize(void* windowHandle) = 0;
    virtual void RenderFrame() = 0;
    virtual void Shutdown() = 0;
};

} // namespace Engine::Renderer
