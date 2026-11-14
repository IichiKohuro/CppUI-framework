#pragma once

#include <windows.h>

#include <cstdint>
#include <string_view>
#include <unordered_map>

#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cppui/theme/tokens.hpp>
#include <cppui/ui/painter.hpp>

#include "chrome/chrome_state.hpp"

namespace cppui {

class Widget;

// Phase 1 renderer: D3D11 -> DXGI flip model -> Direct2D/DirectWrite.
// It deliberately exposes only the bootstrap needs; the retained-mode scene
// and public renderer API arrive in later phases.
class D2DRenderer final : public Painter {
public:
    D2DRenderer() = default;
    ~D2DRenderer();

    D2DRenderer(const D2DRenderer&) = delete;
    D2DRenderer& operator=(const D2DRenderer&) = delete;
    D2DRenderer(D2DRenderer&&) = delete;
    D2DRenderer& operator=(D2DRenderer&&) = delete;

    [[nodiscard]] bool Initialize(HWND hwnd, const ThemeTokens& theme);
    [[nodiscard]] bool Resize(UINT width, UINT height, float dpi);
    [[nodiscard]] bool Render(const ChromeState& chrome = {}, Widget* content = nullptr);
    [[nodiscard]] bool SetTheme(const ThemeTokens& theme);
    // Rebuilds the D3D device, swap chain, D2D device context and all resources.
    // Called after DEVICE_REMOVED/RESET and exposed for the render smoke test.
    [[nodiscard]] bool RecreateDevice();
    void Shutdown() noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
    [[nodiscard]] bool IsWarp() const noexcept { return using_warp_; }
    [[nodiscard]] HRESULT LastError() const noexcept { return last_error_; }

    [[nodiscard]] const ThemeTokens& theme() const noexcept override;
    [[nodiscard]] Size measure_text(std::wstring_view text, FontRole role) const override;
    void fill_rect(const Rect& rect, const Color& color) override;
    void fill_rounded_rect(const Rect& rect, float radius, const Color& color) override;
    void stroke_rounded_rect(const Rect& rect,
                             float radius,
                             const Color& color,
                             float width = 1.0f) override;
    void draw_text(std::wstring_view text,
                   const Rect& rect,
                   FontRole role,
                   const Color& color,
                   TextAlign align = TextAlign::leading,
                   TextVAlign vertical = TextVAlign::center) override;

private:
    bool BuildResources();
    bool CreateDeviceResources();
    bool CreateSwapChain();
    bool CreateTargetBitmap();
    bool CreateTextFormats();
    bool CreateBrushes();
    bool CreateSolidBrush(const Color& color, ID2D1SolidColorBrush** brush);
    [[nodiscard]] IDWriteTextFormat* TextFormatForRole(FontRole role) const noexcept;
    [[nodiscard]] ID2D1SolidColorBrush* BrushFor(const Color& color);

    void ReleaseTarget() noexcept;
    void ReleaseDeviceResources() noexcept;
    void DrawScene(const ChromeState& chrome, Widget* content) noexcept;

    HWND hwnd_{};
    ThemeTokens theme_{};
    UINT width_{};
    UINT height_{};
    float dpi_{96.0f};
    bool initialized_{false};
    bool using_warp_{false};
    HRESULT last_error_{S_OK};

    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain_;
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2d_factory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2d_device_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2d_context_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> target_bitmap_;

    Microsoft::WRL::ComPtr<IDWriteFactory> write_factory_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> title_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> subtitle_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> meta_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> pill_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> version_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> caption_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> badge_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> ui_caption_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> ui_body_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> ui_label_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> ui_heading_format_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> ui_title_format_;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_window_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_surface_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_surface_soft_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_surface_hover_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_border_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_primary_text_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_secondary_text_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_accent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_accent_soft_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_on_accent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_success_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_close_hover_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_on_close_;
    std::unordered_map<std::uint32_t, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> dynamic_brushes_;
};

}  // namespace cppui
