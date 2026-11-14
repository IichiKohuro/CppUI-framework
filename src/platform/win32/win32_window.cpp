#include "platform/win32/win32_window.hpp"

#include <utility>

namespace cppui {
namespace {

constexpr wchar_t kWindowClass[] = L"CppUI.MainWindow";

UINT WindowDpi(HWND hwnd) noexcept {
    const UINT dpi = GetDpiForWindow(hwnd);
    return dpi == 0 ? 96U : dpi;
}

}  // namespace

Win32Window::Win32Window(HINSTANCE instance,
                         std::wstring title,
                         int width_dip,
                         int height_dip,
                         int min_width_dip,
                         int min_height_dip)
    : instance_(instance),
      title_(std::move(title)),
      width_dip_(width_dip),
      height_dip_(height_dip),
      min_width_dip_(min_width_dip),
      min_height_dip_(min_height_dip) {}

Win32Window::~Win32Window() {
    Destroy();
    if (class_registered_) {
        UnregisterClassW(kWindowClass, instance_);
        class_registered_ = false;
    }
}

bool Win32Window::Create(Callbacks callbacks) {
    if (hwnd_ != nullptr) {
        return false;
    }

    callbacks_ = std::move(callbacks);

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &Win32Window::WindowProc;
    window_class.hInstance = instance_;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    window_class.hIconSm = window_class.hIcon;
    window_class.lpszClassName = kWindowClass;

    if (RegisterClassExW(&window_class) == 0) {
        return false;
    }
    class_registered_ = true;

    const UINT dpi = GetDpiForSystem();
    const int initial_dpi = dpi == 0 ? 96 : static_cast<int>(dpi);
    RECT desired{0,
                 0,
                 MulDiv(width_dip_, initial_dpi, 96),
                 MulDiv(height_dip_, initial_dpi, 96)};
    AdjustWindowRectExForDpi(&desired, WS_OVERLAPPEDWINDOW, FALSE, 0, static_cast<UINT>(initial_dpi));

    hwnd_ = CreateWindowExW(0,
                            kWindowClass,
                            title_.c_str(),
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
        class_registered_ = false;
        return false;
    }

    dpi_ = static_cast<float>(WindowDpi(hwnd_));
    CenterOnPrimaryMonitor();
    return true;
}

void Win32Window::Show(int show_command) noexcept {
    if (hwnd_ == nullptr) {
        return;
    }

    ShowWindow(hwnd_, show_command);
    UpdateWindow(hwnd_);
}

int Win32Window::Run() {
    if (hwnd_ == nullptr) {
        return 1;
    }

    MSG message{};
    for (;;) {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == -1) {
            return 1;
        }
        if (result == 0) {
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

void Win32Window::Destroy() noexcept {
    if (hwnd_ != nullptr) {
        const HWND hwnd = hwnd_;
        hwnd_ = nullptr;
        DestroyWindow(hwnd);
    }
}

void Win32Window::Invalidate() noexcept {
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

bool Win32Window::IsMinimized() const noexcept {
    return hwnd_ != nullptr && IsIconic(hwnd_) != FALSE;
}

void Win32Window::NotifyResize(HWND hwnd, UINT width, UINT height) noexcept {
    if (width == 0 || height == 0 || callbacks_.resized == nullptr) {
        return;
    }

    dpi_ = static_cast<float>(WindowDpi(hwnd));
    callbacks_.resized(width, height, dpi_);
}

void Win32Window::CenterOnPrimaryMonitor() noexcept {
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

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        window = static_cast<Win32Window*>(create->lpCreateParams);
        if (window == nullptr) {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return window->HandleMessage(hwnd, message, wparam, lparam);
    }

    if (window == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    const LRESULT result = window->HandleMessage(hwnd, message, wparam, lparam);
    if (message == WM_NCDESTROY) {
        window->hwnd_ = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }
    return result;
}

LRESULT Win32Window::HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_SIZE:
            if (wparam != SIZE_MINIMIZED) {
                const UINT width = LOWORD(lparam);
                const UINT height = HIWORD(lparam);
                if (width > 0 && height > 0) {
                    NotifyResize(hwnd, width, height);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_DPICHANGED: {
            const auto* suggested = reinterpret_cast<const RECT*>(lparam);
            if (suggested != nullptr) {
                SetWindowPos(hwnd,
                             nullptr,
                             suggested->left,
                             suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            dpi_ = static_cast<float>(WindowDpi(hwnd));
            if (callbacks_.dpi_changed != nullptr) {
                callbacks_.dpi_changed(dpi_);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            auto* minmax = reinterpret_cast<MINMAXINFO*>(lparam);
            const UINT window_dpi = WindowDpi(hwnd);
            minmax->ptMinTrackSize.x = MulDiv(min_width_dip_, static_cast<int>(window_dpi), 96);
            minmax->ptMinTrackSize.y = MulDiv(min_height_dip_, static_cast<int>(window_dpi), 96);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            if (callbacks_.paint != nullptr) {
                callbacks_.paint();
            }
            EndPaint(hwnd, &paint);
            return 0;
        }

        case WM_KEYDOWN:
            if (callbacks_.key_down != nullptr && callbacks_.key_down(wparam)) {
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

}  // namespace cppui
