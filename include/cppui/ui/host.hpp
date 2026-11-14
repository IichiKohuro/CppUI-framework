#pragma once

#include <memory>
#include <vector>

#include <cppui/geometry.hpp>

namespace cppui {

class Painter;
class Widget;

class UiHost {
public:
    void set_root(std::unique_ptr<Widget> root);
    [[nodiscard]] Widget* root() const noexcept { return root_.get(); }

    void layout(Painter& painter, const Rect& bounds);
    void paint(Painter& painter) const;

    void mouse_move(Point point);
    void mouse_leave();
    void mouse_down(Point point);
    void mouse_up(Point point);
    bool key_down(int key, bool shift);

    void clear_focus();
    [[nodiscard]] Widget* focused() const noexcept { return focused_; }

private:
    void set_hovered(Widget* widget);
    void set_pressed(Widget* widget);
    void set_focused(Widget* widget);
    void rebuild_focus_order();

    std::unique_ptr<Widget> root_;
    Widget* hovered_{};
    Widget* pressed_{};
    Widget* focused_{};
    std::vector<Widget*> focus_order_;
};

}  // namespace cppui
