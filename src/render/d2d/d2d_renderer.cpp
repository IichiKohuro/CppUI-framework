#include "render/d2d/d2d_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>
#include <utility>

#include <d2d1helper.h>

#include <cppui/ui/widget.hpp>

namespace cppui {
namespace {

constexpr D2D1_COLOR_F ToColorF(const Color& color) noexcept {
    return D2D1_COLOR_F{
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f,
    };
}

bool CreateTextFormat(IDWriteFactory* factory,
                      const wchar_t* family,
                      float size,
                      DWRITE_FONT_WEIGHT weight,
                      DWRITE_TEXT_ALIGNMENT alignment,
                      DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment,
                      IDWriteTextFormat** output) {
    IDWriteTextFormat* format = nullptr;

    HRESULT hr = factory->CreateTextFormat(family,
                                           nullptr,
                                           weight,
                                           DWRITE_FONT_STYLE_NORMAL,
                                           DWRITE_FONT_STRETCH_NORMAL,
                                           size,
                                           L"en-us",
                                           &format);
    if (FAILED(hr)) {
        return false;
    }

    format->SetTextAlignment(alignment);
    format->SetParagraphAlignment(paragraph_alignment);
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    *output = format;
    return true;
}

bool IsDeviceLost(HRESULT hr) noexcept {
    return hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET ||
           hr == DXGI_ERROR_DEVICE_HUNG;
}

void TraceHresult(const wchar_t* operation, HRESULT hr) noexcept {
    wchar_t buffer[192]{};
    swprintf_s(buffer, L"CppUI: %s failed (0x%08X)\n", operation, static_cast<unsigned int>(hr));
    OutputDebugStringW(buffer);
}

bool ForceWarpFromEnvironment() noexcept {
    wchar_t value[8]{};
    const DWORD length = GetEnvironmentVariableW(L"CPPUI_FORCE_WARP", value, ARRAYSIZE(value));
    return length > 0 && value[0] == L'1';
}

}  // namespace

D2DRenderer::~D2DRenderer() {
    Shutdown();
}

bool D2DRenderer::Initialize(HWND hwnd, const ThemeTokens& theme) {
    if (initialized_) {
        Shutdown();
    }

    hwnd_ = hwnd;
    theme_ = theme;
    last_error_ = S_OK;
    using_warp_ = false;

    RECT client{};
    if (!GetClientRect(hwnd_, &client)) {
        last_error_ = HRESULT_FROM_WIN32(GetLastError());
        return false;
    }

    width_ = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
    height_ = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));

    dpi_ = static_cast<float>(GetDpiForWindow(hwnd_));
    if (dpi_ <= 0.0f) {
        dpi_ = 96.0f;
    }

    if (!BuildResources()) {
        Shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

bool D2DRenderer::BuildResources() {
    return CreateDeviceResources() && CreateSwapChain() && CreateTextFormats() &&
           CreateBrushes() && CreateTargetBitmap();
}

bool D2DRenderer::RecreateDevice() {
    if (hwnd_ == nullptr || IsWindow(hwnd_) == FALSE) {
        return false;
    }

    const HWND hwnd = hwnd_;
    const ThemeTokens theme = theme_;
    const UINT width = std::max<UINT>(1, width_);
    const UINT height = std::max<UINT>(1, height_);
    const float dpi = dpi_;

    initialized_ = false;
    ReleaseDeviceResources();

    hwnd_ = hwnd;
    theme_ = theme;
    width_ = width;
    height_ = height;
    dpi_ = dpi;
    last_error_ = S_OK;

    if (!BuildResources()) {
        TraceHresult(L"device recreation", last_error_);
        ReleaseDeviceResources();
        initialized_ = false;
        return false;
    }

    initialized_ = true;
    return true;
}

bool D2DRenderer::CreateDeviceResources() {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    const D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL selected_feature_level{};
    HRESULT hr = E_FAIL;
    bool hardware_created = false;

    if (!ForceWarpFromEnvironment()) {
        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_HARDWARE,
                               nullptr,
                               flags,
                               feature_levels,
                               ARRAYSIZE(feature_levels),
                               D3D11_SDK_VERSION,
                               d3d_device_.GetAddressOf(),
                               &selected_feature_level,
                               d3d_context_.GetAddressOf());
        hardware_created = SUCCEEDED(hr);
    }

    if (!hardware_created) {
        d3d_device_.Reset();
        d3d_context_.Reset();

        // The WARP fallback keeps the framework usable in VMs, over RDP and on
        // machines without a usable hardware adapter.
        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_WARP,
                               nullptr,
                               flags,
                               feature_levels,
                               ARRAYSIZE(feature_levels),
                               D3D11_SDK_VERSION,
                               d3d_device_.GetAddressOf(),
                               &selected_feature_level,
                               d3d_context_.GetAddressOf());
        using_warp_ = true;
        if (SUCCEEDED(hr)) {
            OutputDebugStringW(L"CppUI: using WARP renderer.\n");
        }
    } else {
        using_warp_ = false;
        OutputDebugStringW(L"CppUI: using hardware renderer.\n");
    }

    if (FAILED(hr)) {
        last_error_ = hr;
        using_warp_ = false;
        return false;
    }

    ID2D1Factory1* factory = nullptr;
    D2D1_FACTORY_OPTIONS factory_options{};
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                           __uuidof(ID2D1Factory1),
                           &factory_options,
                           reinterpret_cast<void**>(&factory));
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }
    d2d_factory_.Attach(factory);

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    hr = d3d_device_.As(&dxgi_device);
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    hr = d2d_factory_->CreateDevice(dxgi_device.Get(), d2d_device_.GetAddressOf());
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    hr = d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                          d2d_context_.GetAddressOf());
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    IDWriteFactory* write_factory = nullptr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                             __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(&write_factory));
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }
    write_factory_.Attach(write_factory);

    return true;
}

bool D2DRenderer::CreateSwapChain() {
    if (!d3d_device_ || width_ == 0 || height_ == 0) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = d3d_device_.As(&dxgi_device);
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(adapter.GetAddressOf());
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory;
    hr = adapter->GetParent(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = width_;
    description.Height = height_;
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.Stereo = FALSE;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    description.Flags = 0;

    hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device_.Get(),
                                              hwnd_,
                                              &description,
                                              nullptr,
                                              nullptr,
                                              swap_chain_.GetAddressOf());
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    (void)dxgi_factory->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER);
    return true;
}

bool D2DRenderer::CreateTargetBitmap() {
    if (!swap_chain_ || !d2d_context_) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf()));
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    D2D1_BITMAP_PROPERTIES1 properties{};
    properties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
    properties.dpiX = dpi_;
    properties.dpiY = dpi_;
    properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    hr = d2d_context_->CreateBitmapFromDxgiSurface(surface.Get(),
                                                   &properties,
                                                   target_bitmap_.GetAddressOf());
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }

    d2d_context_->SetTarget(target_bitmap_.Get());
    d2d_context_->SetDpi(dpi_, dpi_);
    return true;
}

bool D2DRenderer::CreateTextFormats() {
    if (!write_factory_) {
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          34.0f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          title_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          14.0f,
                          DWRITE_FONT_WEIGHT_NORMAL,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          subtitle_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          11.0f,
                          DWRITE_FONT_WEIGHT_NORMAL,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          meta_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          10.0f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                          pill_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          10.0f,
                          DWRITE_FONT_WEIGHT_NORMAL,
                          DWRITE_TEXT_ALIGNMENT_TRAILING,
                          DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                          version_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          12.5f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                          caption_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          8.5f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_CENTER,
                          DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                          badge_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          10.0f,
                          DWRITE_FONT_WEIGHT_NORMAL,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          ui_caption_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          13.0f,
                          DWRITE_FONT_WEIGHT_NORMAL,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          ui_body_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          12.0f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          ui_label_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          20.0f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          ui_heading_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    if (!CreateTextFormat(write_factory_.Get(),
                          L"Segoe UI",
                          28.0f,
                          DWRITE_FONT_WEIGHT_SEMI_BOLD,
                          DWRITE_TEXT_ALIGNMENT_LEADING,
                          DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                          ui_title_format_.GetAddressOf())) {
        last_error_ = E_FAIL;
        return false;
    }

    return true;
}

bool D2DRenderer::CreateBrushes() {
    return CreateSolidBrush(theme_.window_bg, brush_window_.GetAddressOf()) &&
           CreateSolidBrush(theme_.surface, brush_surface_.GetAddressOf()) &&
           CreateSolidBrush(theme_.surface_soft, brush_surface_soft_.GetAddressOf()) &&
           CreateSolidBrush(theme_.surface_hover, brush_surface_hover_.GetAddressOf()) &&
           CreateSolidBrush(theme_.border, brush_border_.GetAddressOf()) &&
           CreateSolidBrush(theme_.text_primary, brush_primary_text_.GetAddressOf()) &&
           CreateSolidBrush(theme_.text_secondary, brush_secondary_text_.GetAddressOf()) &&
           CreateSolidBrush(theme_.accent, brush_accent_.GetAddressOf()) &&
           CreateSolidBrush(theme_.accent_soft, brush_accent_soft_.GetAddressOf()) &&
           CreateSolidBrush(theme_.on_accent, brush_on_accent_.GetAddressOf()) &&
           CreateSolidBrush(theme_.success, brush_success_.GetAddressOf()) &&
           CreateSolidBrush(theme_.close_hover, brush_close_hover_.GetAddressOf()) &&
           CreateSolidBrush(theme_.on_close, brush_on_close_.GetAddressOf());
}

bool D2DRenderer::CreateSolidBrush(const Color& color, ID2D1SolidColorBrush** brush) {
    if (!d2d_context_) {
        return false;
    }

    HRESULT hr = d2d_context_->CreateSolidColorBrush(ToColorF(color), brush);
    if (FAILED(hr)) {
        last_error_ = hr;
        return false;
    }
    return true;
}

bool D2DRenderer::SetTheme(const ThemeTokens& theme) {
    theme_ = theme;
    if (!initialized_) {
        return true;
    }

    dynamic_brushes_.clear();
    brush_window_.Reset();
    brush_surface_.Reset();
    brush_surface_soft_.Reset();
    brush_surface_hover_.Reset();
    brush_border_.Reset();
    brush_primary_text_.Reset();
    brush_secondary_text_.Reset();
    brush_accent_.Reset();
    brush_accent_soft_.Reset();
    brush_on_accent_.Reset();
    brush_success_.Reset();
    brush_close_hover_.Reset();
    brush_on_close_.Reset();

    if (!CreateBrushes()) {
        initialized_ = false;
        return false;
    }
    return true;
}

const ThemeTokens& D2DRenderer::theme() const noexcept {
    return theme_;
}

IDWriteTextFormat* D2DRenderer::TextFormatForRole(FontRole role) const noexcept {
    switch (role) {
        case FontRole::caption:
            return ui_caption_format_.Get();
        case FontRole::body:
            return ui_body_format_.Get();
        case FontRole::label:
            return ui_label_format_.Get();
        case FontRole::heading:
            return ui_heading_format_.Get();
        case FontRole::title:
            return ui_title_format_.Get();
    }
    return ui_body_format_.Get();
}

Size D2DRenderer::measure_text(std::wstring_view text, FontRole role) const {
    IDWriteTextFormat* const format = TextFormatForRole(role);
    if (!write_factory_ || format == nullptr) {
        return Size{static_cast<float>(text.size()) * 7.0f, 16.0f};
    }

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    const HRESULT hr = write_factory_->CreateTextLayout(text.data(),
                                                        static_cast<UINT32>(text.size()),
                                                        format,
                                                        10000.0f,
                                                        2000.0f,
                                                        layout.GetAddressOf());
    if (FAILED(hr)) {
        return Size{static_cast<float>(text.size()) * 7.0f, 16.0f};
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics))) {
        return Size{static_cast<float>(text.size()) * 7.0f, 16.0f};
    }

    return Size{std::ceil(metrics.widthIncludingTrailingWhitespace), std::ceil(metrics.height)};
}

ID2D1SolidColorBrush* D2DRenderer::BrushFor(const Color& color) {
    if (!d2d_context_) {
        return nullptr;
    }

    const std::uint32_t key = (static_cast<std::uint32_t>(color.r) << 24) |
                              (static_cast<std::uint32_t>(color.g) << 16) |
                              (static_cast<std::uint32_t>(color.b) << 8) |
                              static_cast<std::uint32_t>(color.a);
    const auto existing = dynamic_brushes_.find(key);
    if (existing != dynamic_brushes_.end()) {
        return existing->second.Get();
    }

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(d2d_context_->CreateSolidColorBrush(ToColorF(color), brush.GetAddressOf()))) {
        return nullptr;
    }

    auto [iterator, inserted] = dynamic_brushes_.emplace(key, std::move(brush));
    (void)inserted;
    return iterator->second.Get();
}

void D2DRenderer::fill_rect(const Rect& rect, const Color& color) {
    if (ID2D1SolidColorBrush* const brush = BrushFor(color)) {
        d2d_context_->FillRectangle(D2D1::RectF(rect.x, rect.y, rect.right(), rect.bottom()), brush);
    }
}

void D2DRenderer::fill_rounded_rect(const Rect& rect, float radius, const Color& color) {
    if (ID2D1SolidColorBrush* const brush = BrushFor(color)) {
        const D2D1_ROUNDED_RECT rounded{
            D2D1::RectF(rect.x, rect.y, rect.right(), rect.bottom()), radius, radius};
        d2d_context_->FillRoundedRectangle(rounded, brush);
    }
}

void D2DRenderer::stroke_rounded_rect(const Rect& rect,
                                      float radius,
                                      const Color& color,
                                      float width) {
    if (ID2D1SolidColorBrush* const brush = BrushFor(color)) {
        const D2D1_ROUNDED_RECT rounded{
            D2D1::RectF(rect.x, rect.y, rect.right(), rect.bottom()), radius, radius};
        d2d_context_->DrawRoundedRectangle(rounded, brush, width);
    }
}

void D2DRenderer::draw_text(std::wstring_view text,
                            const Rect& rect,
                            FontRole role,
                            const Color& color,
                            TextAlign align,
                            TextVAlign vertical) {
    IDWriteTextFormat* const format = TextFormatForRole(role);
    ID2D1SolidColorBrush* const brush = BrushFor(color);
    if (format == nullptr || brush == nullptr) {
        return;
    }

    DWRITE_TEXT_ALIGNMENT text_alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    switch (align) {
        case TextAlign::leading:
            text_alignment = DWRITE_TEXT_ALIGNMENT_LEADING;
            break;
        case TextAlign::center:
            text_alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
            break;
        case TextAlign::trailing:
            text_alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
            break;
    }

    DWRITE_PARAGRAPH_ALIGNMENT paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    switch (vertical) {
        case TextVAlign::top:
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
            break;
        case TextVAlign::center:
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
            break;
        case TextVAlign::bottom:
            paragraph_alignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
            break;
    }

    format->SetTextAlignment(text_alignment);
    format->SetParagraphAlignment(paragraph_alignment);

    d2d_context_->DrawTextW(text.data(),
                            static_cast<UINT32>(text.size()),
                            format,
                            D2D1::RectF(rect.x, rect.y, rect.right(), rect.bottom()),
                            brush,
                            D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

bool D2DRenderer::Resize(UINT width, UINT height, float dpi) {
    if (!initialized_ || !swap_chain_ || !d2d_context_) {
        return false;
    }

    if (width == 0 || height == 0) {
        return true;
    }

    if (dpi <= 0.0f) {
        dpi = 96.0f;
    }

    if (width == width_ && height == height_ && dpi == dpi_) {
        return true;
    }

    last_error_ = S_OK;
    ReleaseTarget();

    HRESULT hr = swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        last_error_ = hr;
        TraceHresult(L"ResizeBuffers", hr);
        if (IsDeviceLost(hr)) {
            width_ = width;
            height_ = height;
            dpi_ = dpi;
            (void)RecreateDevice();
        }
        return false;
    }

    width_ = width;
    height_ = height;
    dpi_ = dpi;

    if (!CreateTargetBitmap()) {
        TraceHresult(L"CreateTargetBitmap", last_error_);
        if (IsDeviceLost(last_error_)) {
            (void)RecreateDevice();
        }
        return false;
    }

    return true;
}

bool D2DRenderer::Render(const ChromeState& chrome, Widget* content) {
    if (!initialized_ || !d2d_context_ || !swap_chain_) {
        return false;
    }

    if (!target_bitmap_ && !CreateTargetBitmap()) {
        TraceHresult(L"CreateTargetBitmap", last_error_);
        if (IsDeviceLost(last_error_)) {
            (void)RecreateDevice();
        }
        return false;
    }

    last_error_ = S_OK;
    d2d_context_->BeginDraw();
    d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
    d2d_context_->SetDpi(dpi_, dpi_);
    DrawScene(chrome, content);

    HRESULT hr = d2d_context_->EndDraw();
    if (FAILED(hr)) {
        last_error_ = hr;
        TraceHresult(L"EndDraw", hr);
        if (hr == D2DERR_RECREATE_TARGET) {
            ReleaseTarget();
            if (!CreateTargetBitmap() && IsDeviceLost(last_error_)) {
                (void)RecreateDevice();
            }
        } else if (IsDeviceLost(hr)) {
            (void)RecreateDevice();
        }
        return false;
    }

    hr = swap_chain_->Present(1, 0);
    if (FAILED(hr)) {
        last_error_ = hr;
        TraceHresult(L"Present", hr);
        if (IsDeviceLost(hr)) {
            (void)RecreateDevice();
        }
        return false;
    }

    return true;
}

void D2DRenderer::DrawScene(const ChromeState& chrome, Widget* content) noexcept {
    if (!d2d_context_) {
        return;
    }

    const D2D1_SIZE_F window_size = d2d_context_->GetSize();
    d2d_context_->Clear(ToColorF(theme_.window_bg));

    const bool active = chrome.active;
    ID2D1SolidColorBrush* titlebar_brush =
        active ? brush_surface_.Get() : brush_surface_soft_.Get();
    ID2D1SolidColorBrush* primary_brush =
        active ? brush_primary_text_.Get() : brush_secondary_text_.Get();

    const D2D1_RECT_F titlebar =
        D2D1::RectF(0.0f, 0.0f, window_size.width, kTitleBarHeightDip);
    d2d_context_->FillRectangle(titlebar, titlebar_brush);
    d2d_context_->DrawLine(D2D1::Point2F(0.0f, kTitleBarHeightDip - 0.5f),
                           D2D1::Point2F(window_size.width, kTitleBarHeightDip - 0.5f),
                           brush_border_.Get(),
                           1.0f);

    const D2D1_RECT_F app_mark = D2D1::RectF(14.0f, 9.0f, 36.0f, 31.0f);
    d2d_context_->FillRoundedRectangle(D2D1::RoundedRect(app_mark, 6.0f, 6.0f),
                                       brush_accent_.Get());
    const D2D1_RECT_F app_glyph = D2D1::RectF(19.0f, 14.0f, 31.0f, 26.0f);
    d2d_context_->DrawRoundedRectangle(D2D1::RoundedRect(app_glyph, 1.5f, 1.5f),
                                       brush_on_accent_.Get(),
                                       1.0f);
    d2d_context_->DrawLine(D2D1::Point2F(app_glyph.left, 18.0f),
                           D2D1::Point2F(app_glyph.right, 18.0f),
                           brush_on_accent_.Get(),
                           1.0f);

    constexpr std::wstring_view caption = L"CppUI";
    d2d_context_->DrawTextW(caption.data(),
                            static_cast<UINT32>(caption.size()),
                            caption_format_.Get(),
                            D2D1::RectF(44.0f, 8.0f, 92.0f, 32.0f),
                            primary_brush,
                            D2D1_DRAW_TEXT_OPTIONS_CLIP);

    const D2D1_RECT_F badge = D2D1::RectF(94.0f, 12.0f, 152.0f, 28.0f);
    d2d_context_->FillRoundedRectangle(D2D1::RoundedRect(badge, 8.0f, 8.0f),
                                       brush_accent_soft_.Get());
    constexpr std::wstring_view badge_label = L"PHASE 3";
    d2d_context_->DrawTextW(badge_label.data(),
                            static_cast<UINT32>(badge_label.size()),
                            badge_format_.Get(),
                            badge,
                            brush_accent_.Get(),
                            D2D1_DRAW_TEXT_OPTIONS_CLIP);

    const float button_width = kCaptionButtonWidthDip;
    const float close_left = window_size.width - button_width;
    const float maximize_left = close_left - button_width;
    const float minimize_left = maximize_left - button_width;
    const float center_y = kTitleBarHeightDip * 0.5f;
    const auto is_hot = [&chrome](ChromeButton button) {
        return chrome.hovered == button || chrome.pressed == button;
    };

    const D2D1_RECT_F close_rect =
        D2D1::RectF(close_left, 0.0f, window_size.width, kTitleBarHeightDip);
    if (is_hot(ChromeButton::close)) {
        d2d_context_->FillRectangle(close_rect, brush_close_hover_.Get());
    }
    ID2D1SolidColorBrush* close_brush = is_hot(ChromeButton::close)
                                            ? brush_on_close_.Get()
                                            : primary_brush;
    const float close_center_x = close_left + button_width * 0.5f;
    d2d_context_->DrawLine(D2D1::Point2F(close_center_x - 5.0f, center_y - 5.0f),
                           D2D1::Point2F(close_center_x + 5.0f, center_y + 5.0f),
                           close_brush,
                           1.0f);
    d2d_context_->DrawLine(D2D1::Point2F(close_center_x + 5.0f, center_y - 5.0f),
                           D2D1::Point2F(close_center_x - 5.0f, center_y + 5.0f),
                           close_brush,
                           1.0f);

    const D2D1_RECT_F maximize_rect =
        D2D1::RectF(maximize_left, 0.0f, close_left, kTitleBarHeightDip);
    if (chrome.pressed == ChromeButton::maximize) {
        d2d_context_->FillRectangle(maximize_rect, brush_accent_soft_.Get());
    } else if (chrome.hovered == ChromeButton::maximize) {
        d2d_context_->FillRectangle(maximize_rect, brush_surface_hover_.Get());
    }
    const float maximize_center_x = maximize_left + button_width * 0.5f;
    if (chrome.maximized) {
        d2d_context_->DrawRectangle(D2D1::RectF(maximize_center_x - 5.0f, center_y - 3.0f,
                                                maximize_center_x + 3.0f, center_y + 5.0f),
                                    primary_brush,
                                    1.0f);
        d2d_context_->DrawRectangle(D2D1::RectF(maximize_center_x - 3.0f, center_y - 5.0f,
                                                maximize_center_x + 5.0f, center_y + 3.0f),
                                    primary_brush,
                                    1.0f);
    } else {
        d2d_context_->DrawRectangle(D2D1::RectF(maximize_center_x - 5.0f, center_y - 5.0f,
                                                maximize_center_x + 5.0f, center_y + 5.0f),
                                    primary_brush,
                                    1.0f);
    }

    const D2D1_RECT_F minimize_rect =
        D2D1::RectF(minimize_left, 0.0f, maximize_left, kTitleBarHeightDip);
    if (chrome.pressed == ChromeButton::minimize) {
        d2d_context_->FillRectangle(minimize_rect, brush_accent_soft_.Get());
    } else if (chrome.hovered == ChromeButton::minimize) {
        d2d_context_->FillRectangle(minimize_rect, brush_surface_hover_.Get());
    }
    const float minimize_center_x = minimize_left + button_width * 0.5f;
    d2d_context_->DrawLine(D2D1::Point2F(minimize_center_x - 5.0f, center_y),
                           D2D1::Point2F(minimize_center_x + 5.0f, center_y),
                           primary_brush,
                           1.0f);

    if (content != nullptr) {
        content->paint(*this);
    }
}
void D2DRenderer::ReleaseTarget() noexcept {
    if (d2d_context_) {
        d2d_context_->SetTarget(nullptr);
    }
    target_bitmap_.Reset();
}

void D2DRenderer::ReleaseDeviceResources() noexcept {
    ReleaseTarget();
    brush_window_.Reset();
    brush_surface_.Reset();
    brush_surface_soft_.Reset();
    brush_surface_hover_.Reset();
    brush_border_.Reset();
    brush_primary_text_.Reset();
    brush_secondary_text_.Reset();
    brush_accent_.Reset();
    brush_accent_soft_.Reset();
    brush_on_accent_.Reset();
    brush_success_.Reset();
    brush_close_hover_.Reset();
    brush_on_close_.Reset();
    dynamic_brushes_.clear();
    ui_title_format_.Reset();
    ui_heading_format_.Reset();
    ui_label_format_.Reset();
    ui_body_format_.Reset();
    ui_caption_format_.Reset();
    badge_format_.Reset();
    caption_format_.Reset();
    version_format_.Reset();
    pill_format_.Reset();
    meta_format_.Reset();
    subtitle_format_.Reset();
    title_format_.Reset();
    write_factory_.Reset();
    d2d_context_.Reset();
    d2d_device_.Reset();
    d2d_factory_.Reset();
    swap_chain_.Reset();
    d3d_context_.Reset();
    d3d_device_.Reset();
}

void D2DRenderer::Shutdown() noexcept {
    ReleaseDeviceResources();
    initialized_ = false;
    using_warp_ = false;
    hwnd_ = nullptr;
    width_ = 0;
    height_ = 0;
    dpi_ = 96.0f;
}

}  // namespace cppui
