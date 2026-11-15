#pragma once

#include <cppui/theme/mode.hpp>

namespace cppui {

enum class ChromeButton {
    none,
    minimize,
    maximize,
    close,
    theme_light,
    theme_dark,
    theme_system,
    action_reset,
};

struct ChromeState {
    const wchar_t* title{nullptr};
    bool active{false};
    bool maximized{false};
    ChromeButton hovered{ChromeButton::none};
    ChromeButton pressed{ChromeButton::none};
    ThemeMode theme_mode{ThemeMode::light};
    ChromeButton theme_hovered{ChromeButton::none};
    ChromeButton theme_pressed{ChromeButton::none};
    ChromeButton action_hovered{ChromeButton::none};
    ChromeButton action_pressed{ChromeButton::none};
    bool show_theme_switcher{true};
    bool show_chrome_action{true};
    bool show_badge{true};
};

inline constexpr float kTitleBarHeightDip = 40.0f;
inline constexpr float kCaptionButtonWidthDip = 46.0f;
inline constexpr float kThemeOptionWidthDip = 36.0f;
inline constexpr float kThemeSwitcherHeightDip = 26.0f;
inline constexpr float kThemeSwitcherGapDip = 8.0f;
inline constexpr float kTitleButtonSeparatorDip = 10.0f;
inline constexpr float kChromeActionWidthDip = 30.0f;

}  // namespace cppui
