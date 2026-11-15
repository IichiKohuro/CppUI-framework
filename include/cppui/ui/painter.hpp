#pragma once

#include <string_view>

#include <cppui/geometry.hpp>
#include <cppui/theme/tokens.hpp>
#include <cppui/ui/density.hpp>

namespace cppui {

class Image;

enum class FontRole {
    caption,
    body,
    label,
    heading,
    title,
};

enum class TextAlign {
    leading,
    center,
    trailing,
};

enum class TextVAlign {
    top,
    center,
    bottom,
};

// Painting contract for retained-mode widgets. The concrete implementation is
// intentionally platform-agnostic from the widget point of view.
class Painter {
public:
    virtual ~Painter() = default;

    [[nodiscard]] virtual const ThemeTokens& theme() const noexcept = 0;
    [[nodiscard]] virtual DensityMode density() const noexcept = 0;
    [[nodiscard]] virtual Size measure_text(std::wstring_view text, FontRole role) const = 0;

    virtual void fill_rect(const Rect& rect, const Color& color) = 0;
    virtual void fill_rounded_rect(const Rect& rect, float radius, const Color& color) = 0;
    virtual void stroke_rounded_rect(const Rect& rect,
                                     float radius,
                                     const Color& color,
                                     float width = 1.0f) = 0;
    virtual void draw_line(Point from,
                           Point to,
                           const Color& color,
                           float width = 1.0f) = 0;
    virtual void draw_image(const Rect& rect, const Image& image, float opacity = 1.0f) = 0;
    virtual void draw_text(std::wstring_view text,
                           const Rect& rect,
                           FontRole role,
                           const Color& color,
                           TextAlign align = TextAlign::leading,
                           TextVAlign vertical = TextVAlign::center) = 0;

    // Clip stack used by scrollable containers. The default is a no-op so a
    // lightweight test painter can ignore clipping.
    virtual void push_clip(const Rect& rect) { (void)rect; }
    virtual void pop_clip() {}
};

}  // namespace cppui
