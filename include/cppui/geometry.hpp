#pragma once

namespace cppui {

struct Point {
    float x{};
    float y{};
};

struct Size {
    float width{};
    float height{};
};

struct Insets {
    float left{};
    float top{};
    float right{};
    float bottom{};
};

struct Rect {
    float x{};
    float y{};
    float width{};
    float height{};

    [[nodiscard]] float right() const noexcept { return x + width; }
    [[nodiscard]] float bottom() const noexcept { return y + height; }

    [[nodiscard]] bool contains(Point point) const noexcept {
        return point.x >= x && point.x < right() && point.y >= y && point.y < bottom();
    }

    [[nodiscard]] Rect inset(const Insets& insets) const noexcept {
        return Rect{
            x + insets.left,
            y + insets.top,
            width - insets.left - insets.right,
            height - insets.top - insets.bottom,
        };
    }
};

}  // namespace cppui
