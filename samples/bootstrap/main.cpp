#include <windows.h>

#include <objbase.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#include <cppui/theme/mode.hpp>
#include <cppui/ui/form.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/widget.hpp>
#include <cppui/ui/widgets.hpp>

#include "platform/win32/win32_window.hpp"
#include "render/d2d/d2d_renderer.hpp"

namespace {

constexpr wchar_t kWindowTitle[] = L"CppUI - Phase 4";
constexpr wchar_t kErrorTitle[] = L"CppUI - Phase 4";
constexpr int kInitialWidthDip = 1000;
constexpr int kInitialHeightDip = 640;
constexpr int kMinWidthDip = 900;
constexpr int kMinHeightDip = 580;

class BootstrapApp final {
public:
    int Run(HINSTANCE instance, int show_command, bool self_test);

private:
    void BuildUi();
    void LayoutUi();
    void LayoutUiForSize(float width, float height);
    void ApplyTheme();
    void SetThemeMode(cppui::ThemeMode mode);
    void SetDensityMode(cppui::DensityMode mode);
    void CountClick();
    void ResetCounter();
    void SetStatusText(std::wstring text);
    void UpdateStatusRight();
    [[nodiscard]] const wchar_t* DensityName() const noexcept;
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
    cppui::Label* density_label_{};
    cppui::StatusBar* status_bar_{};
    cppui::ThemeMode theme_mode_{cppui::ThemeMode::light};
    cppui::DensityMode density_mode_{cppui::DensityMode::cozy};
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
    callbacks.theme_selected = [this](cppui::ThemeMode mode) { SetThemeMode(mode); };
    callbacks.chrome_action = [this](cppui::ChromeButton button) {
        if (button == cppui::ChromeButton::action_reset) {
            ResetCounter();
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
    auto root = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 14.0f);
    root->set_padding(cppui::Insets{40.0f, 26.0f, 40.0f, 22.0f});
    root->set_cross_align(cppui::CrossAlign::center);

    auto header = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 4.0f);
    header->set_cross_align(cppui::CrossAlign::center);
    header->add_child(std::make_unique<cppui::Label>(L"CppUI Workspace",
                                                     cppui::FontRole::title,
                                                     cppui::LabelStyle::primary));
    header->add_child(std::make_unique<cppui::Label>(L"Phase 4 - form preview",
                                                     cppui::FontRole::body,
                                                     cppui::LabelStyle::secondary));
    root->add_child(std::move(header));

    auto body = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 16.0f);
    body->set_cross_align(cppui::CrossAlign::stretch);

    auto sidebar = std::make_unique<cppui::CardPanel>();
    sidebar->set_preferred_width(220.0f);
    sidebar->set_padding(cppui::Insets{18.0f, 18.0f, 18.0f, 18.0f});

    auto navigation = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 8.0f);
    navigation->set_cross_align(cppui::CrossAlign::stretch);
    navigation->add_child(std::make_unique<cppui::Label>(L"РАЗДЕЛЫ",
                                                         cppui::FontRole::caption,
                                                         cppui::LabelStyle::muted));
    auto overview = std::make_unique<cppui::Button>(L"Обзор", cppui::ButtonStyle::secondary);
    overview->set_on_click([this] { SetStatusText(L"Раздел: Обзор"); });
    navigation->add_child(std::move(overview));
    auto appearance = std::make_unique<cppui::Button>(L"Оформление",
                                                      cppui::ButtonStyle::secondary);
    appearance->set_on_click([this] { SetStatusText(L"Раздел: Оформление"); });
    navigation->add_child(std::move(appearance));
    auto about = std::make_unique<cppui::Button>(L"О приложении",
                                                 cppui::ButtonStyle::ghost);
    about->set_on_click([this] { SetStatusText(L"CppUI M0 preview"); });
    navigation->add_child(std::move(about));
    navigation->add_child(std::make_unique<cppui::Label>(L"M0 · Technical Preview",
                                                         cppui::FontRole::caption,
                                                         cppui::LabelStyle::muted));
    sidebar->add_child(std::move(navigation));
    body->add_child(std::move(sidebar));

    auto card = std::make_unique<cppui::CardPanel>();
    card->set_preferred_width(560.0f);
    card->set_padding(cppui::Insets{26.0f, 24.0f, 26.0f, 24.0f});

    auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 10.0f);
    content->set_cross_align(cppui::CrossAlign::stretch);

    content->add_child(std::make_unique<cppui::Label>(L"ОФОРМЛЕНИЕ",
                                                      cppui::FontRole::caption,
                                                      cppui::LabelStyle::muted));
    auto theme_label = std::make_unique<cppui::Label>(L"Тема: Светлая",
                                                      cppui::FontRole::caption,
                                                      cppui::LabelStyle::muted);
    theme_label_ = theme_label.get();
    content->add_child(std::move(theme_label));

    auto density_label = std::make_unique<cppui::Label>(L"Плотность: Обычная",
                                                        cppui::FontRole::caption,
                                                        cppui::LabelStyle::muted);
    density_label_ = density_label.get();
    content->add_child(std::move(density_label));

    auto density_row = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 8.0f);
    auto compact_button = std::make_unique<cppui::Button>(L"Компактно",
                                                          cppui::ButtonStyle::secondary);
    compact_button->set_on_click([this] { SetDensityMode(cppui::DensityMode::compact); });
    density_row->add_child(std::move(compact_button));
    auto cozy_button = std::make_unique<cppui::Button>(L"Обычно", cppui::ButtonStyle::secondary);
    cozy_button->set_on_click([this] { SetDensityMode(cppui::DensityMode::cozy); });
    density_row->add_child(std::move(cozy_button));
    auto comfortable_button = std::make_unique<cppui::Button>(L"Свободно",
                                                              cppui::ButtonStyle::secondary);
    comfortable_button->set_on_click([this] { SetDensityMode(cppui::DensityMode::comfortable); });
    density_row->add_child(std::move(comfortable_button));
    content->add_child(std::move(density_row));

    content->add_child(std::make_unique<cppui::Label>(L"ДЕЙСТВИЯ",
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

    content->add_child(std::make_unique<cppui::Label>(
        L"Tab / Shift+Tab - фокус, Space / Enter - нажатие",
        cppui::FontRole::caption,
        cppui::LabelStyle::muted));

    card->add_child(std::move(content));
    body->add_child(std::move(card));
    root->add_child(std::move(body));

    auto status = std::make_unique<cppui::StatusBar>();
    status->set_left_text(L"Готово");
    status_bar_ = status.get();

    auto form = std::make_unique<cppui::FormPanel>(std::move(root), std::move(status));
    ui_.set_root(std::move(form));

    UpdateStatusRight();
}

void BootstrapApp::LayoutUi() {
    if (!renderer_ready_ || window_ == nullptr) {
        return;
    }

    RECT client{};
    if (GetClientRect(window_->Handle(), &client) == FALSE) {
        return;
    }

    const UINT client_width = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
    const UINT client_height = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));
    if (!renderer_.Resize(client_width, client_height, dpi_)) {
        if (!renderer_.IsInitialized()) {
            StopRenderer();
            return;
        }
    }

    const float width = static_cast<float>(client_width) * 96.0f / dpi_;
    const float height = static_cast<float>(client_height) * 96.0f / dpi_;
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

    if (window_ != nullptr) {
        window_->SetThemeMode(theme_mode_);
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
        SetStatusText(theme_label_->text());
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

void BootstrapApp::SetDensityMode(cppui::DensityMode mode) {
    density_mode_ = mode;
    renderer_.SetDensity(mode);

    if (density_label_ != nullptr) {
        switch (mode) {
            case cppui::DensityMode::compact:
                density_label_->set_text(L"Плотность: Компактная");
                break;
            case cppui::DensityMode::comfortable:
                density_label_->set_text(L"Плотность: Свободная");
                break;
            case cppui::DensityMode::cozy:
            default:
                density_label_->set_text(L"Плотность: Обычная");
                break;
        }
        SetStatusText(density_label_->text());
    }

    UpdateStatusRight();
    LayoutUi();
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::CountClick() {
    ++click_count_;
    if (click_label_ != nullptr) {
        click_label_->set_text(L"Нажатий: " + std::to_wstring(click_count_));
    }
    SetStatusText(L"Нажатий: " + std::to_wstring(click_count_));
    LayoutUi();
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::ResetCounter() {
    click_count_ = 0;
    if (click_label_ != nullptr) {
        click_label_->set_text(L"Нажатий: 0");
    }
    SetStatusText(L"Счётчик сброшен");
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::SetStatusText(std::wstring text) {
    if (status_bar_ == nullptr) {
        return;
    }
    status_bar_->set_left_text(std::move(text));
    if (window_ != nullptr) {
        window_->Invalidate();
    }
}

void BootstrapApp::UpdateStatusRight() {
    if (status_bar_ == nullptr) {
        return;
    }

    const int dpi_percent = static_cast<int>(dpi_ * 100.0f / 96.0f + 0.5f);
    status_bar_->set_right_text(L"CppUI M0 · " + std::to_wstring(dpi_percent) + L"% DPI · " +
                                DensityName());
}

const wchar_t* BootstrapApp::DensityName() const noexcept {
    switch (density_mode_) {
        case cppui::DensityMode::compact:
            return L"Compact";
        case cppui::DensityMode::comfortable:
            return L"Comfortable";
        case cppui::DensityMode::cozy:
        default:
            return L"Cozy";
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

    UpdateStatusRight();
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

    UpdateStatusRight();
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
