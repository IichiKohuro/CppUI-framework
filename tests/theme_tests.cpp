#include <cppui/theme/tokens.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool Expect(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
    }
    return condition;
}

double ChannelToLinear(std::uint8_t channel) {
    const double value = static_cast<double>(channel) / 255.0;
    if (value <= 0.04045) {
        return value / 12.92;
    }
    return std::pow((value + 0.055) / 1.055, 2.4);
}

double Luminance(const cppui::Color& color) {
    return 0.2126 * ChannelToLinear(color.r) + 0.7152 * ChannelToLinear(color.g) +
           0.0722 * ChannelToLinear(color.b);
}

double Contrast(const cppui::Color& first, const cppui::Color& second) {
    const double first_luminance = Luminance(first);
    const double second_luminance = Luminance(second);
    const double lighter = std::max(first_luminance, second_luminance);
    const double darker = std::min(first_luminance, second_luminance);
    return (lighter + 0.05) / (darker + 0.05);
}

}  // namespace

int main() {
    using cppui::Color;

    const cppui::ThemeTokens& light = cppui::light_theme();
    const cppui::ThemeTokens& dark = cppui::dark_theme();

    bool passed = true;
    passed &= Expect(light.accent == Color{0x00, 0x70, 0x5F}, "light accent is Signal Teal");
    passed &= Expect(dark.accent == Color{0x3B, 0xB3, 0x9E}, "dark accent is Signal Teal");
    passed &= Expect(light.surface.a == 255, "light surface is opaque");
    passed &= Expect(dark.surface.a == 255, "dark surface is opaque");
    passed &= Expect(light.text_primary != light.window_bg, "light text contrasts with background");
    passed &= Expect(dark.text_primary != dark.window_bg, "dark text contrasts with background");
    passed &= Expect(light.surface_hover != light.surface, "light hover surface differs from surface");
    passed &= Expect(light.close_hover == Color{0xC4, 0x2B, 0x1C}, "close hover is the caption red");

    passed &= Expect(Contrast(light.primary, light.on_accent) >= 4.5,
                     "light primary has AA contrast with white text");
    passed &= Expect(Contrast(dark.primary, Color{0xFF, 0xFF, 0xFF}) >= 4.5,
                     "dark primary has AA contrast with white text");
    passed &= Expect(Contrast(light.text_primary, light.window_bg) >= 7.0,
                     "light body text has AAA contrast");
    passed &= Expect(Contrast(dark.text_primary, dark.window_bg) >= 7.0,
                     "dark body text has AAA contrast");

    if (passed) {
        std::cout << "CppUI theme token tests passed.\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
