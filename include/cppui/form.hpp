#pragma once

#include <functional>
#include <memory>
#include <string>

#include <cppui/theme/mode.hpp>
#include <cppui/ui/density.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {

class StatusBar;

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

    [[nodiscard]] bool IsCreated() const noexcept;
    [[nodiscard]] void* NativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppui
