#pragma once

#include <cppui/theme/tokens.hpp>

namespace cppui {

enum class ThemeMode {
    light,
    dark,
    system,
};

[[nodiscard]] bool system_prefers_dark() noexcept;
[[nodiscard]] const ThemeTokens& resolve_theme(ThemeMode mode) noexcept;

}  // namespace cppui
