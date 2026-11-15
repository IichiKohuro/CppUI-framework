#pragma once

#include <memory>
#include <vector>

#include <cppui/geometry.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {

class Painter;

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
    [[nodiscard]] bool mouse_wheel(Point point, float delta);
    bool key_down(int key, bool shift);
    bool key_down(int key, KeyModifiers modifiers);
    bool char_input(wchar_t character);
    void tick(float seconds);

    void set_focus(Widget* widget);
    void clear_focus();
    [[nodiscard]] Widget* focused() const noexcept { return focused_; }
    [[nodiscard]] Widget* hovered() const noexcept { return hovered_; }
    [[nodiscard]] const std::wstring& hovered_tooltip() const noexcept { return tooltip_; }
    [[nodiscard]] bool has_animation() const;

private:
    void set_hovered(Widget* widget);
    void set_pressed(Widget* widget);
    void set_focused(Widget* widget);
    void rebuild_focus_order();

    std::unique_ptr<Widget> root_;
    Widget* hovered_{};
    Widget* pressed_{};
    Widget* captured_{};
    Widget* focused_{};
    std::vector<Widget*> focus_order_;
    std::wstring tooltip_;
};

}  // namespace cppui
