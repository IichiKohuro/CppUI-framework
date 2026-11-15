#include <cppui/ui/containers.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

#include <cppui/ui/painter.hpp>

namespace cppui {
namespace {

constexpr float kScrollBarThickness = 12.0f;
constexpr float kMinThumbLength = 24.0f;

float ContentHeightLimit() noexcept {
    return 100000.0f;
}

float Sum(const std::vector<float>& values) {
    float total = 0.0f;
    for (float value : values) {
        total += value;
    }
    return total;
}

}  // namespace

Panel::Panel(PanelStyle style, float corner_radius)
    : style_(style), corner_radius_(corner_radius) {}

Size Panel::measure(Painter& painter, const Size& available) {
    const float inner_width = std::max(0.0f, available.width - padding_.left - padding_.right);
    const float inner_height = std::max(0.0f, available.height - padding_.top - padding_.bottom);
    float content_width = 0.0f;
    float content_height = 0.0f;
    for (const auto& child : children()) {
        if (!child->visible()) {
            continue;
        }
        const Size size = child->measure(painter, Size{inner_width, inner_height});
        content_width = std::max(content_width, size.width);
        content_height = std::max(content_height, size.height);
    }

    if (preferred_height_ > 0.0f) {
        content_height = preferred_height_;
    }

    const Size desired{content_width + padding_.left + padding_.right,
                       content_height + padding_.top + padding_.bottom};
    set_desired_size(desired);
    return desired;
}

void Panel::arrange(const Rect& bounds) {
    set_bounds(bounds);
    content_rect_ = bounds.inset(padding_);
    content_rect_.width = std::max(0.0f, content_rect_.width);
    content_rect_.height = std::max(0.0f, content_rect_.height);
    for (const auto& child : children()) {
        if (child->visible()) {
            child->arrange(content_rect_);
        }
    }
}

void Panel::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    switch (style_) {
        case PanelStyle::none:
            break;
        case PanelStyle::surface:
            painter.fill_rounded_rect(bounds(), corner_radius_, tokens.surface);
            if (border_) {
                painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border, 1.0f);
            }
            break;
        case PanelStyle::soft:
            painter.fill_rounded_rect(bounds(), corner_radius_, tokens.surface_soft);
            if (border_) {
                painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border, 1.0f);
            }
            break;
        case PanelStyle::card:
            painter.fill_rounded_rect(bounds(), corner_radius_, tokens.surface);
            if (border_) {
                painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border_strong, 1.0f);
            }
            break;
    }

    if (clip_children_) {
        const Rect clip{content_rect_.x - 2.0f,
                        content_rect_.y - 2.0f,
                        content_rect_.width + 4.0f,
                        content_rect_.height + 4.0f};
        painter.push_clip(clip);
        paint_children(painter);
        painter.pop_clip();
    } else {
        paint_children(painter);
    }
}

GroupPanel::GroupPanel(std::wstring title, PanelStyle style, float corner_radius)
    : Panel(style, corner_radius), title_(std::move(title)) {
    set_padding(Insets{16.0f, 14.0f, 16.0f, 16.0f});
}

Size GroupPanel::measure(Painter& painter, const Size& available) {
    const Insets padding = padding_value();
    const Size title_size = painter.measure_text(title_, FontRole::label);
    header_height_ = std::max(26.0f, title_size.height + 12.0f);

    const float inner_width = std::max(0.0f, available.width - padding.left - padding.right);
    const float inner_height =
        std::max(0.0f, available.height - padding.top - padding.bottom - header_height_);
    float content_width = 0.0f;
    float content_height = 0.0f;
    for (const auto& child : children()) {
        if (!child->visible()) {
            continue;
        }
        const Size size = child->measure(painter, Size{inner_width, inner_height});
        content_width = std::max(content_width, size.width);
        content_height = std::max(content_height, size.height);
    }

    const Size desired{content_width + padding.left + padding.right,
                       header_height_ + content_height + padding.top + padding.bottom};
    set_desired_size(desired);
    return desired;
}

void GroupPanel::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Insets padding = padding_value();
    const Rect inner = bounds.inset(padding);
    const Rect content{inner.x,
                       inner.y + header_height_,
                       std::max(0.0f, inner.width),
                       std::max(0.0f, inner.height - header_height_)};
    for (const auto& child : children()) {
        if (child->visible()) {
            child->arrange(content);
        }
    }
}

void GroupPanel::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const Insets padding = padding_value();
    painter.fill_rounded_rect(bounds(), corner_radius_value(), tokens.surface);
    if (border_value()) {
        painter.stroke_rounded_rect(bounds(), corner_radius_value(), tokens.border, 1.0f);
    }

    const Rect header{bounds().x + padding.left,
                      bounds().y + padding.top,
                      std::max(0.0f, bounds().width - padding.left - padding.right),
                      header_height_};
    painter.draw_text(title_,
                      header,
                      FontRole::label,
                      tokens.text_secondary,
                      TextAlign::leading,
                      TextVAlign::center);
    painter.fill_rect(Rect{header.x, header.bottom() - 1.0f, header.width, 1.0f}, tokens.border);

    const Rect content{header.x,
                       header.bottom(),
                       header.width,
                       std::max(0.0f, bounds().bottom() - padding.bottom - header.bottom())};
    // Control strokes are centered on their bounds. A clip exactly at the
    // content edge would shave half of the left/top border and make every
    // first column control look cut off.
    const Rect clip{content.x - 2.0f,
                    content.y - 2.0f,
                    content.width + 4.0f,
                    content.height + 4.0f};
    painter.push_clip(clip);
    paint_children(painter);
    painter.pop_clip();
}

void GridPanel::set_columns(std::vector<float> weights) {
    if (weights.empty()) {
        weights.push_back(1.0f);
    }
    for (float& weight : weights) {
        weight = std::max(0.0f, weight);
    }
    column_weights_ = std::move(weights);
}

void GridPanel::set_rows(std::vector<float> weights) {
    for (float& weight : weights) {
        weight = std::max(0.0f, weight);
    }
    row_weights_ = std::move(weights);
    if (row_weights_.empty()) {
        row_weights_.push_back(1.0f);
    }
}

void GridPanel::set_gap(float horizontal, float vertical) noexcept {
    gap_x_ = std::max(0.0f, horizontal);
    gap_y_ = std::max(0.0f, vertical);
}

void GridPanel::add_cell(std::unique_ptr<Widget> child,
                         int column,
                         int row,
                         int column_span,
                         int row_span) {
    if (!child) {
        return;
    }
    const int safe_column = std::max(0, column);
    const int safe_row = std::max(0, row);
    Widget::add_child(std::move(child));
    cells_.push_back(Cell{safe_column,
                          safe_row,
                          std::max(1, column_span),
                          std::max(1, row_span)});
    const std::size_t required_columns = static_cast<std::size_t>(safe_column + column_span);
    while (column_weights_.size() < required_columns) {
        column_weights_.push_back(1.0f);
    }
    const std::size_t required_rows = static_cast<std::size_t>(safe_row + row_span);
    while (row_weights_.size() < required_rows) {
        row_weights_.push_back(1.0f);
    }
}

Size GridPanel::measure(Painter& painter, const Size& available) {
    if (cells_.size() != children().size()) {
        cells_.clear();
        for (std::size_t index = 0; index < children().size(); ++index) {
            const std::size_t columns = std::max<std::size_t>(1, column_weights_.size());
            cells_.push_back(Cell{static_cast<int>(index % columns),
                                  static_cast<int>(index / columns),
                                  1,
                                  1});
        }
    }

    int column_count = std::max<int>(1, static_cast<int>(column_weights_.size()));
    int row_count = std::max<int>(1, static_cast<int>(row_weights_.size()));
    for (const Cell& cell : cells_) {
        column_count = std::max(column_count, cell.column + cell.column_span);
        row_count = std::max(row_count, cell.row + cell.row_span);
    }
    while (static_cast<int>(column_weights_.size()) < column_count) {
        column_weights_.push_back(1.0f);
    }
    while (static_cast<int>(row_weights_.size()) < row_count) {
        row_weights_.push_back(1.0f);
    }

    const float inner_width =
        std::max(0.0f, available.width - padding_.left - padding_.right);
    const float inner_height =
        std::max(0.0f, available.height - padding_.top - padding_.bottom);
    const float total_gap_x = gap_x_ * static_cast<float>(column_count - 1);
    const float total_gap_y = gap_y_ * static_cast<float>(row_count - 1);
    const float usable_width = std::max(0.0f, inner_width - total_gap_x);

    // Weight 0 means an auto-sized column. This is used for compact controls
    // such as a dialog's action button or a form header action.
    std::vector<Size> preferred(children().size());
    for (std::size_t index = 0; index < children().size(); ++index) {
        if (children()[index]->visible()) {
            preferred[index] =
                children()[index]->measure(painter, Size{inner_width, inner_height});
        }
    }

    column_widths_.assign(static_cast<std::size_t>(column_count), 0.0f);
    float auto_width = 0.0f;
    float star_weight = 0.0f;
    for (int column = 0; column < column_count; ++column) {
        const float weight = column_weights_[static_cast<std::size_t>(column)];
        if (weight <= 0.0f) {
            float width = 0.0f;
            for (std::size_t index = 0; index < cells_.size() && index < preferred.size();
                 ++index) {
                const Cell& cell = cells_[index];
                if (cell.column == column && cell.column_span == 1) {
                    width = std::max(width, preferred[index].width);
                }
            }
            column_widths_[static_cast<std::size_t>(column)] = width;
            auto_width += width;
        } else {
            star_weight += weight;
        }
    }

    const float star_width = std::max(0.0f, usable_width - auto_width);
    const float total_weight = std::max(0.001f, star_weight);
    for (int column = 0; column < column_count; ++column) {
        const float weight = column_weights_[static_cast<std::size_t>(column)];
        if (weight > 0.0f) {
            column_widths_[static_cast<std::size_t>(column)] =
                star_width * weight / total_weight;
        }
    }

    row_heights_.assign(static_cast<std::size_t>(row_count), 0.0f);
    for (std::size_t index = 0; index < children().size(); ++index) {
        if (!children()[index]->visible()) {
            continue;
        }
        const Cell& cell = cells_[index];
        float cell_width = 0.0f;
        for (int column = cell.column; column < cell.column + cell.column_span; ++column) {
            cell_width += column_widths_[static_cast<std::size_t>(column)];
        }
        cell_width += gap_x_ * static_cast<float>(cell.column_span - 1);
        const Size size = children()[index]->measure(
            painter, Size{std::max(0.0f, cell_width), std::max(0.0f, inner_height)});
        float per_row = size.height / static_cast<float>(cell.row_span);
        for (int row = cell.row; row < cell.row + cell.row_span; ++row) {
            float& height = row_heights_[static_cast<std::size_t>(row)];
            height = std::max(height, per_row);
        }
    }

    float desired_height = Sum(row_heights_) + total_gap_y;
    if (stretch_rows_ && inner_height > desired_height) {
        const float extra = inner_height - desired_height;
        const float row_weight_total = std::max(0.001f, Sum(row_weights_));
        for (int row = 0; row < row_count; ++row) {
            row_heights_[static_cast<std::size_t>(row)] +=
                extra * row_weights_[static_cast<std::size_t>(row)] / row_weight_total;
        }
        desired_height = inner_height;
    }

    const Size desired{inner_width + padding_.left + padding_.right,
                       desired_height + padding_.top + padding_.bottom};
    set_desired_size(desired);
    return desired;
}

void GridPanel::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Rect inner = bounds.inset(padding_);
    const int column_count =
        std::max<int>(1, static_cast<int>(column_weights_.size()));
    const int row_count = std::max<int>(1, static_cast<int>(row_weights_.size()));
    const float total_gap_x = gap_x_ * static_cast<float>(column_count - 1);
    const float usable_width =
        std::max(0.0f, inner.width - total_gap_x);

    std::vector<float> column_x(static_cast<std::size_t>(column_count), inner.x);
    std::vector<float> column_width(static_cast<std::size_t>(column_count), 0.0f);
    float auto_width = 0.0f;
    float star_weight = 0.0f;
    for (int column = 0; column < column_count; ++column) {
        const float weight = column_weights_[static_cast<std::size_t>(column)];
        if (weight <= 0.0f) {
            const float width =
                column < static_cast<int>(column_widths_.size())
                    ? column_widths_[static_cast<std::size_t>(column)]
                    : 0.0f;
            column_width[static_cast<std::size_t>(column)] = width;
            auto_width += width;
        } else {
            star_weight += weight;
        }
    }
    const float star_width = std::max(0.0f, usable_width - auto_width);
    const float total_weight = std::max(0.001f, star_weight);
    float cursor_x = inner.x;
    for (int column = 0; column < column_count; ++column) {
        const float weight = column_weights_[static_cast<std::size_t>(column)];
        if (weight > 0.0f) {
            column_width[static_cast<std::size_t>(column)] =
                star_width * weight / total_weight;
        }
        column_x[static_cast<std::size_t>(column)] = cursor_x;
        cursor_x += column_width[static_cast<std::size_t>(column)] + gap_x_;
    }

    if (static_cast<int>(row_heights_.size()) != row_count) {
        row_heights_.assign(static_cast<std::size_t>(row_count), 0.0f);
    }

    std::vector<float> row_y(static_cast<std::size_t>(row_count), inner.y);
    float cursor_y = inner.y;
    for (int row = 0; row < row_count; ++row) {
        row_y[static_cast<std::size_t>(row)] = cursor_y;
        cursor_y += row_heights_[static_cast<std::size_t>(row)] + gap_y_;
    }

    for (std::size_t index = 0; index < children().size() && index < cells_.size(); ++index) {
        if (!children()[index]->visible()) {
            continue;
        }
        const Cell& cell = cells_[index];
        float width = 0.0f;
        for (int column = cell.column; column < cell.column + cell.column_span; ++column) {
            width += column_width[static_cast<std::size_t>(column)];
        }
        width += gap_x_ * static_cast<float>(cell.column_span - 1);
        float height = 0.0f;
        for (int row = cell.row; row < cell.row + cell.row_span; ++row) {
            height += row_heights_[static_cast<std::size_t>(row)];
        }
        height += gap_y_ * static_cast<float>(cell.row_span - 1);
        children()[index]->arrange(Rect{column_x[static_cast<std::size_t>(cell.column)],
                                       row_y[static_cast<std::size_t>(cell.row)],
                                       std::max(0.0f, width),
                                       std::max(0.0f, height)});
    }
}

void GridPanel::paint(Painter& painter) const {
    paint_children(painter);
}

ScrollBar::ScrollBar(Orientation orientation) : orientation_(orientation) {}

void ScrollBar::set_range(float content_size, float viewport_size) {
    content_size_ = std::max(0.0f, content_size);
    viewport_size_ = std::max(0.0f, viewport_size);
    value_ = std::clamp(value_, 0.0f, maximum());
}

float ScrollBar::maximum() const noexcept {
    return std::max(0.0f, content_size_ - viewport_size_);
}

void ScrollBar::set_value(float value) {
    SetValueInternal(value, true);
}

void ScrollBar::SetValueInternal(float value, bool notify) {
    const float clamped = std::clamp(value, 0.0f, maximum());
    if (std::abs(clamped - value_) < 0.01f) {
        return;
    }
    value_ = clamped;
    if (notify && on_scroll_) {
        on_scroll_(value_);
    }
}

Rect ScrollBar::thumb_rect() const noexcept {
    const bool vertical = orientation_ == Orientation::vertical;
    const float track_length = vertical ? bounds().height : bounds().width;
    if (track_length <= 0.0f) {
        return bounds();
    }
    const float ratio =
        content_size_ > 0.0f ? std::clamp(viewport_size_ / content_size_, 0.0f, 1.0f) : 1.0f;
    const float thumb_length = std::max(kMinThumbLength, track_length * ratio);
    const float available = std::max(0.0f, track_length - thumb_length);
    const float max_value = maximum();
    const float position = max_value > 0.0f ? std::clamp(value_ / max_value, 0.0f, 1.0f) : 0.0f;
    if (vertical) {
        return Rect{bounds().x + 2.0f,
                    bounds().y + available * position,
                    std::max(0.0f, bounds().width - 4.0f),
                    thumb_length};
    }
    return Rect{bounds().x + available * position,
                bounds().y + 2.0f,
                thumb_length,
                std::max(0.0f, bounds().height - 4.0f)};
}

void ScrollBar::on_mouse_down(Point point) {
    const Rect thumb = thumb_rect();
    const bool vertical = orientation_ == Orientation::vertical;
    if (thumb.contains(point)) {
        dragging_ = true;
        drag_offset_ = vertical ? point.y - thumb.y : point.x - thumb.x;
        return;
    }

    const float track_length = vertical ? bounds().height : bounds().width;
    const float thumb_length = vertical ? thumb.height : thumb.width;
    const float available = std::max(0.0f, track_length - thumb_length);
    if (available <= 0.0f || maximum() <= 0.0f) {
        return;
    }
    const float position = vertical ? point.y - bounds().y : point.x - bounds().x;
    const float ratio = std::clamp((position - thumb_length * 0.5f) / available, 0.0f, 1.0f);
    SetValueInternal(ratio * maximum(), true);
    dragging_ = true;
    drag_offset_ = thumb_length * 0.5f;
}

void ScrollBar::on_mouse_up(Point point) {
    (void)point;
    dragging_ = false;
}

void ScrollBar::on_mouse_move(Point point) {
    if (!dragging_) {
        return;
    }
    const bool vertical = orientation_ == Orientation::vertical;
    const float track_length = vertical ? bounds().height : bounds().width;
    const Rect thumb = thumb_rect();
    const float thumb_length = vertical ? thumb.height : thumb.width;
    const float available = std::max(0.0f, track_length - thumb_length);
    if (available <= 0.0f || maximum() <= 0.0f) {
        return;
    }
    const float position =
        (vertical ? point.y - bounds().y : point.x - bounds().x) - drag_offset_;
    const float ratio = std::clamp(position / available, 0.0f, 1.0f);
    SetValueInternal(ratio * maximum(), true);
}

void ScrollBar::on_mouse_leave() {
    dragging_ = false;
}

Size ScrollBar::measure(Painter& painter, const Size& available) {
    (void)painter;
    const bool vertical = orientation_ == Orientation::vertical;
    const Size desired = vertical
                             ? Size{kScrollBarThickness, std::max(kMinThumbLength, available.height)}
                             : Size{std::max(kMinThumbLength, available.width), kScrollBarThickness};
    set_desired_size(desired);
    return desired;
}

void ScrollBar::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void ScrollBar::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const float radius = std::min(6.0f, std::min(bounds().width, bounds().height) * 0.5f);
    painter.fill_rounded_rect(bounds(), radius, tokens.surface_soft);
    const Rect thumb = thumb_rect();
    const Color color = pressed() || dragging_ ? tokens.accent : tokens.border_strong;
    painter.fill_rounded_rect(thumb, std::max(3.0f, radius - 1.0f), color);
}

ScrollViewer::ScrollViewer(std::unique_ptr<Widget> content) {
    if (content) {
        content_ = content.get();
        add_child(std::move(content));
    }
    auto bar = std::make_unique<ScrollBar>(Orientation::vertical);
    bar_ = bar.get();
    bar_->set_on_scroll([this](float value) {
        offset_ = value;
        ApplyScroll();
    });
    add_child(std::move(bar));
}

bool ScrollViewer::focusable() const noexcept {
    return false;
}

float ScrollViewer::maximum_scroll() const noexcept {
    return std::max(0.0f, content_height_ - viewport_.height);
}

void ScrollViewer::scroll_by(float delta) {
    scroll_to(offset_ + delta);
}

void ScrollViewer::scroll_to(float offset) {
    offset_ = std::clamp(offset, 0.0f, maximum_scroll());
    UpdateBar();
    ApplyScroll();
}

bool ScrollViewer::on_mouse_wheel(float delta) {
    if (!scrollable_ || delta == 0.0f) {
        return false;
    }
    scroll_by(-delta * 64.0f);
    return true;
}

Widget* ScrollViewer::hit_test(Point point) const {
    if (!visible() || !bounds().contains(point)) {
        return nullptr;
    }
    if (bar_ != nullptr && bar_->visible()) {
        if (Widget* hit = bar_->hit_test(point)) {
            return hit;
        }
    }
    if (viewport_.contains(point) && content_ != nullptr) {
        if (Widget* hit = content_->hit_test(point)) {
            return hit;
        }
    }
    return accepts_mouse() ? const_cast<ScrollViewer*>(this) : nullptr;
}

Size ScrollViewer::measure(Painter& painter, const Size& available) {
    const float inner_width =
        std::max(0.0f, available.width - padding_.left - padding_.right);
    const float inner_height =
        std::max(0.0f, available.height - padding_.top - padding_.bottom);
    if (content_ == nullptr) {
        content_height_ = 0.0f;
        scrollable_ = false;
        const Size desired{available.width, available.height};
        set_desired_size(desired);
        return desired;
    }

    const Size first_pass =
        content_->measure(painter, Size{inner_width, ContentHeightLimit()});
    content_height_ = first_pass.height;
    scrollable_ = content_height_ > inner_height + 0.5f;

    const float content_width = scrollable_ ? std::max(0.0f, inner_width - kScrollBarThickness)
                                            : inner_width;
    const Size content_size =
        content_->measure(painter, Size{content_width, ContentHeightLimit()});
    content_height_ = content_size.height;
    scrollable_ = content_height_ > inner_height + 0.5f;
    if (scrollable_) {
        const Size remeasured = content_->measure(
            painter, Size{std::max(0.0f, inner_width - kScrollBarThickness), ContentHeightLimit()});
        content_height_ = remeasured.height;
    }

    offset_ = std::clamp(offset_, 0.0f, std::max(0.0f, content_height_ - inner_height));
    const Size desired{std::min(available.width, content_width + padding_.left + padding_.right),
                       std::min(available.height,
                                content_height_ + padding_.top + padding_.bottom)};
    set_desired_size(desired);
    return desired;
}

void ScrollViewer::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Rect inner = bounds.inset(padding_);
    viewport_ = Rect{inner.x,
                     inner.y,
                     std::max(0.0f, inner.width - (scrollable_ ? kScrollBarThickness : 0.0f)),
                     std::max(0.0f, inner.height)};

    if (bar_ != nullptr) {
        bar_->set_visible(scrollable_);
        if (scrollable_) {
            bar_->arrange(Rect{bounds.right() - padding_.right - kScrollBarThickness,
                               inner.y,
                               kScrollBarThickness,
                               std::max(0.0f, inner.height)});
        }
    }
    if (content_ != nullptr) {
        content_->arrange(Rect{viewport_.x,
                               viewport_.y - offset_,
                               viewport_.width,
                               std::max(content_height_, viewport_.height)});
    }
    UpdateBar();
}

void ScrollViewer::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    if (background_ != PanelStyle::none) {
        const Color fill = background_ == PanelStyle::soft ? tokens.surface_soft : tokens.surface;
        painter.fill_rounded_rect(bounds(), 8.0f, fill);
        painter.stroke_rounded_rect(bounds(), 8.0f, tokens.border, 1.0f);
    }

    painter.push_clip(viewport_);
    if (content_ != nullptr) {
        content_->paint(painter);
    }
    painter.pop_clip();

    if (bar_ != nullptr && bar_->visible()) {
        bar_->paint(painter);
    }
}

void ScrollViewer::UpdateBar() {
    if (bar_ == nullptr) {
        return;
    }
    bar_->set_range(content_height_, viewport_.height);
    bar_->set_value(offset_);
}

void ScrollViewer::ApplyScroll() {
    if (content_ == nullptr) {
        return;
    }
    content_->arrange(Rect{viewport_.x,
                           viewport_.y - offset_,
                           viewport_.width,
                           std::max(content_height_, viewport_.height)});
    if (bar_ != nullptr) {
        bar_->set_value(offset_);
    }
}

}  // namespace cppui
