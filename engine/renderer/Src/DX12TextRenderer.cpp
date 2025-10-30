#include <Engine/Renderer/TextRenderer.h>
#include <wrl/client.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sstream>
#include <windows.h>

#include <Engine/Utils/StringUtils.h>

using Microsoft::WRL::ComPtr;

namespace Engine::Renderer {
    class DX12TextRenderer : public TextRenderer {
    public:
        DX12TextRenderer() = default;
        ~DX12TextRenderer() override { Shutdown(); }



        void Shutdown() override {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_queue.clear();
            m_text_formats.clear();
            m_font_registry.clear();
            m_dwrite_factory.Reset();
            m_d2d_factory.Reset();
        }

    private:
        struct DrawItem {
            DrawTextParams params;
            int fontId{0};
        };

        ComPtr<ID2D1Factory1> m_d2d_factory;
        ComPtr<ID2D1HwndRenderTarget> m_d2d_rt;
        ComPtr<IDWriteFactory> m_dwrite_factory;

        std::mutex m_mutex;
        std::map<int, std::string> m_font_registry; // fontId -> family name
        int m_next_font_id{1};

        // cache by (fontId,fontSize)
        std::map<std::pair<int,int>, ComPtr<IDWriteTextFormat>> m_text_formats;

        std::vector<DrawItem> m_queue;
        uint32_t m_fb_width{800}, m_fb_height{600};
        HWND m_hwnd{};

        ComPtr<IDWriteTextFormat> GetTextFormat(int fontId, int fontSize) {
            std::pair<int,int> key{fontId, fontSize};
            auto it = m_text_formats.find(key);
            if (it != m_text_formats.end()) return it->second;

            std::string family = "Segoe UI";
            auto fit = m_font_registry.find(fontId);
            if (fit != m_font_registry.end()) family = fit->second;

            ComPtr<IDWriteTextFormat> fmt;
            const std::wstring wfamily = Utils::StringUtils::Utf8ToWide(family);
            if (FAILED(m_dwrite_factory->CreateTextFormat(
                    wfamily.c_str(),
                    nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    static_cast<FLOAT>(fontSize),
                    L"", // locale
                    &fmt)))
            {
                return nullptr;
            }
            // Left align by default; wrapping is not set here
            fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            m_text_formats.emplace(key, fmt);
            return fmt;
        }
    };
}