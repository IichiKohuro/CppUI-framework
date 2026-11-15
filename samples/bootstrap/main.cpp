#include <windows.h>

#include <objbase.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

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
    int Run(HINSTANCE instance,
            int show_command,
            bool self_test,
            bool benchmark,
            bool capture_visual);

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
    int RunBenchmark(cppui::Win32Window& window);
    int RunVisualCapture(cppui::Win32Window& window);

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
    std::chrono::steady_clock::time_point run_start_{};
    int click_count_{};
    bool renderer_ready_{false};
    int paint_failures_{0};
};

int BootstrapApp::Run(HINSTANCE instance,
                      int show_command,
                      bool self_test,
                      bool benchmark,
                      bool capture_visual) {
    run_start_ = std::chrono::steady_clock::now();
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
    if (benchmark) {
        return RunBenchmark(window);
    }
    if (capture_visual) {
        return RunVisualCapture(window);
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

    // The swap-chain target can lag a Win32 client resize; re-sync before layout
    // so DIP hit-testing and painted coordinates stay identical.
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
    cppui::ChromeState chrome{};
    chrome.active = true;

    if (!renderer_.Render(chrome, ui_.root())) {
        OutputDebugStringW(L"CppUI self-test: initial UI render failed.\n");
        return 2;
    }

    struct MatrixCase {
        float width_dip;
        float height_dip;
        float dpi;
        cppui::DensityMode density;
        cppui::ThemeMode theme;
    };

    const MatrixCase cases[] = {
        {1000.0f, 640.0f, 96.0f, cppui::DensityMode::cozy, cppui::ThemeMode::light},
        {1000.0f, 640.0f, 120.0f, cppui::DensityMode::cozy, cppui::ThemeMode::dark},
        {900.0f, 580.0f, 150.0f, cppui::DensityMode::compact, cppui::ThemeMode::light},
        {1120.0f, 720.0f, 200.0f, cppui::DensityMode::comfortable, cppui::ThemeMode::dark},
    };

    int case_index = 0;
    for (const MatrixCase& test_case : cases) {
        const float scale = test_case.dpi / 96.0f;
        const UINT width = static_cast<UINT>(std::lround(test_case.width_dip * scale));
        const UINT height = static_cast<UINT>(std::lround(test_case.height_dip * scale));

        if (!renderer_.SetTheme(cppui::resolve_theme(test_case.theme))) {
            OutputDebugStringW(L"CppUI self-test: DPI matrix theme failed.\n");
            return 10 + case_index;
        }
        renderer_.SetDensity(test_case.density);
        if (!renderer_.Resize(width, height, test_case.dpi)) {
            OutputDebugStringW(L"CppUI self-test: DPI matrix resize failed.\n");
            return 10 + case_index;
        }

        LayoutUiForSize(test_case.width_dip, test_case.height_dip);
        const cppui::Size target = renderer_.TargetSizeDip();
        if (std::fabs(target.width - test_case.width_dip) > 1.5f ||
            std::fabs(target.height - test_case.height_dip) > 1.5f) {
            OutputDebugStringW(L"CppUI self-test: target DIP size mismatch.\n");
            return 20 + case_index;
        }

        if (!renderer_.Render(chrome, ui_.root())) {
            OutputDebugStringW(L"CppUI self-test: DPI matrix render failed.\n");
            return 30 + case_index;
        }
        ++case_index;
    }

    if (!renderer_.SetTheme(cppui::light_theme())) {
        OutputDebugStringW(L"CppUI self-test: restore theme failed.\n");
        return 40;
    }
    renderer_.SetDensity(cppui::DensityMode::cozy);
    if (!renderer_.Resize(1250, 800, 120.0f)) {
        OutputDebugStringW(L"CppUI self-test: restore resize failed.\n");
        return 40;
    }
    LayoutUiForSize(1000.0f, 640.0f);

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
    OutputDebugStringW(L"CppUI self-test: passed (4 DPI/density/theme cases).\n");
    return 0;
}

int BootstrapApp::RunBenchmark(cppui::Win32Window& window) {
    cppui::ChromeState chrome{};
    chrome.active = true;
    using Clock = std::chrono::steady_clock;
    const auto elapsed_ms = [](Clock::time_point start, Clock::time_point end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    };

    if (!renderer_.Resize(1250, 800, 120.0f)) {
        OutputDebugStringW(L"CppUI benchmark: initial resize failed.\n");
        return 2;
    }
    LayoutUiForSize(1000.0f, 640.0f);

    double startup_ms = 0.0;
    for (int warmup = 0; warmup < 10; ++warmup) {
        if (!renderer_.Render(chrome, ui_.root())) {
            OutputDebugStringW(L"CppUI benchmark: warmup render failed.\n");
            return 3;
        }
        if (warmup == 0) {
            startup_ms = elapsed_ms(run_start_, Clock::now());
        }
    }



    FILETIME creation_start{};
    FILETIME exit_start{};
    FILETIME cpu_kernel_start{};
    FILETIME cpu_user_start{};
    FILETIME creation_end{};
    FILETIME exit_end{};
    FILETIME cpu_kernel_end{};
    FILETIME cpu_user_end{};
    (void)GetProcessTimes(GetCurrentProcess(),
                          &creation_start,
                          &exit_start,
                          &cpu_kernel_start,
                          &cpu_user_start);

    std::vector<double> frame_times;
    frame_times.reserve(120);
    for (int frame = 0; frame < 120; ++frame) {
        const auto start = Clock::now();
        if (!renderer_.Render(chrome, ui_.root())) {
            OutputDebugStringW(L"CppUI benchmark: frame render failed.\n");
            return 4;
        }
        frame_times.push_back(elapsed_ms(start, Clock::now()));
    }

    (void)GetProcessTimes(GetCurrentProcess(),
                          &creation_end,
                          &exit_end,
                          &cpu_kernel_end,
                          &cpu_user_end);

    const auto filetime_ms = [](const FILETIME& time) {
        ULARGE_INTEGER value{};
        value.LowPart = time.dwLowDateTime;
        value.HighPart = time.dwHighDateTime;
        return static_cast<double>(value.QuadPart) / 10000.0;
    };
    const double cpu_ms_per_frame =
        ((filetime_ms(cpu_kernel_end) + filetime_ms(cpu_user_end)) -
         (filetime_ms(cpu_kernel_start) + filetime_ms(cpu_user_start))) /
        static_cast<double>(frame_times.size());

    const auto layout_start = Clock::now();
    for (int iteration = 0; iteration < 500; ++iteration) {
        LayoutUiForSize(1000.0f, 640.0f);
    }
    const double layout_avg_ms = elapsed_ms(layout_start, Clock::now()) / 500.0;

    const struct ResizeCase {
        float width_dip;
        float height_dip;
    } resize_cases[] = {
        {900.0f, 580.0f},
        {1000.0f, 640.0f},
        {1120.0f, 720.0f},
        {1280.0f, 800.0f},
        {1600.0f, 900.0f},
    };

    double resize_total_ms = 0.0;
    int resize_frames = 0;
    for (const ResizeCase& resize_case : resize_cases) {
        const UINT width = static_cast<UINT>(std::lround(resize_case.width_dip * 1.25f));
        const UINT height = static_cast<UINT>(std::lround(resize_case.height_dip * 1.25f));
        const auto start = Clock::now();
        if (!renderer_.Resize(width, height, 120.0f)) {
            OutputDebugStringW(L"CppUI benchmark: resize failed.\n");
            return 5;
        }
        LayoutUiForSize(resize_case.width_dip, resize_case.height_dip);
        for (int frame = 0; frame < 5; ++frame) {
            if (!renderer_.Render(chrome, ui_.root())) {
                OutputDebugStringW(L"CppUI benchmark: resize render failed.\n");
                return 6;
            }
        }
        resize_total_ms += elapsed_ms(start, Clock::now());
        resize_frames += 5;
    }
    const double resize_avg_ms = resize_total_ms / static_cast<double>(resize_frames);

    if (!renderer_.Resize(1250, 800, 120.0f)) {
        OutputDebugStringW(L"CppUI benchmark: restore resize failed.\n");
        return 7;
    }
    LayoutUiForSize(1000.0f, 640.0f);

    const auto theme_start = Clock::now();
    for (int iteration = 0; iteration < 60; ++iteration) {
        const cppui::ThemeTokens& theme =
            (iteration % 2 == 0) ? cppui::dark_theme() : cppui::light_theme();
        if (!renderer_.SetTheme(theme) || !renderer_.Render(chrome, ui_.root())) {
            OutputDebugStringW(L"CppUI benchmark: theme switch failed.\n");
            return 8;
        }
    }
    const double theme_avg_ms = elapsed_ms(theme_start, Clock::now()) / 60.0;

    const auto density_start = Clock::now();
    const cppui::DensityMode densities[] = {
        cppui::DensityMode::compact,
        cppui::DensityMode::cozy,
        cppui::DensityMode::comfortable,
    };
    for (int iteration = 0; iteration < 30; ++iteration) {
        renderer_.SetDensity(densities[iteration % 3]);
        LayoutUiForSize(1000.0f, 640.0f);
        if (!renderer_.Render(chrome, ui_.root())) {
            OutputDebugStringW(L"CppUI benchmark: density switch failed.\n");
            return 9;
        }
    }
    const double density_avg_ms = elapsed_ms(density_start, Clock::now()) / 30.0;

    if (!renderer_.SetTheme(cppui::light_theme())) {
        OutputDebugStringW(L"CppUI benchmark: restore theme failed.\n");
        return 10;
    }
    renderer_.SetDensity(cppui::DensityMode::cozy);

    std::sort(frame_times.begin(), frame_times.end());
    const double frame_total =
        std::accumulate(frame_times.begin(), frame_times.end(), 0.0);
    const double frame_avg_ms = frame_total / static_cast<double>(frame_times.size());
    const size_t p95_index =
        std::min(frame_times.size() - 1, static_cast<size_t>(frame_times.size() * 0.95));

    const cppui::Size target = renderer_.TargetSizeDip();

    std::ofstream output("cppui-benchmark.txt", std::ios::trunc);
    if (!output) {
        OutputDebugStringW(L"CppUI benchmark: output file failed.\n");
        return 11;
    }

    output << "CppUI benchmark\n";
#ifdef NDEBUG
    output << "configuration=Release\n";
#else
    output << "configuration=Debug\n";
#endif
    output << "frame_avg_ms=" << frame_avg_ms << '\n';
    output << "frame_p95_ms=" << frame_times[p95_index] << '\n';
    output << "frame_cpu_avg_ms=" << cpu_ms_per_frame << '\n';
    output << "frame_fps=" << (1000.0 / frame_avg_ms) << '\n';
    output << "startup_to_first_frame_ms=" << startup_ms << '\n';
    output << "layout_avg_ms=" << layout_avg_ms << '\n';
    output << "resize_avg_ms=" << resize_avg_ms << '\n';
    output << "theme_switch_avg_ms=" << theme_avg_ms << '\n';
    output << "density_switch_avg_ms=" << density_avg_ms << '\n';
    output << "target_dip=" << target.width << 'x' << target.height << '\n';

    renderer_.Shutdown();
    renderer_ready_ = false;
    window_ = nullptr;
    window.Destroy();
    return 0;
}

int BootstrapApp::RunVisualCapture(cppui::Win32Window& window) {
    (void)CreateDirectoryW(L"visual-output", nullptr);

    const auto capture = [this](cppui::ThemeMode theme,
                                cppui::DensityMode density,
                                const wchar_t* path) {
        SetThemeMode(theme);
        SetDensityMode(density);
        if (theme_label_ != nullptr) {
            SetStatusText(theme_label_->text());
        }
        if (!renderer_.Resize(1250, 800, 120.0f)) {
            return false;
        }
        LayoutUiForSize(1000.0f, 640.0f);

        cppui::ChromeState chrome{};
        chrome.active = true;
        chrome.theme_mode = theme;
        return renderer_.RenderAndCapture(chrome, ui_.root(), path);
    };

    if (!capture(cppui::ThemeMode::light,
                 cppui::DensityMode::cozy,
                 L"visual-output/phase5-light-cozy.png")) {
        OutputDebugStringW(L"CppUI visual: light capture failed.\n");
        return 2;
    }
    if (!capture(cppui::ThemeMode::dark,
                 cppui::DensityMode::cozy,
                 L"visual-output/phase5-dark-cozy.png")) {
        OutputDebugStringW(L"CppUI visual: dark capture failed.\n");
        return 3;
    }
    if (!capture(cppui::ThemeMode::light,
                 cppui::DensityMode::compact,
                 L"visual-output/phase5-light-compact.png")) {
        OutputDebugStringW(L"CppUI visual: compact capture failed.\n");
        return 4;
    }

    renderer_.Shutdown();
    renderer_ready_ = false;
    window_ = nullptr;
    window.Destroy();
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
    const bool benchmark = command_line.find(L"--benchmark") != std::wstring_view::npos;
    const bool capture_visual = command_line.find(L"--capture-visual") != std::wstring_view::npos;

    BootstrapApp app;
    const int result = app.Run(instance, show_command, self_test, benchmark, capture_visual);

    CoUninitialize();
    return result;
}
