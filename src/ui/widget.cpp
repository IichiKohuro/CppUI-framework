#include <cppui/ui/widget.hpp>

#include <cppui/ui/painter.hpp>

namespace cppui {

Widget* Widget::hit_test(Point point) const {
    if (!bounds_.contains(point)) {
        return nullptr;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (Widget* hit = it->get()->hit_test(point)) {
            return hit;
        }
    }

    return accepts_mouse() ? const_cast<Widget*>(this) : nullptr;
}

void Widget::add_child(std::unique_ptr<Widget> child) {
    children_.push_back(std::move(child));
}

void Widget::collect_focusable(std::vector<Widget*>& output) {
    if (focusable()) {
        output.push_back(this);
    }
    for (const auto& child : children_) {
        child->collect_focusable(output);
    }
}

void Widget::paint_children(Painter& painter) const {
    for (const auto& child : children_) {
        child->paint(painter);
    }
}

}  // namespace cppui
