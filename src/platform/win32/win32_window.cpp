#include "platform/win32/win32_window.hpp"

#include <windowsx.h>

#include <dwmapi.h>

#include <utility>

namespace cppui {
namespace {

constexpr wchar_t kWindowClass[] = L"CppUI.MainWindow";
constexpr int kResizeBorderDip = 7;

UINT WindowDpi(HWND hwnd) noexcept {
    const UINT dpi = GetDpiForWindow(hwnd);
    return dpi == 0 ? 96U : dpi;
}

int DipToPixels(int dip, UINT dpi) noexcept {
    return MulDiv(dip, static_cast<int>(dpi), 96);
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

    const UINT system_dpi = GetDpiForSystem();
    const UINT initial_dpi = system_dpi == 0 ? 96U : system_dpi;

    hwnd_ = CreateWindowExW(0,
                            kWindowClass,
                            title_.c_str(),
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            DipToPixels(width_dip_, initial_dpi),
                            DipToPixels(height_dip_, initial_dpi),
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
    ApplyDwmChrome();
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

bool Win32Window::IsWindowMinimized() const noexcept {
    return hwnd_ != nullptr && IsIconic(hwnd_) != FALSE;
}

ChromeState Win32Window::GetChromeState() const noexcept {
    return ChromeState{
        active_,
        hwnd_ != nullptr && IsZoomed(hwnd_) != FALSE,
        hovered_,
        pressed_,
    };
}

ChromeButton Win32Window::ButtonAt(POINT screen_point) const noexcept {
    if (hwnd_ == nullptr) {
        return ChromeButton::none;
    }

    POINT client = screen_point;
    if (ScreenToClient(hwnd_, &client) == FALSE) {
        return ChromeButton::none;
    }

    RECT client_rect{};
    if (GetClientRect(hwnd_, &client_rect) == FALSE) {
        return ChromeButton::none;
    }

    const UINT dpi = WindowDpi(hwnd_);
    const int title_bar_height = DipToPixels(static_cast<int>(kTitleBarHeightDip), dpi);
    const int button_width = DipToPixels(static_cast<int>(kCaptionButtonWidthDip), dpi);

    if (client.x < 0 || client.x >= client_rect.right || client.y < 0 || client.y >= title_bar_height) {
        return ChromeButton::none;
    }

    const int right = client_rect.right;
    if (client.x >= right - button_width) {
        return ChromeButton::close;
    }
    if (client.x >= right - button_width * 2) {
        return ChromeButton::maximize;
    }
    if (client.x >= right - button_width * 3) {
        return ChromeButton::minimize;
    }
    return ChromeButton::none;
}

ChromeButton Win32Window::ButtonFromHitCode(LRESULT hit_code) noexcept {
    switch (hit_code) {
        case HTMINBUTTON:
            return ChromeButton::minimize;
        case HTMAXBUTTON:
            return ChromeButton::maximize;
        case HTCLOSE:
            return ChromeButton::close;
        default:
            return ChromeButton::none;
    }
}

LRESULT Win32Window::HitTest(POINT screen_point) const noexcept {
    if (hwnd_ == nullptr) {
        return HTNOWHERE;
    }

    POINT client = screen_point;
    if (ScreenToClient(hwnd_, &client) == FALSE) {
        return HTNOWHERE;
    }

    RECT client_rect{};
    if (GetClientRect(hwnd_, &client_rect) == FALSE) {
        return HTNOWHERE;
    }

    const UINT dpi = WindowDpi(hwnd_);
    const bool maximized = IsZoomed(hwnd_) != FALSE;

    if (!maximized) {
        const int border = DipToPixels(kResizeBorderDip, dpi);
        const bool left = client.x < border;
        const bool right = client.x >= client_rect.right - border;
        const bool top = client.y < border;
        const bool bottom = client.y >= client_rect.bottom - border;

        if (top && left) {
            return HTTOPLEFT;
        }
        if (top && right) {
            return HTTOPRIGHT;
        }
        if (bottom && left) {
            return HTBOTTOMLEFT;
        }
        if (bottom && right) {
            return HTBOTTOMRIGHT;
        }
        if (left) {
            return HTLEFT;
        }
        if (right) {
            return HTRIGHT;
        }
        if (top) {
            return HTTOP;
        }
        if (bottom) {
            return HTBOTTOM;
        }
    }

    switch (ButtonAt(screen_point)) {
        case ChromeButton::minimize:
            return HTMINBUTTON;
        case ChromeButton::maximize:
            return HTMAXBUTTON;
        case ChromeButton::close:
            return HTCLOSE;
        case ChromeButton::none:
            break;
    }

    const int title_bar_height = DipToPixels(static_cast<int>(kTitleBarHeightDip), dpi);
    if (client.x >= 0 && client.x < client_rect.right && client.y >= 0 &&
        client.y < title_bar_height) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

void Win32Window::PerformChromeAction(ChromeButton button) noexcept {
    if (hwnd_ == nullptr) {
        return;
    }

    switch (button) {
        case ChromeButton::minimize:
            ShowWindow(hwnd_, SW_MINIMIZE);
            break;
        case ChromeButton::maximize:
            ShowWindow(hwnd_, IsZoomed(hwnd_) != FALSE ? SW_RESTORE : SW_MAXIMIZE);
            InvalidateRect(hwnd_, nullptr, FALSE);
            break;
        case ChromeButton::close:
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            break;
        case ChromeButton::none:
            break;
    }
}

void Win32Window::SetActive(bool active) noexcept {
    if (active_ == active) {
        return;
    }
    active_ = active;
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void Win32Window::SetHovered(ChromeButton button) noexcept {
    if (hovered_ == button) {
        return;
    }
    hovered_ = button;
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void Win32Window::ApplyDwmChrome() noexcept {
    if (hwnd_ == nullptr) {
        return;
    }

    const DWM_WINDOW_CORNER_PREFERENCE corner_preference = DWMWCP_ROUND;
    (void)DwmSetWindowAttribute(hwnd_,
                                DWMWA_WINDOW_CORNER_PREFERENCE,
                                &corner_preference,
                                sizeof(corner_preference));

    const MARGINS margins{1, 1, 1, 1};
    (void)DwmExtendFrameIntoClientArea(hwnd_, &margins);
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
        case WM_NCCALCSIZE:
            if (wparam == TRUE) {
                auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
                if (IsZoomed(hwnd) != FALSE) {
                    const UINT dpi = WindowDpi(hwnd);
                    const int frame_x = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
                                        GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
                    const int frame_y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) +
                                        GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
                    params->rgrc[0].left += frame_x;
                    params->rgrc[0].top += frame_y;
                    params->rgrc[0].right -= frame_x;
                    params->rgrc[0].bottom -= frame_y;
                }
                return 0;
            }
            break;

        case WM_NCPAINT:
            return 0;

        case WM_NCACTIVATE:
            SetActive(wparam != FALSE);
            return TRUE;

        case WM_ACTIVATE:
            SetActive(LOWORD(wparam) != WA_INACTIVE);
            return 0;

        case WM_NCHITTEST:
            return HitTest(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});

        case WM_NCMOUSEMOVE: {
            const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            SetHovered(ButtonAt(point));
            TRACKMOUSEEVENT track{};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE | TME_NONCLIENT;
            track.hwndTrack = hwnd;
            (void)TrackMouseEvent(&track);
            return 0;
        }

        case WM_NCMOUSELEAVE:
            SetHovered(ChromeButton::none);
            return 0;

        case WM_MOUSEMOVE:
            SetHovered(ChromeButton::none);
            break;

        case WM_NCLBUTTONDOWN: {
            const ChromeButton button = ButtonFromHitCode(wparam);
            if (button != ChromeButton::none) {
                SetActive(true);
                pressed_ = button;
                SetHovered(button);
                SetCapture(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            break;
        }

        case WM_NCLBUTTONUP: {
            if (pressed_ != ChromeButton::none) {
                const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                const ChromeButton current = ButtonAt(point);
                const ChromeButton pressed = pressed_;
                pressed_ = ChromeButton::none;
                if (GetCapture() == hwnd) {
                    ReleaseCapture();
                }
                SetHovered(current);
                if (current == pressed) {
                    PerformChromeAction(pressed);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            break;
        }

        case WM_CAPTURECHANGED:
            pressed_ = ChromeButton::none;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

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
