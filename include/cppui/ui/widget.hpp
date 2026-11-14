#pragma once

#include <memory>
#include <vector>

#include <cppui/geometry.hpp>

namespace cppui {

class Painter;

class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(Widget&&) = delete;

    [[nodiscard]] virtual Size measure(Painter& painter, const Size& available) = 0;
    virtual void arrange(const Rect& bounds) = 0;
    virtual void paint(Painter& painter) const = 0;

    [[nodiscard]] virtual bool focusable() const noexcept { return false; }
    [[nodiscard]] virtual bool accepts_mouse() const noexcept { return false; }
    virtual void on_activate() {}
    virtual bool on_key_down(int key, bool shift) {
        (void)key;
        (void)shift;
        return false;
    }

    [[nodiscard]] Widget* hit_test(Point point) const;
    void add_child(std::unique_ptr<Widget> child);
    [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const noexcept {
        return children_;
    }

    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }
    [[nodiscard]] bool focused() const noexcept { return focused_; }

    void set_hovered(bool value) noexcept { hovered_ = value; }
    void set_pressed(bool value) noexcept { pressed_ = value; }
    void set_focused(bool value) noexcept { focused_ = value; }

    void collect_focusable(std::vector<Widget*>& output);

protected:
    void set_bounds(const Rect& bounds) noexcept { bounds_ = bounds; }
    void set_desired_size(const Size& size) noexcept { desired_ = size; }
    [[nodiscard]] Size desired_size() const noexcept { return desired_; }
    void paint_children(Painter& painter) const;

private:
    Rect bounds_{};
    Size desired_{};
    std::vector<std::unique_ptr<Widget>> children_;
    bool hovered_{false};
    bool pressed_{false};
    bool focused_{false};
};

}  // namespace cppui
