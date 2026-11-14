#include <windows.h>

#include <objbase.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#include <cppui/theme/mode.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/widget.hpp>
#include <cppui/ui/widgets.hpp>

#include "platform/win32/win32_window.hpp"
#include "render/d2d/d2d_renderer.hpp"

namespace {

constexpr wchar_t kWindowTitle[] = L"CppUI - Phase 3";
constexpr wchar_t kErrorTitle[] = L"CppUI - Phase 3";
constexpr int kInitialWidthDip = 1000;
constexpr int kInitialHeightDip = 640;
constexpr int kMinWidthDip = 720;
constexpr int kMinHeightDip = 480;

class BootstrapApp final {
public:
    int Run(HINSTANCE instance, int show_command, bool self_test);

private:
    void BuildUi();
    void LayoutUi();
    void LayoutUiForSize(float width, float height);
    void ApplyTheme();
    void SetThemeMode(cppui::ThemeMode mode);
    void CountClick();
    void StopRenderer();
    int RunSelfTest(cppui::Win32Window& window);

    void OnPaint();
    void OnResize(std::uint32_t width, std::uint32_t height, float dpi);
    void OnDpiChanged(float dpi);
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnMouseUp(int x, int y);
    void OnMouseLeave();
    bool OnKeyDown(WPARAM key);

    [[nodiscard]] cppui::Point ToDip(int x, int y) const;

    cppui::Win32Window* window_{};
    cppui::D2DRenderer renderer_;
    cppui::UiHost ui_;
    cppui::Label* click_label_{};
    cppui::Label* theme_label_{};
    cppui::ThemeMode theme_mode_{cppui::ThemeMode::light};
    float dpi_{96.0f};
    int click_count_{};
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
    callbacks.mouse_move = [this](int x, int y) { OnMouseMove(x, y); };
    callbacks.mouse_down = [this](int x, int y) { OnMouseDown(x, y); };
    callbacks.mouse_up = [this](int x, int y) { OnMouseUp(x, y); };
    callbacks.mouse_leave = [this] { OnMouseLeave(); };
    callbacks.system_theme_changed = [this] {
        if (theme_mode_ == cppui::ThemeMode::system) {
            ApplyTheme();
        }
    };

    if (!window.Create(callbacks)) {
        MessageBoxW(nullptr, L"Win32 window creation failed.", kErrorTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    window_ = &window;
    dpi_ = window.Dpi();

    if (!renderer_.Initialize(window.Handle(), cppui::light_theme())) {
        MessageBoxW(window.Handle(),
                    L"Direct2D/D3D11 renderer initialization failed. See the debug output for details.",
                    kErrorTitle,
                    MB_OK | MB_ICONERROR);
        window_ = nullptr;
        return 1;
    }
    renderer_ready_ = true;

    BuildUi();
    ApplyTheme();

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

void BootstrapApp::BuildUi() {
    auto root = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 10.0f);
    root->set_padding(cppui::Insets{48.0f, 28.0f, 48.0f, 28.0f});
    root->set_cross_align(cppui::CrossAlign::center);

    auto title = std::make_unique<cppui::Label>(L"CppUI",
                                                cppui::FontRole::title,
                                                cppui::LabelStyle::primary);
    root->add_child(std::move(title));

    auto subtitle = std::make_unique<cppui::Label>(L"Phase 3 - retained-mode UI",
                                                   cppui::FontRole::body,
                                                   cppui::LabelStyle::secondary);
    root->add_child(std::move(subtitle));

    auto card = std::make_unique<cppui::CardPanel>();
    card->set_preferred_width(620.0f);
    card->set_padding(cppui::Insets{28.0f, 26.0f, 28.0f, 26.0f});

    auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 10.0f);
    content->set_cross_align(cppui::CrossAlign::stretch);

    content->add_child(std::make_unique<cppui::Label>(L"ТЕМА",
                                                      cppui::FontRole::caption,
                                                      cppui::LabelStyle::muted));
    auto theme_row = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 8.0f);
    auto light_button = std::make_unique<cppui::Button>(L"Светлая", cppui::ButtonStyle::secondary);
    light_button->set_on_click([this] { SetThemeMode(cppui::ThemeMode::light); });
    theme_row->add_child(std::move(light_button));
    auto dark_button = std::make_unique<cppui::Button>(L"Тёмная", cppui::ButtonStyle::secondary);
    dark_button->set_on_click([this] { SetThemeMode(cppui::ThemeMode::dark); });
    theme_row->add_child(std::move(dark_button));
    auto system_button = std::make_unique<cppui::Button>(L"Системная", cppui::ButtonStyle::secondary);
    system_button->set_on_click([this] { SetThemeMode(cppui::ThemeMode::system); });
    theme_row->add_child(std::move(system_button));
    content->add_child(std::move(theme_row));

    content->add_child(std::make_unique<cppui::Label>(L"КНОПКИ",
                                                      cppui::FontRole::caption,
                                                      cppui::LabelStyle::muted));
    auto button_row = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 8.0f);
    auto primary = std::make_unique<cppui::Button>(L"Основная", cppui::ButtonStyle::primary);
    primary->set_on_click([this] { CountClick(); });
    button_row->add_child(std::move(primary));
    auto secondary = std::make_unique<cppui::Button>(L"Вторичная", cppui::ButtonStyle::secondary);
    secondary->set_on_click([this] { CountClick(); });
    button_row->add_child(std::move(secondary));
    auto ghost = std::make_unique<cppui::Button>(L"Прозрачная", cppui::ButtonStyle::ghost);
    ghost->set_on_click([this] { CountClick(); });
    button_row->add_child(std::move(ghost));
    auto disabled = std::make_unique<cppui::Button>(L"Недоступна", cppui::ButtonStyle::secondary);
    disabled->set_enabled(false);
    button_row->add_child(std::move(disabled));
    content->add_child(std::move(button_row));

    auto click_label = std::make_unique<cppui::Label>(L"Нажатий: 0",
                                                      cppui::FontRole::body,
                                                      cppui::LabelStyle::secondary);
    click_label_ = click_label.get();
    content->add_child(std::move(click_label));

    auto theme_label = std::make_unique<cppui::Label>(L"Тема: Светлая",
                                                      cppui::FontRole::caption,
                                                      cppui::LabelStyle::muted);
    theme_label_ = theme_label.get();
    content->add_child(std::move(theme_label));

    content->add_child(std::make_unique<cppui::Label>(
        L"Tab / Shift+Tab - фокус, Space / Enter - нажатие",
        cppui::FontRole::caption,
        cppui::LabelStyle::muted));

    card->add_child(std::move(content));
    root->add_child(std::move(card));

    auto hint = std::make_unique<cppui::Label>(L"Retained-mode Widget / StackPanel / Button",
                                               cppui::FontRole::caption,
                                               cppui::LabelStyle::muted);
    root->add_child(std::move(hint));

    ui_.set_root(std::move(root));
}

void BootstrapApp::LayoutUi() {
    if (!renderer_ready_ || window_ == nullptr) {
        return;
    }

    RECT client{};
    if (GetClientRect(window_->Handle(), &client) == FALSE) {
        return;
    }

    const float width = static_cast<float>(client.right - client.left) * 96.0f / dpi_;
    const float height = static_cast<float>(client.bottom - client.top) * 96.0f / dpi_;
    LayoutUiForSize(width, height);
}

void BootstrapApp::LayoutUiForSize(float width, float height) {
    if (!renderer_ready_) {
        return;
    }

    const float content_height = std::max(0.0f, height - cppui::kTitleBarHeightDip);
    ui_.layout(renderer_,
               cppui::Rect{0.0f, cppui::kTitleBarHeightDip, std::max(0.0f, width), content_height});
}

void BootstrapApp::ApplyTheme() {
    if (!renderer_ready_) {
        return;
    }

    const cppui::ThemeTokens& tokens = cppui::resolve_theme(theme_mode_);
    if (!renderer_.SetTheme(tokens)) {
        StopRenderer();
        return;
    }

    if (theme_label_ != nullptr) {
        switch (theme_mode_) {
            case cppui::ThemeMode::light:
                theme_label_->set_text(L"Тема: Светлая");
                break;
            case cppui::ThemeMode::dark:
                theme_label_->set_text(L"Тема: Тёмная");
                break;
            case cppui::ThemeMode::system:
                theme_label_->set_text(cppui::system_prefers_dark()
                                            ? L"Тема: Системная (тёмная)"
                                            : L"Тема: Системная (светлая)");
                break;
        }
    }

    LayoutUi();
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::SetThemeMode(cppui::ThemeMode mode) {
    theme_mode_ = mode;
    ApplyTheme();
}

void BootstrapApp::CountClick() {
    ++click_count_;
    if (click_label_ != nullptr) {
        click_label_->set_text(L"Нажатий: " + std::to_wstring(click_count_));
    }
    LayoutUi();
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

cppui::Point BootstrapApp::ToDip(int x, int y) const {
    return cppui::Point{static_cast<float>(x) * 96.0f / dpi_,
                        static_cast<float>(y) * 96.0f / dpi_};
}

void BootstrapApp::OnMouseMove(int x, int y) {
    if (!renderer_ready_) {
        return;
    }
    ui_.mouse_move(ToDip(x, y));
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::OnMouseDown(int x, int y) {
    if (!renderer_ready_) {
        return;
    }
    ui_.mouse_down(ToDip(x, y));
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::OnMouseUp(int x, int y) {
    if (!renderer_ready_) {
        return;
    }
    ui_.mouse_up(ToDip(x, y));
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::OnMouseLeave() {
    if (!renderer_ready_) {
        return;
    }
    ui_.mouse_leave();
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::OnPaint() {
    if (!renderer_ready_ || window_ == nullptr) {
        return;
    }

    const cppui::ChromeState chrome = window_->GetChromeState();
    if (renderer_.Render(chrome, ui_.root())) {
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
    dpi_ = dpi;
    if (!renderer_ready_) {
        return;
    }

    if (!renderer_.Resize(width, height, dpi)) {
        if (!renderer_.IsInitialized()) {
            StopRenderer();
            return;
        }
    }

    LayoutUiForSize(static_cast<float>(width) * 96.0f / dpi,
                    static_cast<float>(height) * 96.0f / dpi);
}

void BootstrapApp::OnDpiChanged(float dpi) {
    dpi_ = dpi;
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
            return;
        }
    }

    LayoutUiForSize(static_cast<float>(width) * 96.0f / dpi,
                    static_cast<float>(height) * 96.0f / dpi);
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
        if (renderer_ready_) {
            LayoutUi();
        }
        if (window_ != nullptr && renderer_ready_) {
            window_->Invalidate();
        }
        return true;
    }

    if (renderer_ready_) {
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (ui_.key_down(static_cast<int>(key), shift)) {
            if (window_ != nullptr) {
                window_->Invalidate();
            }
            return true;
        }
    }

    return false;
}

int BootstrapApp::RunSelfTest(cppui::Win32Window& window) {
    const cppui::ChromeState chrome{
        true,
        false,
        cppui::ChromeButton::none,
        cppui::ChromeButton::none,
    };

    if (!renderer_.Render(chrome, ui_.root())) {
        OutputDebugStringW(L"CppUI self-test: initial UI render failed.\n");
        return 2;
    }

    if (!renderer_.Resize(800, 500, window.Dpi())) {
        OutputDebugStringW(L"CppUI self-test: resize failed.\n");
        return 3;
    }

    LayoutUiForSize(800.0f, 500.0f);
    if (!renderer_.Render(chrome, ui_.root())) {
        OutputDebugStringW(L"CppUI self-test: render after resize failed.\n");
        return 4;
    }

    if (!renderer_.RecreateDevice()) {
        OutputDebugStringW(L"CppUI self-test: device recreation failed.\n");
        return 5;
    }

    if (!renderer_.Render(chrome, ui_.root())) {
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
