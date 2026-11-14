#pragma once

#include <windows.h>

#include <cstdint>
#include <functional>
#include <string>

#include "chrome/chrome_state.hpp"

namespace cppui {

// Minimal top-level Win32 window for Phase 1. It owns the message loop and
// forwards paint/resize/DPI events; custom chrome and widget hosting arrive
// in later phases.
class Win32Window final {
public:
    struct Callbacks {
        std::function<void()> paint;
        std::function<void(std::uint32_t width, std::uint32_t height, float dpi)> resized;
        std::function<void(float dpi)> dpi_changed;
        std::function<bool(WPARAM key)> key_down;
        std::function<void(int x, int y)> mouse_move;
        std::function<void()> mouse_leave;
        std::function<void(int x, int y)> mouse_down;
        std::function<void(int x, int y)> mouse_up;
        std::function<void()> system_theme_changed;
        std::function<void(ThemeMode mode)> theme_selected;
    };

    Win32Window(HINSTANCE instance,
                std::wstring title,
                int width_dip,
                int height_dip,
                int min_width_dip = 720,
                int min_height_dip = 480);
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;
    Win32Window(Win32Window&&) = delete;
    Win32Window& operator=(Win32Window&&) = delete;

    [[nodiscard]] bool Create(Callbacks callbacks);
    void Show(int show_command) noexcept;
    [[nodiscard]] int Run();
    void Destroy() noexcept;
    void Invalidate() noexcept;

    [[nodiscard]] HWND Handle() const noexcept { return hwnd_; }
    [[nodiscard]] float Dpi() const noexcept { return dpi_; }
    [[nodiscard]] bool IsWindowMinimized() const noexcept;
    [[nodiscard]] ChromeState GetChromeState() const noexcept;
    void SetThemeMode(ThemeMode mode) noexcept;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void CenterOnPrimaryMonitor() noexcept;
    void NotifyResize(HWND hwnd, UINT width, UINT height) noexcept;
    [[nodiscard]] LRESULT HitTest(POINT screen_point) const noexcept;
    [[nodiscard]] ChromeButton ButtonAt(POINT screen_point) const noexcept;
    [[nodiscard]] ChromeButton ThemeButtonAt(POINT screen_point) const noexcept;
    static ChromeButton ButtonFromHitCode(LRESULT hit_code) noexcept;
    static ThemeMode ThemeModeForButton(ChromeButton button) noexcept;
    void CompleteChromePress(POINT screen_point) noexcept;
    void CompleteThemePress(POINT screen_point) noexcept;
    void PerformChromeAction(ChromeButton button) noexcept;
    void SetActive(bool active) noexcept;
    void SetHovered(ChromeButton button) noexcept;
    void ApplyDwmChrome() noexcept;

    HINSTANCE instance_{};
    HWND hwnd_{};
    std::wstring title_;
    int width_dip_{};
    int height_dip_{};
    int min_width_dip_{};
    int min_height_dip_{};
    float dpi_{96.0f};
    Callbacks callbacks_{};
    bool class_registered_{false};
    bool active_{false};
    ChromeButton hovered_{ChromeButton::none};
    ChromeButton pressed_{ChromeButton::none};
    ThemeMode theme_mode_{ThemeMode::light};
    ChromeButton theme_hovered_{ChromeButton::none};
    ChromeButton theme_pressed_{ChromeButton::none};
};

}  // namespace cppui
