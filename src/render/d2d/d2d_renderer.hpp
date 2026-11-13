#pragma once

#include <windows.h>

#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cppui/theme/tokens.hpp>

namespace cppui {

// Phase 0 renderer: D3D11 -> DXGI flip model -> Direct2D/DirectWrite.
// It deliberately exposes only the bootstrap needs; the retained-mode scene
// and public renderer API arrive in later phases.
class D2DRenderer final {
public:
    D2DRenderer() = default;
    ~D2DRenderer();

    D2DRenderer(const D2DRenderer&) = delete;
    D2DRenderer& operator=(const D2DRenderer&) = delete;
    D2DRenderer(D2DRenderer&&) = delete;
    D2DRenderer& operator=(D2DRenderer&&) = delete;

    [[nodiscard]] bool Initialize(HWND hwnd, const ThemeTokens& theme);
    [[nodiscard]] bool Resize(UINT width, UINT height, float dpi);
    [[nodiscard]] bool Render();
    void Shutdown() noexcept;

    [[nodiscard]] HRESULT LastError() const noexcept { return last_error_; }

private:
    bool CreateDeviceResources();
    bool CreateSwapChain();
    bool CreateTargetBitmap();
    bool CreateTextFormats();
    bool CreateBrushes();
    bool CreateSolidBrush(const Color& color, ID2D1SolidColorBrush** brush);

    void ReleaseTarget() noexcept;
    void ReleaseDeviceResources() noexcept;
    void DrawScene() noexcept;

    HWND hwnd_{};
    ThemeTokens theme_{};
    UINT width_{};
    UINT height_{};
    float dpi_{96.0f};
    bool initialized_{false};
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

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_window_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_surface_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_border_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_primary_text_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_secondary_text_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_accent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_accent_soft_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_on_accent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_success_;
};

}  // namespace cppui
