#pragma once

#include <windows.h>

#include <cstdint>
#include <functional>
#include <string>

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
    [[nodiscard]] bool IsMinimized() const noexcept;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void CenterOnPrimaryMonitor() noexcept;
    void NotifyResize(HWND hwnd, UINT width, UINT height) noexcept;

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
};

}  // namespace cppui
