#include "platform/win32/win32_window.hpp"

#include <windowsx.h>

#include <dwmapi.h>
#include <imm.h>

#include <algorithm>
#include <atomic>
#include <utility>

namespace cppui {
namespace {

constexpr int kResizeBorderDip = 7;
std::atomic<unsigned int> g_window_class_counter{0};

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
      class_name_(L"CppUI.Window." +
                  std::to_wstring(++g_window_class_counter)),
      title_(std::move(title)),
      width_dip_(width_dip),
      height_dip_(height_dip),
      min_width_dip_(min_width_dip),
      min_height_dip_(min_height_dip) {}

Win32Window::~Win32Window() {
    Destroy();
    if (class_registered_) {
        UnregisterClassW(class_name_.c_str(), instance_);
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
    window_class.lpszClassName = class_name_.c_str();

    if (RegisterClassExW(&window_class) == 0) {
        return false;
    }
    class_registered_ = true;

    const UINT system_dpi = GetDpiForSystem();
    const UINT initial_dpi = system_dpi == 0 ? 96U : system_dpi;

    hwnd_ = CreateWindowExW(0,
                            class_name_.c_str(),
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
        UnregisterClassW(class_name_.c_str(), instance_);
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

int Win32Window::RunModal() {
    if (hwnd_ == nullptr) {
        return 1;
    }

    MSG message{};
    while (hwnd_ != nullptr && IsWindow(hwnd_) != FALSE) {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == -1) {
            return 1;
        }
        if (result == 0) {
            PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}

void Win32Window::Destroy() noexcept {
    if (hwnd_ != nullptr) {
        for (UINT_PTR timer : timers_) {
            KillTimer(hwnd_, timer);
        }
        timers_.clear();
        const HWND hwnd = hwnd_;
        hwnd_ = nullptr;
        DestroyWindow(hwnd);
    }
}

void Win32Window::StartTimer(UINT_PTR timer_id, UINT milliseconds) noexcept {
    if (hwnd_ == nullptr || milliseconds == 0) {
        return;
    }
    if (timers_.contains(timer_id)) {
        return;
    }
    if (SetTimer(hwnd_, timer_id, milliseconds, nullptr) != 0) {
        timers_.insert(timer_id);
    }
}

void Win32Window::StopTimer(UINT_PTR timer_id) noexcept {
    if (hwnd_ == nullptr) {
        return;
    }
    KillTimer(hwnd_, timer_id);
    timers_.erase(timer_id);
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
        title_.c_str(),
        active_,
        hwnd_ != nullptr && IsZoomed(hwnd_) != FALSE,
        hovered_,
        pressed_,
        theme_mode_,
        theme_hovered_,
        theme_pressed_,
        action_hovered_,
        action_pressed_,
        show_theme_switcher_,
        show_chrome_action_,
        show_badge_,
    };
}

void Win32Window::SetChromeExtras(bool theme_switcher, bool action, bool badge) noexcept {
    show_theme_switcher_ = theme_switcher;
    show_chrome_action_ = action;
    show_badge_ = badge;
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void Win32Window::SetThemeMode(ThemeMode mode) noexcept {
    if (theme_mode_ == mode) {
        return;
    }
    theme_mode_ = mode;
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
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

ChromeButton Win32Window::ThemeButtonAt(POINT screen_point) const noexcept {
    if (hwnd_ == nullptr || !show_theme_switcher_) {
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
    const int button_width = DipToPixels(static_cast<int>(kCaptionButtonWidthDip), dpi);
    const int option_width = DipToPixels(static_cast<int>(kThemeOptionWidthDip), dpi);
    const int switch_height = DipToPixels(static_cast<int>(kThemeSwitcherHeightDip), dpi);
    const int switch_gap = DipToPixels(static_cast<int>(kThemeSwitcherGapDip), dpi);
    const int separator_gap = DipToPixels(static_cast<int>(kTitleButtonSeparatorDip), dpi);
    const int title_height = DipToPixels(static_cast<int>(kTitleBarHeightDip), dpi);

    const int separator_x = client_rect.right - button_width * 3 - separator_gap;
    const int switch_right = separator_x - switch_gap;
    const int switch_left = switch_right - option_width * 3;
    const int switch_top = (title_height - switch_height) / 2;

    if (client.x < switch_left || client.x >= switch_right || client.y < switch_top ||
        client.y >= switch_top + switch_height) {
        return ChromeButton::none;
    }

    const int index = (client.x - switch_left) / option_width;
    switch (index) {
        case 0:
            return ChromeButton::theme_light;
        case 1:
            return ChromeButton::theme_dark;
        case 2:
            return ChromeButton::theme_system;
        default:
            return ChromeButton::none;
    }
}

ChromeButton Win32Window::ActionButtonAt(POINT screen_point) const noexcept {
    if (hwnd_ == nullptr || !show_chrome_action_) {
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
    const int button_width = DipToPixels(static_cast<int>(kCaptionButtonWidthDip), dpi);
    const int option_width = DipToPixels(static_cast<int>(kThemeOptionWidthDip), dpi);
    const int action_width = DipToPixels(static_cast<int>(kChromeActionWidthDip), dpi);
    const int switch_height = DipToPixels(static_cast<int>(kThemeSwitcherHeightDip), dpi);
    const int switch_gap = DipToPixels(static_cast<int>(kThemeSwitcherGapDip), dpi);
    const int separator_gap = DipToPixels(static_cast<int>(kTitleButtonSeparatorDip), dpi);
    const int title_height = DipToPixels(static_cast<int>(kTitleBarHeightDip), dpi);

    const int separator_x = client_rect.right - button_width * 3 - separator_gap;
    const int switch_right = separator_x - switch_gap;
    const int switch_left = switch_right - option_width * 3;
    const int action_right = switch_left - switch_gap;
    const int action_left = action_right - action_width;
    const int action_top = (title_height - switch_height) / 2;

    if (client.x < action_left || client.x >= action_right || client.y < action_top ||
        client.y >= action_top + switch_height) {
        return ChromeButton::none;
    }
    return ChromeButton::action_reset;
}

ThemeMode Win32Window::ThemeModeForButton(ChromeButton button) noexcept {
    switch (button) {
        case ChromeButton::theme_dark:
            return ThemeMode::dark;
        case ChromeButton::theme_system:
            return ThemeMode::system;
        case ChromeButton::theme_light:
        default:
            return ThemeMode::light;
    }
}

void Win32Window::CompleteThemePress(POINT screen_point) noexcept {
    if (theme_pressed_ == ChromeButton::none) {
        return;
    }

    const ChromeButton current = ThemeButtonAt(screen_point);
    const ChromeButton pressed = theme_pressed_;
    theme_pressed_ = ChromeButton::none;
    theme_hovered_ = current;

    if (current == pressed && callbacks_.theme_selected != nullptr) {
        callbacks_.theme_selected(ThemeModeForButton(pressed));
    }
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void Win32Window::CompleteActionPress(POINT screen_point) noexcept {
    if (action_pressed_ == ChromeButton::none) {
        return;
    }

    const ChromeButton current = ActionButtonAt(screen_point);
    const ChromeButton pressed = action_pressed_;
    action_pressed_ = ChromeButton::none;
    action_hovered_ = current;

    if (current == pressed && callbacks_.chrome_action != nullptr) {
        callbacks_.chrome_action(pressed);
    }
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
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

    if (ActionButtonAt(screen_point) != ChromeButton::none ||
        ThemeButtonAt(screen_point) != ChromeButton::none) {
        return HTCLIENT;
    }

    switch (ButtonAt(screen_point)) {
        case ChromeButton::minimize:
            return HTMINBUTTON;
        case ChromeButton::maximize:
            return HTMAXBUTTON;
        case ChromeButton::close:
            return HTCLOSE;
        case ChromeButton::theme_light:
        case ChromeButton::theme_dark:
        case ChromeButton::theme_system:
        case ChromeButton::action_reset:
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

void Win32Window::CompleteChromePress(POINT screen_point) noexcept {
    if (pressed_ == ChromeButton::none) {
        return;
    }

    const ChromeButton current = ButtonAt(screen_point);
    const ChromeButton pressed = pressed_;
    pressed_ = ChromeButton::none;
    SetHovered(current);

    if (current == pressed) {
        PerformChromeAction(pressed);
    } else if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
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
        case ChromeButton::theme_light:
        case ChromeButton::theme_dark:
        case ChromeButton::theme_system:
        case ChromeButton::action_reset:
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

void Win32Window::Activate() noexcept {
    if (hwnd_ == nullptr) {
        return;
    }
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    (void)SetForegroundWindow(hwnd_);
    (void)SetActiveWindow(hwnd_);
    (void)SetFocus(hwnd_);
}

void Win32Window::CenterOnWindow(HWND owner) noexcept {
    if (hwnd_ == nullptr || owner == nullptr || IsWindow(owner) == FALSE) {
        CenterOnPrimaryMonitor();
        return;
    }

    RECT owner_rect{};
    RECT window_rect{};
    if (!GetWindowRect(owner, &owner_rect) || !GetWindowRect(hwnd_, &window_rect)) {
        return;
    }

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    const HMONITOR monitor = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    const int width = window_rect.right - window_rect.left;
    const int height = window_rect.bottom - window_rect.top;
    int x = owner_rect.left + ((owner_rect.right - owner_rect.left) - width) / 2;
    int y = owner_rect.top + ((owner_rect.bottom - owner_rect.top) - height) / 2;
    x = std::clamp(x, static_cast<int>(monitor_info.rcWork.left),
                   static_cast<int>(monitor_info.rcWork.right) - width);
    y = std::clamp(y, static_cast<int>(monitor_info.rcWork.top),
                   static_cast<int>(monitor_info.rcWork.bottom) - height);

    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Win32Window::SetImeCaretRect(int client_x, int client_y, int height) noexcept {
    if (hwnd_ == nullptr) {
        return;
    }
    HIMC context = ImmGetContext(hwnd_);
    if (context == nullptr) {
        return;
    }

    POINT point{client_x, client_y};
    (void)ClientToScreen(hwnd_, &point);

    COMPOSITIONFORM composition{};
    composition.dwStyle = CFS_POINT;
    composition.ptCurrentPos = point;
    (void)ImmSetCompositionWindow(context, &composition);

    CANDIDATEFORM candidate{};
    candidate.dwIndex = 0;
    candidate.dwStyle = CFS_CANDIDATEPOS;
    candidate.ptCurrentPos = point;
    candidate.rcArea = RECT{point.x, point.y - height, point.x + 2, point.y + height};
    (void)ImmSetCandidateWindow(context, &candidate);
    ImmReleaseContext(hwnd_, context);
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
            }
            // Returning 0 for both TRUE and FALSE is required: otherwise the
            // first WM_NCCALCSIZE (FALSE) leaves a standard caption on the
            // dialog, shrinking its client area by ~40 px and shifting every
            // button/hit area.
            return 0;

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
            const ChromeButton button = ButtonAt(point);
            SetHovered(button);
            if (theme_hovered_ != ChromeButton::none ||
                action_hovered_ != ChromeButton::none) {
                theme_hovered_ = ChromeButton::none;
                action_hovered_ = ChromeButton::none;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (pressed_ != ChromeButton::none && button != pressed_) {
                pressed_ = ChromeButton::none;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            TRACKMOUSEEVENT track{};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE | TME_NONCLIENT;
            track.hwndTrack = hwnd;
            (void)TrackMouseEvent(&track);
            return 0;
        }

        case WM_NCMOUSELEAVE:
            pressed_ = ChromeButton::none;
            SetHovered(ChromeButton::none);
            return 0;

        case WM_MOUSEMOVE:
            if (pressed_ != ChromeButton::none) {
                pressed_ = ChromeButton::none;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            SetHovered(ChromeButton::none);
            {
                POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ClientToScreen(hwnd, &screen);
                const ChromeButton theme = ThemeButtonAt(screen);
                if (theme != theme_hovered_) {
                    theme_hovered_ = theme;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                const ChromeButton action = ActionButtonAt(screen);
                if (action != action_hovered_) {
                    action_hovered_ = action;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            if (callbacks_.mouse_move != nullptr) {
                callbacks_.mouse_move(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            }
            {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd;
                (void)TrackMouseEvent(&track);
            }
            break;

        case WM_MOUSELEAVE:
            SetHovered(ChromeButton::none);
            theme_hovered_ = ChromeButton::none;
            theme_pressed_ = ChromeButton::none;
            action_hovered_ = ChromeButton::none;
            action_pressed_ = ChromeButton::none;
            if (callbacks_.mouse_leave != nullptr) {
                callbacks_.mouse_leave();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_LBUTTONDOWN:
            {
                POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ClientToScreen(hwnd, &screen);
                const ChromeButton action = ActionButtonAt(screen);
                if (action != ChromeButton::none) {
                    SetActive(true);
                    action_pressed_ = action;
                    action_hovered_ = action;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                const ChromeButton theme = ThemeButtonAt(screen);
                if (theme != ChromeButton::none) {
                    SetActive(true);
                    theme_pressed_ = theme;
                    theme_hovered_ = theme;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            if (callbacks_.mouse_down != nullptr) {
                callbacks_.mouse_down(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            }
            return 0;

        case WM_NCLBUTTONDOWN: {
            const ChromeButton button = ButtonFromHitCode(wparam);
            if (button != ChromeButton::none) {
                // Do not SetCapture here: once captured, the mouse-up is
                // delivered as a client WM_LBUTTONUP and the native caption
                // button path is bypassed. Keep the button armed until the
                // non-client up or the client fallback arrives.
                SetActive(true);
                pressed_ = button;
                SetHovered(button);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            break;
        }

        case WM_NCLBUTTONUP:
            if (pressed_ != ChromeButton::none) {
                CompleteChromePress(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            if (action_pressed_ != ChromeButton::none) {
                POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ClientToScreen(hwnd, &screen);
                CompleteActionPress(screen);
                return 0;
            }
            if (theme_pressed_ != ChromeButton::none) {
                POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ClientToScreen(hwnd, &screen);
                CompleteThemePress(screen);
                return 0;
            }
            if (pressed_ != ChromeButton::none) {
                // Some input stacks still deliver the release as a client
                // message; accept that path as a fallback.
                POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ClientToScreen(hwnd, &screen);
                CompleteChromePress(screen);
                return 0;
            }
            if (callbacks_.mouse_up != nullptr) {
                callbacks_.mouse_up(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            }
            return 0;

        case WM_SETTINGCHANGE:
            if (lparam != 0 &&
                lstrcmpiW(reinterpret_cast<LPCWSTR>(lparam), L"ImmersiveColorSet") == 0 &&
                callbacks_.system_theme_changed != nullptr) {
                callbacks_.system_theme_changed();
            }
            break;

        case WM_THEMECHANGED:
            if (callbacks_.system_theme_changed != nullptr) {
                callbacks_.system_theme_changed();
            }
            return 0;

        case WM_CANCELMODE:
            if (pressed_ != ChromeButton::none) {
                pressed_ = ChromeButton::none;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;

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

            // Maximize into the monitor work area rather than the full monitor.
            // This keeps a normal taskbar visible on multi-monitor setups and
            // with a per-monitor taskbar configuration.
            MONITORINFO monitor{};
            monitor.cbSize = sizeof(monitor);
            const HMONITOR current =
                MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (current != nullptr && GetMonitorInfoW(current, &monitor) != FALSE) {
                const RECT& work = monitor.rcWork;
                const RECT& full = monitor.rcMonitor;
                minmax->ptMaxPosition.x = work.left - full.left;
                minmax->ptMaxPosition.y = work.top - full.top;
                minmax->ptMaxSize.x = work.right - work.left;
                minmax->ptMaxSize.y = work.bottom - work.top;
            }
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

        case WM_CHAR:
        case WM_IME_CHAR:
            if (callbacks_.char_input != nullptr &&
                callbacks_.char_input(static_cast<wchar_t>(wparam))) {
                return 0;
            }
            break;

        case WM_IME_COMPOSITION:
            if ((lparam & GCS_RESULTSTR) != 0) {
                HIMC context = ImmGetContext(hwnd);
                if (context != nullptr) {
                    const LONG bytes = ImmGetCompositionStringW(context, GCS_RESULTSTR, nullptr, 0);
                    if (bytes > 0) {
                        std::wstring text(static_cast<std::size_t>(bytes) / sizeof(wchar_t), L'\0');
                        const LONG copied = ImmGetCompositionStringW(context,
                                                                   GCS_RESULTSTR,
                                                                   text.data(),
                                                                   bytes);
                        if (copied > 0) {
                            text.resize(static_cast<std::size_t>(copied) / sizeof(wchar_t));
                            if (callbacks_.char_input != nullptr) {
                                for (wchar_t character : text) {
                                    (void)callbacks_.char_input(character);
                                }
                            }
                        }
                    }
                    ImmReleaseContext(hwnd, context);
                }
                return 0;
            }
            break;

        case WM_MOUSEWHEEL:
            if (callbacks_.mouse_wheel != nullptr) {
                POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                (void)ScreenToClient(hwnd, &point);
                const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) /
                                    static_cast<float>(WHEEL_DELTA);
                callbacks_.mouse_wheel(point.x, point.y, delta);
            }
            return 0;

        case WM_TIMER:
            if (callbacks_.timer != nullptr) {
                callbacks_.timer(wparam);
                return 0;
            }
            break;

        case WM_DESTROY:
            for (UINT_PTR timer : timers_) {
                KillTimer(hwnd, timer);
            }
            timers_.clear();
            if (quit_on_destroy_) {
                PostQuitMessage(0);
            }
            return 0;

        case WM_CLOSE:
            if (callbacks_.closing != nullptr) {
                callbacks_.closing();
            }
            break;

        case WM_NCDESTROY:
            break;

        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace cppui
