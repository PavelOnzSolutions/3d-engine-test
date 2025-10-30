#pragma once

namespace Engine::Renderer {
    struct OverlayText {
        std::wstring text;
        int x; // pixel x (left or right depending on rightAlign)
        int y; // pixel y (top)
        uint32_t color; // 0xAARRGGBB or 0xRRGGBB
        int fontSize;
        bool rightAlign;
    };

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        // Optional: provide scene file path (.glb/.gltf) to renderer before or after Initialize.
        // Default no-op implementation keeps existing backends source-compatible.
        virtual void SetScenePath(const char * /*scene_path_utf8*/) {
        }

        // Optional: set a 4x4 model matrix (row-major, 16 floats). Default no-op.
        virtual void SetModelMatrix(const float * /*m16_row_major*/) {
        }

        virtual bool Initialize(void *windowHandle) = 0;

        virtual void RenderFrame() = 0;
        virtual void Shutdown() = 0;
        virtual void ClearOverlayTexts() = 0;
        virtual void AddOverlayText(const OverlayText& text) = 0;
    };
} // namespace Engine::Renderer
