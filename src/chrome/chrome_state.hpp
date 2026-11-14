#pragma once

namespace cppui {

enum class ChromeButton {
    none,
    minimize,
    maximize,
    close,
};

struct ChromeState {
    bool active{false};
    bool maximized{false};
    ChromeButton hovered{ChromeButton::none};
    ChromeButton pressed{ChromeButton::none};
};

inline constexpr float kTitleBarHeightDip = 40.0f;
inline constexpr float kCaptionButtonWidthDip = 46.0f;

}  // namespace cppui
