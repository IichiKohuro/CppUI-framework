#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cppui/geometry.hpp>

namespace cppui {

class Painter;

enum class Orientation {
    vertical,
    horizontal,
};

struct KeyModifiers {
    bool shift{};
    bool control{};
    bool alt{};
};

// Base class for the retained-mode tree. Input callbacks are intentionally
// small and overridable; a control that does not care about an event ignores it.
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
    virtual void on_mouse_down(Point point) {
        (void)point;
    }
    virtual void on_mouse_up(Point point) {
        (void)point;
    }
    virtual void on_mouse_move(Point point) {
        (void)point;
    }
    virtual void on_mouse_leave() {}
    [[nodiscard]] virtual bool on_mouse_wheel(float delta) {
        (void)delta;
        return false;
    }
    virtual bool on_key_down(int key, KeyModifiers modifiers) {
        (void)key;
        (void)modifiers;
        return false;
    }
    virtual bool on_char(wchar_t character) {
        (void)character;
        return false;
    }

    // A control can opt out of click-on-release activation when it owns a
    // drag interaction (scrollbar, text selection).
    [[nodiscard]] virtual bool activate_on_click() const noexcept { return true; }

    // Animation is opt-in. A host can call on_tick and repaint only while at
    // least one visible widget reports true.
    [[nodiscard]] virtual bool animating() const noexcept { return false; }
    // Modal overlays/popups return true so the host restricts focus and input
    // to that subtree.
    [[nodiscard]] virtual bool focus_trap() const noexcept { return false; }
    virtual void on_tick(float seconds) { (void)seconds; }
    virtual void on_focus_changed(bool focused) { (void)focused; }

    // Window-coordinate DIP rectangle used to place IME composition/candidate
    // windows. Controls with a text caret override this; an empty rect means
    // no caret is available.
    [[nodiscard]] virtual Rect ime_caret_rect() const noexcept { return {}; }

    [[nodiscard]] virtual Widget* hit_test(Point point) const;
    void add_child(std::unique_ptr<Widget> child);
    std::unique_ptr<Widget> remove_child(Widget* child);
    [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const noexcept {
        return children_;
    }

    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    [[nodiscard]] Widget* parent() const noexcept { return parent_; }
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }
    [[nodiscard]] bool focused() const noexcept { return focused_; }
    [[nodiscard]] bool visible() const noexcept { return visible_; }

    void set_hovered(bool value) noexcept { hovered_ = value; }
    void set_pressed(bool value) noexcept { pressed_ = value; }
    void set_focused(bool value) noexcept { focused_ = value; }
    void set_visible(bool value) noexcept;
    void set_tooltip(std::wstring text) { tooltip_ = std::move(text); }
    [[nodiscard]] const std::wstring& tooltip() const noexcept { return tooltip_; }

    void set_validation_error(std::wstring text) { validation_error_ = std::move(text); }
    void clear_validation_error() { validation_error_.clear(); }
    [[nodiscard]] bool has_validation_error() const noexcept { return !validation_error_.empty(); }
    [[nodiscard]] const std::wstring& validation_error() const noexcept { return validation_error_; }

    virtual void collect_focusable(std::vector<Widget*>& output);

protected:
    void set_bounds(const Rect& bounds) noexcept { bounds_ = bounds; }
    void set_desired_size(const Size& size) noexcept { desired_ = size; }
    [[nodiscard]] Size desired_size() const noexcept { return desired_; }
    void paint_children(Painter& painter) const;
    virtual void on_child_added(Widget& child) { (void)child; }

private:
    Rect bounds_{};
    Size desired_{};
    Widget* parent_{};
    std::vector<std::unique_ptr<Widget>> children_;
    std::wstring tooltip_;
    std::wstring validation_error_;
    bool hovered_{false};
    bool pressed_{false};
    bool focused_{false};
    bool visible_{true};
};

}  // namespace cppui
