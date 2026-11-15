#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <cppui/theme/mode.hpp>
#include <cppui/ui/density.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {

class StatusBar;
class PopupHost;

// Per-monitor DPI awareness is the host process's responsibility. Call
//
//   SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//
// before creating any window or Form. Custom chrome hit testing and DIP layout
// assume per-monitor V2; a system-DPI or DPI-unaware process gets virtualized
// geometry instead of a hard failure. Form logs a diagnostic when it detects
// this. The check reads the calling thread's awareness context.
[[nodiscard]] bool IsPerMonitorDpiAware() noexcept;

// Top-level form host: owns the Win32 window, renderer, UI host and status bar.
// Platform and render types stay behind a pimpl so the public header has no
// windows.h/COM leakage.
class Form final {
public:
    Form(std::wstring title,
         int width_dip,
         int height_dip,
         int min_width_dip = 900,
         int min_height_dip = 580);
    ~Form();

    Form(const Form&) = delete;
    Form& operator=(const Form&) = delete;
    Form(Form&&) = delete;
    Form& operator=(Form&&) = delete;

    [[nodiscard]] bool Create();
    void SetContent(std::unique_ptr<Widget> content);

    [[nodiscard]] StatusBar* status_bar() noexcept;
    [[nodiscard]] const StatusBar* status_bar() const noexcept;
    [[nodiscard]] PopupHost* popups() noexcept;
    [[nodiscard]] const PopupHost* popups() const noexcept;
    void SetStatusText(std::wstring text);

    void SetThemeMode(ThemeMode mode);
    [[nodiscard]] ThemeMode theme_mode() const noexcept;
    void SetDensityMode(DensityMode mode);
    [[nodiscard]] DensityMode density_mode() const noexcept;

    void SetOnThemeChanged(std::function<void(ThemeMode)> callback);
    void SetOnDensityChanged(std::function<void(DensityMode)> callback);
    void SetOnChromeAction(std::function<void()> callback);

    void Show();
    int Run();
    void Invalidate();
    void Close();
    // Captures the form client into a PNG at the current client size.
    [[nodiscard]] bool CapturePng(std::wstring_view path);
    // Captures at an explicit DIP size, independent of the desktop resolution.
    // The window may be smaller than the requested size (for example on a CI
    // runner with a 1024x768 desktop); the offscreen target is laid out at the
    // requested size and the client layout is restored afterwards.
    [[nodiscard]] bool CapturePng(std::wstring_view path, int width_dip, int height_dip);

    [[nodiscard]] bool IsCreated() const noexcept;
    [[nodiscard]] void* NativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppui
