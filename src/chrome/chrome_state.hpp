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
};

struct ChromeState {
    bool active{false};
    bool maximized{false};
    ChromeButton hovered{ChromeButton::none};
    ChromeButton pressed{ChromeButton::none};
    ThemeMode theme_mode{ThemeMode::light};
    ChromeButton theme_hovered{ChromeButton::none};
    ChromeButton theme_pressed{ChromeButton::none};
};

inline constexpr float kTitleBarHeightDip = 40.0f;
inline constexpr float kCaptionButtonWidthDip = 46.0f;
inline constexpr float kThemeOptionWidthDip = 36.0f;
inline constexpr float kThemeSwitcherHeightDip = 26.0f;
inline constexpr float kThemeSwitcherGapDip = 8.0f;
inline constexpr float kTitleButtonSeparatorDip = 10.0f;

}  // namespace cppui
