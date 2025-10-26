#pragma once

namespace Engine::Renderer {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    // Optional: provide scene file path (.glb/.gltf) to renderer before or after Initialize.
    // Default no-op implementation keeps existing backends source-compatible.
    virtual void SetScenePath(const char* /*scene_path_utf8*/) {}

    virtual bool Initialize(void* windowHandle) = 0;
    virtual void RenderFrame() = 0;
    virtual void Shutdown() = 0;
};

} // namespace Engine::Renderer
