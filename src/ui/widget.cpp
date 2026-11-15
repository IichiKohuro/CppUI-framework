#include <cppui/ui/widget.hpp>

#include <algorithm>
#include <utility>

#include <cppui/ui/painter.hpp>

namespace cppui {

Widget* Widget::hit_test(Point point) const {
    if (!visible_ || !bounds_.contains(point)) {
        return nullptr;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (Widget* hit = it->get()->hit_test(point)) {
            return hit;
        }
    }

    return accepts_mouse() ? const_cast<Widget*>(this) : nullptr;
}

void Widget::set_visible(bool value) noexcept {
    visible_ = value;
    if (!visible_) {
        hovered_ = false;
        pressed_ = false;
    }
}

void Widget::add_child(std::unique_ptr<Widget> child) {
    if (!child) {
        return;
    }
    child->parent_ = this;
    Widget& reference = *child;
    children_.push_back(std::move(child));
    on_child_added(reference);
}

std::unique_ptr<Widget> Widget::remove_child(Widget* child) {
    if (child == nullptr) {
        return nullptr;
    }
    const auto position = std::find_if(children_.begin(),
                                       children_.end(),
                                       [child](const std::unique_ptr<Widget>& candidate) {
                                           return candidate.get() == child;
                                       });
    if (position == children_.end()) {
        return nullptr;
    }
    std::unique_ptr<Widget> removed = std::move(*position);
    removed->parent_ = nullptr;
    children_.erase(position);
    return removed;
}

void Widget::collect_focusable(std::vector<Widget*>& output) {
    if (!visible_) {
        return;
    }
    if (focusable()) {
        output.push_back(this);
    }
    for (const auto& child : children_) {
        child->collect_focusable(output);
    }
}

void Widget::paint_children(Painter& painter) const {
    for (const auto& child : children_) {
        if (child->visible()) {
            child->paint(painter);
        }
    }
}

}  // namespace cppui
