#include <cppui/ui/host.hpp>

#include <algorithm>
#include <utility>

#include <cppui/ui/painter.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {
namespace {

constexpr int kKeyTab = 0x09;

}  // namespace

void UiHost::set_root(std::unique_ptr<Widget> root) {
    hovered_ = nullptr;
    pressed_ = nullptr;
    focused_ = nullptr;
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

    if (pressed_ != nullptr && hit != pressed_) {
        set_pressed(nullptr);
    }
}

void UiHost::mouse_leave() {
    set_hovered(nullptr);
    set_pressed(nullptr);
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
        set_pressed(hit);
    }
}

void UiHost::mouse_up(Point point) {
    if (!root_) {
        return;
    }

    Widget* const hit = root_->hit_test(point);
    Widget* const pressed = pressed_;
    set_pressed(nullptr);
    set_hovered(hit);

    if (pressed != nullptr && pressed == hit) {
        pressed->on_activate();
    }
}

bool UiHost::key_down(int key, bool shift) {
    if (focused_ != nullptr && focused_->on_key_down(key, shift)) {
        return true;
    }

    if (key != kKeyTab || focus_order_.empty()) {
        return false;
    }

    auto current = std::find(focus_order_.begin(), focus_order_.end(), focused_);
    std::size_t index = 0;
    if (current == focus_order_.end()) {
        index = shift ? focus_order_.size() - 1 : 0;
    } else {
        const std::size_t current_index = static_cast<std::size_t>(current - focus_order_.begin());
        if (shift) {
            index = (current_index + focus_order_.size() - 1) % focus_order_.size();
        } else {
            index = (current_index + 1) % focus_order_.size();
        }
    }

    set_focused(focus_order_[index]);
    return true;
}

void UiHost::clear_focus() {
    set_focused(nullptr);
}

void UiHost::set_hovered(Widget* widget) {
    if (hovered_ == widget) {
        return;
    }
    if (hovered_ != nullptr) {
        hovered_->set_hovered(false);
    }
    hovered_ = widget;
    if (hovered_ != nullptr) {
        hovered_->set_hovered(true);
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
    }
    focused_ = widget;
    if (focused_ != nullptr) {
        focused_->set_focused(true);
    }
}

void UiHost::rebuild_focus_order() {
    focus_order_.clear();
    if (root_) {
        root_->collect_focusable(focus_order_);
    }
}

}  // namespace cppui
