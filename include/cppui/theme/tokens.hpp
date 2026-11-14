#pragma once

#include <cstdint>

namespace cppui {

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{255};

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

// Semantic tokens only. Controls must not depend on raw palette names.
struct ThemeTokens {
    Color window_bg;
    Color surface;
    Color surface_soft;
    Color surface_hover;
    Color border;
    Color border_strong;
    Color text_primary;
    Color text_secondary;
    Color text_tertiary;
    Color accent;
    Color accent_soft;
    Color primary;
    Color primary_top;
    Color on_accent;
    Color success;
    Color warning;
    Color danger;
    Color close_hover;
    Color on_close;
};

[[nodiscard]] const ThemeTokens& light_theme() noexcept;
[[nodiscard]] const ThemeTokens& dark_theme() noexcept;

}  // namespace cppui
