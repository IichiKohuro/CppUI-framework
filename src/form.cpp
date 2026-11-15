#include <cppui/form.hpp>

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

#include <cppui/geometry.hpp>
#include <cppui/ui/form.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/painter.hpp>
#include <cppui/ui/popup.hpp>

#include "platform/win32/win32_window.hpp"
#include "render/d2d/d2d_renderer.hpp"

namespace cppui {
namespace {

constexpr UINT_PTR kAnimationTimer = 1;
constexpr UINT_PTR kTooltipTimer = 2;
constexpr UINT kAnimationIntervalMs = 16;
constexpr UINT kTooltipDelayMs = 600;

}  // namespace

bool IsPerMonitorDpiAware() noexcept {
    const DPI_AWARENESS_CONTEXT context = GetThreadDpiAwarenessContext();
    return GetAwarenessFromDpiAwarenessContext(context) == DPI_AWARENESS_PER_MONITOR_AWARE;
}

struct Form::Impl {
    Impl(std::wstring form_title, int width, int height, int min_width, int min_height)
        : title(std::move(form_title)),
          width_dip(width),
          height_dip(height),
          min_width_dip(min_width),
          min_height_dip(min_height) {}

    [[nodiscard]] bool Create();
    void SetContent(std::unique_ptr<Widget> content);
    void BuildRoot(std::unique_ptr<Widget> content);
    void Layout();
    void UpdateStatusRight();
    void SetStatusText(std::wstring text);
    void SetThemeMode(ThemeMode mode);
    void SetDensityMode(DensityMode mode);
    [[nodiscard]] std::wstring ThemeText() const;
    [[nodiscard]] const wchar_t* DensityName() const noexcept;
    void Paint();
    void OnResize(std::uint32_t width, std::uint32_t height, float dpi);
    void OnDpiChanged(float dpi);
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnMouseUp(int x, int y);
    void OnMouseLeave();
    void OnMouseWheel(int x, int y, float delta);
    bool OnKeyDown(WPARAM key);
    bool OnChar(wchar_t character);
    void OnTimer(UINT_PTR timer_id);
    void UpdateAnimationTimer();
    void UpdateImePosition();
    void HideTooltip();
    [[nodiscard]] Point ToDip(int x, int y) const;
    void OnThemeChanged();
    bool CapturePng(std::wstring_view path);
    void Shutdown();

    std::wstring title;
    int width_dip{};
    int height_dip{};
    int min_width_dip{};
    int min_height_dip{};

    std::unique_ptr<Win32Window> window;
    std::unique_ptr<D2DRenderer> renderer;
    UiHost ui;
    PopupHost* popup_host{};
    std::unique_ptr<Widget> pending_content;
    StatusBar* status_view{};
    ThemeMode theme_mode{ThemeMode::light};
    DensityMode density_mode{DensityMode::cozy};
    float dpi{96.0f};
    bool created{false};
    Point last_mouse{};
    Widget* tooltip_owner{};
    bool animation_timer_active{false};
    std::chrono::steady_clock::time_point last_tick{};

    std::function<void(ThemeMode)> on_theme_changed;
    std::function<void(DensityMode)> on_density_changed;
    std::function<void()> on_chrome_action;
};

bool Form::Impl::Create() {
    if (created) {
        return true;
    }

    if (!IsPerMonitorDpiAware()) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            OutputDebugStringW(
                L"CppUI: process is not per-monitor DPI aware. Call "
                L"SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) "
                L"before creating a Form.\n");
        }
    }

    window = std::make_unique<Win32Window>(GetModuleHandleW(nullptr),
                                           title,
                                           width_dip,
                                           height_dip,
                                           min_width_dip,
                                           min_height_dip);

    Win32Window::Callbacks callbacks{};
    callbacks.paint = [this] { Paint(); };
    callbacks.resized = [this](std::uint32_t width, std::uint32_t height, float new_dpi) {
        OnResize(width, height, new_dpi);
    };
    callbacks.dpi_changed = [this](float new_dpi) { OnDpiChanged(new_dpi); };
    callbacks.key_down = [this](WPARAM key) { return OnKeyDown(key); };
    callbacks.char_input = [this](wchar_t character) { return OnChar(character); };
    callbacks.mouse_move = [this](int x, int y) { OnMouseMove(x, y); };
    callbacks.mouse_down = [this](int x, int y) { OnMouseDown(x, y); };
    callbacks.mouse_up = [this](int x, int y) { OnMouseUp(x, y); };
    callbacks.mouse_wheel = [this](int x, int y, float delta) { OnMouseWheel(x, y, delta); };
    callbacks.mouse_leave = [this] { OnMouseLeave(); };
    callbacks.timer = [this](UINT_PTR timer_id) { OnTimer(timer_id); };
    callbacks.system_theme_changed = [this] {
        if (theme_mode == ThemeMode::system) {
            SetThemeMode(ThemeMode::system);
        }
    };
    callbacks.theme_selected = [this](ThemeMode mode) { SetThemeMode(mode); };
    callbacks.chrome_action = [this](ChromeButton) {
        if (on_chrome_action) {
            on_chrome_action();
        }
    };

    if (!window->Create(std::move(callbacks))) {
        window.reset();
        return false;
    }

    renderer = std::make_unique<D2DRenderer>();
    if (!renderer->Initialize(window->Handle(), resolve_theme(theme_mode))) {
        window->Destroy();
        renderer.reset();
        window.reset();
        return false;
    }

    dpi = window->Dpi();
    window->SetThemeMode(theme_mode);
    renderer->SetDensity(density_mode);
    created = true;

    if (pending_content) {
        BuildRoot(std::move(pending_content));
    }
    return true;
}

void Form::Impl::BuildRoot(std::unique_ptr<Widget> content) {
    if (content == nullptr) {
        return;
    }

    auto status = std::make_unique<StatusBar>();
    status_view = status.get();
    status_view->set_left_text(ThemeText());
    UpdateStatusRight();

    auto panel = std::make_unique<FormPanel>(std::move(content), std::move(status));
    auto root = std::make_unique<PopupHost>(std::move(panel));
    popup_host = root.get();
    popup_host->set_focus_bridge(
        [this]() -> Widget* { return ui.focused(); },
        [this](Widget* widget) {
            ui.set_focus(widget);
            if (window) {
                window->Invalidate();
            }
        });
    popup_host->set_on_layout_requested([this] {
        Layout();
        if (window) {
            window->Invalidate();
        }
    });
    tooltip_owner = nullptr;
    ui.set_root(std::move(root));
    Layout();
    if (window) {
        window->Invalidate();
    }
}

void Form::Impl::SetContent(std::unique_ptr<Widget> content) {
    if (!created) {
        pending_content = std::move(content);
        return;
    }
    BuildRoot(std::move(content));
}

void Form::Impl::Layout() {
    if (!created || !window || !renderer || ui.root() == nullptr) {
        return;
    }

    RECT client{};
    if (GetClientRect(window->Handle(), &client) == FALSE) {
        return;
    }

    const UINT width = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
    const UINT height = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));
    dpi = window->Dpi();

    if (!renderer->Resize(width, height, dpi)) {
        return;
    }

    const float dip_width = static_cast<float>(width) * 96.0f / dpi;
    const float dip_height = static_cast<float>(height) * 96.0f / dpi;
    const float content_height = std::max(0.0f, dip_height - kTitleBarHeightDip);

    ui.layout(*renderer,
              Rect{0.0f, kTitleBarHeightDip, std::max(0.0f, dip_width), content_height});
    UpdateImePosition();
    UpdateStatusRight();
}

void Form::Impl::UpdateStatusRight() {
    if (status_view == nullptr) {
        return;
    }
    const int dpi_percent = static_cast<int>(dpi * 100.0f / 96.0f + 0.5f);
    status_view->set_right_text(L"CppUI v0.1 · " + std::to_wstring(dpi_percent) + L"% DPI · " +
                                DensityName());
}

void Form::Impl::SetStatusText(std::wstring text) {
    if (status_view != nullptr) {
        status_view->set_left_text(std::move(text));
    }
}

std::wstring Form::Impl::ThemeText() const {
    switch (theme_mode) {
        case ThemeMode::light:
            return L"Тема: Светлая";
        case ThemeMode::dark:
            return L"Тема: Тёмная";
        case ThemeMode::system:
        default:
            return system_prefers_dark() ? L"Тема: Системная (тёмная)"
                                         : L"Тема: Системная (светлая)";
    }
}

const wchar_t* Form::Impl::DensityName() const noexcept {
    switch (density_mode) {
        case DensityMode::compact:
            return L"Compact";
        case DensityMode::comfortable:
            return L"Comfortable";
        case DensityMode::cozy:
        default:
            return L"Cozy";
    }
}

void Form::Impl::SetThemeMode(ThemeMode mode) {
    theme_mode = mode;
    if (!created || !renderer || !window) {
        return;
    }

    if (!renderer->SetTheme(resolve_theme(theme_mode))) {
        return;
    }
    window->SetThemeMode(theme_mode);
    SetStatusText(ThemeText());
    Layout();
    window->Invalidate();
    if (on_theme_changed) {
        on_theme_changed(theme_mode);
    }
}

void Form::Impl::SetDensityMode(DensityMode mode) {
    density_mode = mode;
    if (!created || !renderer) {
        return;
    }

    renderer->SetDensity(density_mode);
    SetStatusText(L"Плотность: " + std::wstring(DensityName()));
    Layout();
    if (window) {
        window->Invalidate();
    }
    if (on_density_changed) {
        on_density_changed(density_mode);
    }
}

void Form::Impl::Paint() {
    if (!created || !renderer || !window) {
        return;
    }
    (void)renderer->Render(window->GetChromeState(), ui.root());
    UpdateAnimationTimer();
}

void Form::Impl::UpdateAnimationTimer() {
    if (!created || !window) {
        return;
    }
    if (ui.has_animation()) {
        if (!animation_timer_active) {
            last_tick = std::chrono::steady_clock::now();
            animation_timer_active = true;
            window->StartTimer(kAnimationTimer, kAnimationIntervalMs);
        }
    } else if (animation_timer_active) {
        animation_timer_active = false;
        window->StopTimer(kAnimationTimer);
    }
}

void Form::Impl::UpdateImePosition() {
    if (!window) {
        return;
    }
    Widget* const focused = ui.focused();
    if (focused == nullptr) {
        return;
    }
    const Rect caret = focused->ime_caret_rect();
    if (caret.width <= 0.0f || caret.height <= 0.0f) {
        return;
    }
    const float scale = dpi / 96.0f;
    window->SetImeCaretRect(static_cast<int>(std::lround(caret.x * scale)),
                            static_cast<int>(std::lround(caret.y * scale)),
                            static_cast<int>(std::lround(caret.height * scale)));
}

void Form::Impl::HideTooltip() {
    if (popup_host != nullptr) {
        popup_host->hide_tooltip();
    }
    tooltip_owner = nullptr;
}

void Form::Impl::OnTimer(UINT_PTR timer_id) {
    if (!created || !window) {
        return;
    }
    if (timer_id == kTooltipTimer) {
        window->StopTimer(kTooltipTimer);
        if (tooltip_owner != nullptr && !tooltip_owner->tooltip().empty() &&
            popup_host != nullptr && !popup_host->is_open()) {
            popup_host->show_tooltip(tooltip_owner->tooltip(), last_mouse);
        }
        return;
    }
    if (timer_id != kAnimationTimer) {
        return;
    }
    if (!ui.has_animation()) {
        animation_timer_active = false;
        window->StopTimer(kAnimationTimer);
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const float seconds =
        std::chrono::duration<float>(now - last_tick).count();
    last_tick = now;
    ui.tick(std::clamp(seconds, 0.0f, 0.05f));
    window->Invalidate();
}

Point Form::Impl::ToDip(int x, int y) const {
    return Point{static_cast<float>(x) * 96.0f / dpi,
                 static_cast<float>(y) * 96.0f / dpi};
}

void Form::Impl::OnResize(std::uint32_t width, std::uint32_t height, float new_dpi) {
    (void)width;
    (void)height;
    dpi = new_dpi;
    if (!created || !renderer || !window) {
        return;
    }
    HideTooltip();
    Layout();
    window->Invalidate();
}

void Form::Impl::OnDpiChanged(float new_dpi) {
    dpi = new_dpi;
    if (!created || !renderer || !window) {
        return;
    }
    HideTooltip();
    Layout();
    window->Invalidate();
}

void Form::Impl::OnMouseMove(int x, int y) {
    if (!created) {
        return;
    }
    last_mouse = ToDip(x, y);
    ui.mouse_move(last_mouse);

    if (window) {
        Widget* const hovered = ui.hovered();
        if (hovered != tooltip_owner) {
            window->StopTimer(kTooltipTimer);
            HideTooltip();
            tooltip_owner = hovered;
            if (hovered != nullptr && !hovered->tooltip().empty() &&
                (popup_host == nullptr || !popup_host->is_open())) {
                window->StartTimer(kTooltipTimer, kTooltipDelayMs);
            }
        }
        window->Invalidate();
    }
}

void Form::Impl::OnMouseDown(int x, int y) {
    if (!created) {
        return;
    }
    if (window) {
        window->StopTimer(kTooltipTimer);
    }
    HideTooltip();
    ui.mouse_down(ToDip(x, y));
    UpdateImePosition();
    if (window) {
        window->Invalidate();
    }
}

void Form::Impl::OnMouseUp(int x, int y) {
    if (!created) {
        return;
    }
    ui.mouse_up(ToDip(x, y));
    if (window) {
        window->Invalidate();
    }
}

void Form::Impl::OnMouseLeave() {
    if (!created) {
        return;
    }
    if (window) {
        window->StopTimer(kTooltipTimer);
    }
    HideTooltip();
    ui.mouse_leave();
    if (window) {
        window->Invalidate();
    }
}

void Form::Impl::OnMouseWheel(int x, int y, float delta) {
    if (!created) {
        return;
    }
    HideTooltip();
    if (ui.mouse_wheel(ToDip(x, y), delta) && window) {
        window->Invalidate();
    }
}

bool Form::Impl::OnKeyDown(WPARAM key) {
    if (key == VK_F5 && renderer && created) {
        if (renderer->RecreateDevice()) {
            Layout();
            if (window) {
                window->Invalidate();
            }
        }
        return true;
    }

    if (created) {
        KeyModifiers modifiers{};
        modifiers.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        modifiers.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (ui.key_down(static_cast<int>(key), modifiers)) {
            HideTooltip();
            UpdateImePosition();
            if (window) {
                window->Invalidate();
            }
            return true;
        }
        if (key == VK_ESCAPE && popup_host != nullptr && popup_host->is_open()) {
            popup_host->close();
            if (window) {
                window->Invalidate();
            }
            return true;
        }
    }
    return false;
}

bool Form::Impl::OnChar(wchar_t character) {
    if (!created || !ui.char_input(character)) {
        return false;
    }
    HideTooltip();
    UpdateImePosition();
    if (window) {
        window->Invalidate();
    }
    return true;
}

bool Form::Impl::CapturePng(std::wstring_view path) {
    if (!created || !renderer || !window) {
        return false;
    }
    Layout();
    return renderer->RenderAndCapture(window->GetChromeState(), ui.root(), path);
}

void Form::Impl::Shutdown() {
    if (renderer) {
        renderer->Shutdown();
    }
    if (window) {
        window->Destroy();
    }
    ui.set_root(nullptr);
    popup_host = nullptr;
    status_view = nullptr;
    tooltip_owner = nullptr;
    animation_timer_active = false;
    pending_content.reset();
    window.reset();
    renderer.reset();
    created = false;
}

Form::Form(std::wstring title, int width_dip, int height_dip, int min_width_dip, int min_height_dip)
    : impl_(std::make_unique<Impl>(std::move(title),
                                   width_dip,
                                   height_dip,
                                   min_width_dip,
                                   min_height_dip)) {}

Form::~Form() {
    if (impl_) {
        impl_->Shutdown();
    }
}

bool Form::Create() {
    return impl_ != nullptr && impl_->Create();
}

void Form::SetContent(std::unique_ptr<Widget> content) {
    if (impl_) {
        impl_->SetContent(std::move(content));
    }
}

StatusBar* Form::status_bar() noexcept {
    return impl_ != nullptr ? impl_->status_view : nullptr;
}

const StatusBar* Form::status_bar() const noexcept {
    return impl_ != nullptr ? impl_->status_view : nullptr;
}

PopupHost* Form::popups() noexcept {
    return impl_ != nullptr ? impl_->popup_host : nullptr;
}

const PopupHost* Form::popups() const noexcept {
    return impl_ != nullptr ? impl_->popup_host : nullptr;
}

void Form::SetStatusText(std::wstring text) {
    if (impl_) {
        impl_->SetStatusText(std::move(text));
    }
}

void Form::SetThemeMode(ThemeMode mode) {
    if (impl_) {
        impl_->SetThemeMode(mode);
    }
}

ThemeMode Form::theme_mode() const noexcept {
    return impl_ != nullptr ? impl_->theme_mode : ThemeMode::light;
}

void Form::SetDensityMode(DensityMode mode) {
    if (impl_) {
        impl_->SetDensityMode(mode);
    }
}

DensityMode Form::density_mode() const noexcept {
    return impl_ != nullptr ? impl_->density_mode : DensityMode::cozy;
}

void Form::SetOnThemeChanged(std::function<void(ThemeMode)> callback) {
    if (impl_) {
        impl_->on_theme_changed = std::move(callback);
    }
}

void Form::SetOnDensityChanged(std::function<void(DensityMode)> callback) {
    if (impl_) {
        impl_->on_density_changed = std::move(callback);
    }
}

void Form::SetOnChromeAction(std::function<void()> callback) {
    if (impl_) {
        impl_->on_chrome_action = std::move(callback);
    }
}

void Form::Show() {
    if (!impl_) {
        return;
    }
    if (!impl_->Create()) {
        return;
    }
    impl_->Layout();
    impl_->window->Show(SW_SHOWNORMAL);
}

int Form::Run() {
    if (!impl_ || !impl_->Create()) {
        return 1;
    }
    impl_->Layout();
    const int result = impl_->window->Run();
    impl_->Shutdown();
    return result;
}

void Form::Invalidate() {
    if (impl_ && impl_->window) {
        impl_->window->Invalidate();
    }
}

void Form::Close() {
    if (impl_ && impl_->window) {
        impl_->window->Destroy();
    }
}

bool Form::CapturePng(std::wstring_view path) {
    return impl_ != nullptr && impl_->CapturePng(path);
}

bool Form::IsCreated() const noexcept {
    return impl_ != nullptr && impl_->created;
}

void* Form::NativeHandle() const noexcept {
    return impl_ != nullptr && impl_->window ? impl_->window->Handle() : nullptr;
}

}  // namespace cppui
