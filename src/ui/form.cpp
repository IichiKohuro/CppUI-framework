#include <cppui/ui/form.hpp>

#include <algorithm>
#include <utility>

namespace cppui {

Size StatusBar::measure(Painter& painter, const Size& available) {
    const DensityMetrics metrics = density_metrics(painter.density());
    const Size desired{available.width, std::max(24.0f, metrics.control_height - 6.0f)};
    set_desired_size(desired);
    return desired;
}

void StatusBar::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void StatusBar::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const Rect area = bounds();

    painter.fill_rect(area, tokens.surface_soft);
    painter.fill_rect(Rect{area.x, area.y, area.width, 1.0f}, tokens.border);

    const float padding = density_metrics(painter.density()).control_padding;
    const Rect text_area{
        area.x + padding,
        area.y + 1.0f,
        std::max(0.0f, area.width - padding * 2.0f),
        std::max(0.0f, area.height - 1.0f),
    };

    painter.draw_text(left_text_,
                      text_area,
                      FontRole::caption,
                      tokens.text_tertiary,
                      TextAlign::leading,
                      TextVAlign::center);
    painter.draw_text(right_text_,
                      text_area,
                      FontRole::caption,
                      tokens.text_tertiary,
                      TextAlign::trailing,
                      TextVAlign::center);
}

FormPanel::FormPanel(std::unique_ptr<Widget> content, std::unique_ptr<Widget> status_bar) {
    add_child(std::move(content));
    add_child(std::move(status_bar));
}

Widget* FormPanel::content() const noexcept {
    return children().size() > 0 ? children()[0].get() : nullptr;
}

Widget* FormPanel::status_bar() const noexcept {
    return children().size() > 1 ? children()[1].get() : nullptr;
}

Size FormPanel::measure(Painter& painter, const Size& available) {
    Widget* const content_widget = content();
    Widget* const status_widget = status_bar();
    if (content_widget == nullptr || status_widget == nullptr) {
        return available;
    }

    status_size_ = status_widget->measure(painter, available);

    const float content_height = std::max(0.0f, available.height - status_size_.height);
    content_size_ = content_widget->measure(painter, Size{available.width, content_height});

    const Size desired{available.width, content_size_.height + status_size_.height};
    set_desired_size(desired);
    return desired;
}

void FormPanel::arrange(const Rect& bounds) {
    Widget* const content_widget = content();
    Widget* const status_widget = status_bar();
    if (content_widget == nullptr || status_widget == nullptr) {
        set_bounds(bounds);
        return;
    }

    const float status_height = status_size_.height;
    const Rect status_rect{
        bounds.x,
        bounds.bottom() - status_height,
        bounds.width,
        status_height,
    };
    const Rect content_rect{
        bounds.x,
        bounds.y,
        bounds.width,
        std::max(0.0f, bounds.height - status_height),
    };

    content_widget->arrange(content_rect);
    status_widget->arrange(status_rect);
    set_bounds(bounds);
}

void FormPanel::paint(Painter& painter) const {
    paint_children(painter);
}

}  // namespace cppui
