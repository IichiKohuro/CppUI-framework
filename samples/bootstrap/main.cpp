#include <windows.h>

#include <objbase.h>

#include <algorithm>
#include <cstdint>
#include <string_view>

#include <cppui/theme/tokens.hpp>

#include "platform/win32/win32_window.hpp"
#include "render/d2d/d2d_renderer.hpp"

namespace {

constexpr wchar_t kWindowTitle[] = L"CppUI - Phase 1";
constexpr wchar_t kErrorTitle[] = L"CppUI - Phase 1";
constexpr int kInitialWidthDip = 1000;
constexpr int kInitialHeightDip = 640;
constexpr int kMinWidthDip = 720;
constexpr int kMinHeightDip = 480;

class BootstrapApp final {
public:
    int Run(HINSTANCE instance, int show_command, bool self_test);

private:
    int RunSelfTest(cppui::Win32Window& window);
    void OnPaint();
    void OnResize(std::uint32_t width, std::uint32_t height, float dpi);
    void OnDpiChanged(float dpi);
    bool OnKeyDown(WPARAM key);
    void StopRenderer();

    cppui::Win32Window* window_{};
    cppui::D2DRenderer renderer_;
    bool renderer_ready_{false};
    int paint_failures_{0};
};

int BootstrapApp::Run(HINSTANCE instance, int show_command, bool self_test) {
    cppui::Win32Window window(instance,
                              kWindowTitle,
                              kInitialWidthDip,
                              kInitialHeightDip,
                              kMinWidthDip,
                              kMinHeightDip);

    cppui::Win32Window::Callbacks callbacks{};
    callbacks.paint = [this] { OnPaint(); };
    callbacks.resized = [this](std::uint32_t width, std::uint32_t height, float dpi) {
        OnResize(width, height, dpi);
    };
    callbacks.dpi_changed = [this](float dpi) { OnDpiChanged(dpi); };
    callbacks.key_down = [this](WPARAM key) { return OnKeyDown(key); };

    if (!window.Create(callbacks)) {
        MessageBoxW(nullptr,
                    L"Win32 window creation failed.",
                    kErrorTitle,
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    window_ = &window;

    if (!renderer_.Initialize(window.Handle(), cppui::light_theme())) {
        MessageBoxW(window.Handle(),
                    L"Direct2D/D3D11 renderer initialization failed. See the debug output for details.",
                    kErrorTitle,
                    MB_OK | MB_ICONERROR);
        window_ = nullptr;
        return 1;
    }
    renderer_ready_ = true;

    if (self_test) {
        return RunSelfTest(window);
    }

    window.Show(show_command);
    const int result = window.Run();

    renderer_.Shutdown();
    renderer_ready_ = false;
    window_ = nullptr;
    return result;
}

int BootstrapApp::RunSelfTest(cppui::Win32Window& window) {
    if (!renderer_.Render()) {
        OutputDebugStringW(L"CppUI self-test: initial render failed.\n");
        return 2;
    }

    if (!renderer_.Resize(800, 500, window.Dpi())) {
        OutputDebugStringW(L"CppUI self-test: resize failed.\n");
        return 3;
    }

    if (!renderer_.Render()) {
        OutputDebugStringW(L"CppUI self-test: render after resize failed.\n");
        return 4;
    }

    if (!renderer_.RecreateDevice()) {
        OutputDebugStringW(L"CppUI self-test: device recreation failed.\n");
        return 5;
    }

    if (!renderer_.Render()) {
        OutputDebugStringW(L"CppUI self-test: render after device recreation failed.\n");
        return 6;
    }

    renderer_.Shutdown();
    renderer_ready_ = false;
    window_ = nullptr;
    window.Destroy();
    OutputDebugStringW(L"CppUI self-test: passed.\n");
    return 0;
}

void BootstrapApp::OnPaint() {
    if (!renderer_ready_) {
        return;
    }

    if (renderer_.Render()) {
        paint_failures_ = 0;
        return;
    }

    if (!renderer_.IsInitialized()) {
        StopRenderer();
        return;
    }

    ++paint_failures_;
    if (paint_failures_ <= 2) {
        window_->Invalidate();
    } else {
        StopRenderer();
    }
}

void BootstrapApp::OnResize(std::uint32_t width, std::uint32_t height, float dpi) {
    if (!renderer_ready_) {
        return;
    }

    if (!renderer_.Resize(width, height, dpi)) {
        if (!renderer_.IsInitialized()) {
            StopRenderer();
        }
    }
}

void BootstrapApp::OnDpiChanged(float dpi) {
    if (!renderer_ready_ || window_ == nullptr) {
        return;
    }

    RECT client{};
    if (GetClientRect(window_->Handle(), &client) == FALSE) {
        return;
    }

    const UINT width = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
    const UINT height = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));
    if (!renderer_.Resize(width, height, dpi)) {
        if (!renderer_.IsInitialized()) {
            StopRenderer();
        }
    }
}

bool BootstrapApp::OnKeyDown(WPARAM key) {
    if (key == VK_ESCAPE) {
        if (window_ != nullptr) {
            window_->Destroy();
        }
        return true;
    }

    if (key == VK_F5 && renderer_ready_) {
        renderer_ready_ = renderer_.RecreateDevice();
        paint_failures_ = 0;
        if (!renderer_ready_) {
            OutputDebugStringW(L"CppUI: manual device recreation failed.\n");
        } else if (window_ != nullptr) {
            window_->Invalidate();
        }
        return true;
    }

    return false;
}

void BootstrapApp::StopRenderer() {
    renderer_ready_ = false;
    OutputDebugStringW(L"CppUI: renderer stopped after unrecoverable failure.\n");
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        return 1;
    }

    const std::wstring_view command_line = GetCommandLineW();
    const bool self_test = command_line.find(L"--self-test") != std::wstring_view::npos;

    BootstrapApp app;
    const int result = app.Run(instance, show_command, self_test);

    CoUninitialize();
    return result;
}
