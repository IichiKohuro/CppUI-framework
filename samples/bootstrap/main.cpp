#include <windows.h>

#include <objbase.h>

#include <algorithm>

#include <cppui/theme/tokens.hpp>

#include "render/d2d/d2d_renderer.hpp"

namespace {

constexpr wchar_t kWindowClass[] = L"CppUI.BootstrapWindow";
constexpr wchar_t kWindowTitle[] = L"CppUI - Phase 0";
constexpr int kInitialWidthDip = 1000;
constexpr int kInitialHeightDip = 640;
constexpr int kMinWidthDip = 720;
constexpr int kMinHeightDip = 480;

class BootstrapApp final {
public:
    int Run(HINSTANCE instance, int show_command);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void CenterOnPrimaryMonitor();
    void ResizeRenderer(UINT width, UINT height);

    HINSTANCE instance_{};
    HWND hwnd_{};
    cppui::D2DRenderer renderer_;
    bool renderer_ready_{false};
    UINT dpi_{96};
};

int BootstrapApp::Run(HINSTANCE instance, int show_command) {
    instance_ = instance;

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &BootstrapApp::WindowProc;
    window_class.hInstance = instance_;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    window_class.hIconSm = window_class.hIcon;
    window_class.lpszClassName = kWindowClass;

    if (RegisterClassExW(&window_class) == 0) {
        return 1;
    }

    dpi_ = GetDpiForSystem();
    if (dpi_ == 0) {
        dpi_ = 96;
    }

    RECT desired{0, 0, MulDiv(kInitialWidthDip, static_cast<int>(dpi_), 96),
                 MulDiv(kInitialHeightDip, static_cast<int>(dpi_), 96)};
    AdjustWindowRectExForDpi(&desired, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi_);

    hwnd_ = CreateWindowExW(0,
                            kWindowClass,
                            kWindowTitle,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            desired.right - desired.left,
                            desired.bottom - desired.top,
                            nullptr,
                            nullptr,
                            instance_,
                            this);
    if (hwnd_ == nullptr) {
        UnregisterClassW(kWindowClass, instance_);
        return 1;
    }

    dpi_ = GetDpiForWindow(hwnd_);
    if (dpi_ == 0) {
        dpi_ = 96;
    }

    if (!renderer_.Initialize(hwnd_, cppui::light_theme())) {
        MessageBoxW(hwnd_,
                    L"Direct2D renderer initialization failed. See the debug output for HRESULT details.",
                    L"CppUI - Phase 0",
                    MB_OK | MB_ICONERROR);
        DestroyWindow(hwnd_);
        UnregisterClassW(kWindowClass, instance_);
        return 1;
    }
    renderer_ready_ = true;

    CenterOnPrimaryMonitor();
    ShowWindow(hwnd_, show_command);
    UpdateWindow(hwnd_);

    MSG message{};
    int result = 0;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    result = static_cast<int>(message.wParam);

    renderer_.Shutdown();
    renderer_ready_ = false;
    UnregisterClassW(kWindowClass, instance_);
    return result;
}

void BootstrapApp::ResizeRenderer(UINT width, UINT height) {
    if (!renderer_ready_) {
        return;
    }

    if (!renderer_.Resize(width, height, static_cast<float>(dpi_))) {
        renderer_ready_ = false;
        OutputDebugStringW(L"CppUI: Direct2D renderer resize failed.\n");
    }
}

void BootstrapApp::CenterOnPrimaryMonitor() {
    if (hwnd_ == nullptr) {
        return;
    }

    RECT window_rect{};
    if (!GetWindowRect(hwnd_, &window_rect)) {
        return;
    }

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    const HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    const int x =
        monitor_info.rcWork.left + ((monitor_info.rcWork.right - monitor_info.rcWork.left) - width) / 2;
    const int y =
        monitor_info.rcWork.top + ((monitor_info.rcWork.bottom - monitor_info.rcWork.top) - height) / 2;

    SetWindowPos(hwnd_,
                 nullptr,
                 x,
                 y,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK BootstrapApp::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    BootstrapApp* app = reinterpret_cast<BootstrapApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        app = static_cast<BootstrapApp*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        return app->HandleMessage(hwnd, message, wparam, lparam);
    }

    if (app != nullptr) {
        const LRESULT result = app->HandleMessage(hwnd, message, wparam, lparam);
        if (message == WM_NCDESTROY) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return result;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT BootstrapApp::HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_SIZE:
            if (renderer_ready_) {
                const UINT width = LOWORD(lparam);
                const UINT height = HIWORD(lparam);
                UINT window_dpi = GetDpiForWindow(hwnd);
                if (window_dpi == 0) {
                    window_dpi = 96;
                }
                dpi_ = window_dpi;
                if (width > 0 && height > 0) {
                    ResizeRenderer(width, height);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            if (dpi_ == 0) {
                dpi_ = 96;
            }

            const auto* suggested = reinterpret_cast<const RECT*>(lparam);
            SetWindowPos(hwnd,
                         nullptr,
                         suggested->left,
                         suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);

            if (renderer_ready_) {
                RECT client{};
                GetClientRect(hwnd, &client);
                const UINT width = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
                const UINT height = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));
                ResizeRenderer(width, height);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            auto* minmax = reinterpret_cast<MINMAXINFO*>(lparam);
            UINT window_dpi = GetDpiForWindow(hwnd);
            if (window_dpi == 0) {
                window_dpi = 96;
            }
            minmax->ptMinTrackSize.x = MulDiv(kMinWidthDip, static_cast<int>(window_dpi), 96);
            minmax->ptMinTrackSize.y = MulDiv(kMinHeightDip, static_cast<int>(window_dpi), 96);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            if (renderer_ready_ && !renderer_.Render()) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            EndPaint(hwnd, &paint);
            return 0;
        }

        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_NCDESTROY:
            break;

        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        return 1;
    }

    BootstrapApp app;
    const int result = app.Run(instance, show_command);

    CoUninitialize();
    return result;
}
