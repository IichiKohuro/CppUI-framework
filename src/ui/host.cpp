#include <cppui/ui/host.hpp>

#include <algorithm>
#include <utility>

#include <cppui/ui/painter.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {
namespace {

constexpr int kKeyTab = 0x09;

Widget* FindFocusTrap(Widget& widget) {
    if (!widget.visible()) {
        return nullptr;
    }
    if (widget.focus_trap()) {
        return &widget;
    }
    for (const auto& child : widget.children()) {
        if (child != nullptr) {
            if (Widget* trap = FindFocusTrap(*child)) {
                return trap;
            }
        }
    }
    return nullptr;
}

bool IsInside(const Widget* widget, const Widget* ancestor) {
    while (widget != nullptr) {
        if (widget == ancestor) {
            return true;
        }
        widget = widget->parent();
    }
    return false;
}

bool HasAnimation(const Widget& widget) {
    if (!widget.visible()) {
        return false;
    }
    if (widget.animating()) {
        return true;
    }
    for (const auto& child : widget.children()) {
        if (child && HasAnimation(*child)) {
            return true;
        }
    }
    return false;
}

}  // namespace

void UiHost::set_root(std::unique_ptr<Widget> root) {
    hovered_ = nullptr;
    pressed_ = nullptr;
    captured_ = nullptr;
    focused_ = nullptr;
    tooltip_.clear();
    focus_order_.clear();
    root_ = std::move(root);
}

void UiHost::layout(Painter& painter, const Rect& bounds) {
    if (!root_) {
        return;
    }

    const Size measured = root_->measure(painter, Size{bounds.width, bounds.height});
    (void)measured;
    root_->arrange(bounds);
    rebuild_focus_order();

    if (focused_ != nullptr && std::find(focus_order_.begin(), focus_order_.end(), focused_) ==
                                  focus_order_.end()) {
        focused_ = nullptr;
    }
}

void UiHost::paint(Painter& painter) const {
    if (root_) {
        root_->paint(painter);
    }
}

void UiHost::mouse_move(Point point) {
    if (!root_) {
        return;
    }

    Widget* const hit = root_->hit_test(point);
    if (hit != hovered_) {
        set_hovered(hit);
    }

    if (captured_ != nullptr) {
        captured_->on_mouse_move(point);
    } else if (pressed_ != nullptr && hit != pressed_) {
        set_pressed(nullptr);
    }
}

void UiHost::mouse_leave() {
    if (captured_ != nullptr) {
        captured_->on_mouse_leave();
    }
    set_hovered(nullptr);
    set_pressed(nullptr);
    captured_ = nullptr;
}

void UiHost::mouse_down(Point point) {
    if (!root_) {
        return;
    }

    Widget* const hit = root_->hit_test(point);
    set_hovered(hit);

    if (hit != nullptr && hit->focusable()) {
        set_focused(hit);
    } else if (hit == nullptr) {
        set_focused(nullptr);
    }

    if (hit != nullptr) {
        captured_ = hit;
        set_pressed(hit);
        hit->on_mouse_down(point);
    } else {
        captured_ = nullptr;
    }
}

void UiHost::mouse_up(Point point) {
    if (!root_) {
        return;
    }

    Widget* const hit = root_->hit_test(point);
    Widget* const pressed = pressed_;
    Widget* const captured = captured_;
    captured_ = nullptr;
    set_pressed(nullptr);
    set_hovered(hit);

    if (captured != nullptr) {
        captured->on_mouse_up(point);
    }

    if (pressed != nullptr && pressed == hit && pressed->activate_on_click()) {
        pressed->on_activate();
    }
}

bool UiHost::mouse_wheel(Point point, float delta) {
    if (!root_) {
        return false;
    }

    Widget* target = root_->hit_test(point);
    while (target != nullptr) {
        if (target->on_mouse_wheel(delta)) {
            return true;
        }
        target = target->parent();
    }
    return false;
}

bool UiHost::key_down(int key, bool shift) {
    KeyModifiers modifiers{};
    modifiers.shift = shift;
    return key_down(key, modifiers);
}

bool UiHost::key_down(int key, KeyModifiers modifiers) {
    if (root_) {
        if (Widget* trap = FindFocusTrap(*root_); trap != nullptr && !IsInside(focused_, trap)) {
            set_focused(nullptr);
            return true;
        }
    }

    if (focused_ != nullptr && focused_->on_key_down(key, modifiers)) {
        return true;
    }

    if (key != kKeyTab || focus_order_.empty()) {
        return false;
    }

    auto current = std::find(focus_order_.begin(), focus_order_.end(), focused_);
    std::size_t index = 0;
    if (current == focus_order_.end()) {
        index = modifiers.shift ? focus_order_.size() - 1 : 0;
    } else {
        const std::size_t current_index = static_cast<std::size_t>(current - focus_order_.begin());
        if (modifiers.shift) {
            index = (current_index + focus_order_.size() - 1) % focus_order_.size();
        } else {
            index = (current_index + 1) % focus_order_.size();
        }
    }

    set_focused(focus_order_[index]);
    return true;
}

bool UiHost::char_input(wchar_t character) {
    if (root_) {
        if (Widget* trap = FindFocusTrap(*root_); trap != nullptr && !IsInside(focused_, trap)) {
            set_focused(nullptr);
            return true;
        }
    }
    return focused_ != nullptr && focused_->on_char(character);
}

void UiHost::tick(float seconds) {
    if (!root_) {
        return;
    }

    struct TickWalker {
        float seconds;
        static void visit(Widget& widget, float dt) {
            if (!widget.visible()) {
                return;
            }
            widget.on_tick(dt);
            for (const auto& child : widget.children()) {
                if (child) {
                    visit(*child, dt);
                }
            }
        }
    };
    TickWalker::visit(*root_, seconds);
}

void UiHost::set_focus(Widget* widget) {
    if (widget != nullptr && root_) {
        const bool belongs = widget == root_.get() ||
                             std::find(focus_order_.begin(), focus_order_.end(), widget) !=
                                 focus_order_.end();
        if (!belongs) {
            rebuild_focus_order();
        }
    }
    set_focused(widget);
}

void UiHost::clear_focus() {
    set_focused(nullptr);
}

bool UiHost::has_animation() const {
    return root_ != nullptr && HasAnimation(*root_);
}

void UiHost::set_hovered(Widget* widget) {
    if (hovered_ == widget) {
        return;
    }
    if (hovered_ != nullptr) {
        hovered_->set_hovered(false);
    }
    hovered_ = widget;
    tooltip_.clear();
    if (hovered_ != nullptr) {
        hovered_->set_hovered(true);
        tooltip_ = hovered_->tooltip();
    }
}

void UiHost::set_pressed(Widget* widget) {
    if (pressed_ == widget) {
        return;
    }
    if (pressed_ != nullptr) {
        pressed_->set_pressed(false);
    }
    pressed_ = widget;
    if (pressed_ != nullptr) {
        pressed_->set_pressed(true);
    }
}

void UiHost::set_focused(Widget* widget) {
    if (focused_ == widget) {
        return;
    }
    if (focused_ != nullptr) {
        focused_->set_focused(false);
        focused_->on_focus_changed(false);
    }
    focused_ = widget;
    if (focused_ != nullptr) {
        focused_->set_focused(true);
        focused_->on_focus_changed(true);
    }
}

void UiHost::rebuild_focus_order() {
    focus_order_.clear();
    if (!root_) {
        return;
    }
    if (Widget* trap = FindFocusTrap(*root_)) {
        trap->collect_focusable(focus_order_);
        return;
    }
    root_->collect_focusable(focus_order_);
}

}  // namespace cppui
