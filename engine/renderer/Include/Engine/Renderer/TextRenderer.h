#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Engine::Renderer {
    struct ColorU32 { uint32_t rgba; }; // 0xAARRGGBB or 0xRRGGBB
    struct TextLayoutResult { float width; float height; };

    struct DrawTextParams {
        std::string utf8;      // UTF-8 text
        float x, y;            // top-left baseline in pixels
        int fontSize;          // px
        ColorU32 color;
        bool rightAlign{false};
        // optional: maxWidth, wrap, truncation, clipping rect, etc.
    };

    class TextRenderer {
    public:
        virtual ~TextRenderer() = default;

        // Must be called after GPU device/context is available
        virtual bool Initialize(void* nativeWindowHandle) = 0;
        virtual void Shutdown() = 0;

        // Load or get font; returns font id handle (opaque)
        virtual int LoadFontFromFile(const std::string& pathUtf8, const std::string& family) = 0;
        virtual void UnloadFont(int fontId) = 0;

        // Frame lifecycle: Begin/End let impls prepare command buffers / map staging buffers
        virtual void BeginFrame(uint32_t fbWidth, uint32_t fbHeight) = 0;

        // High-level API: queue text for this frame (cheap operation)
        virtual void DrawText(const DrawTextParams& params, int fontId = 0) = 0;

        // Issue GPU draws; should be called after main scene draw but before present
        virtual void FlushAndRender(void* perFrameCmdListOrCtx) = 0;

        // Utilities
        virtual TextLayoutResult MeasureText(const std::string& utf8, int fontSize, int fontId) = 0;

        // Called when device is lost or swapchain/resolution changes
        virtual void OnResize(uint32_t newWidth, uint32_t newHeight) = 0;
    };
}