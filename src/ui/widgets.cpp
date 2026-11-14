#include <cppui/ui/widgets.hpp>

#include <algorithm>
#include <utility>

namespace cppui {
namespace {

constexpr int kKeyEnter = 0x0D;
constexpr int kKeySpace = 0x20;
constexpr Color kTransparent{0, 0, 0, 0};

}  // namespace

StackPanel::StackPanel(Orientation orientation, float spacing)
    : orientation_(orientation), spacing_(spacing) {}

Size StackPanel::measure(Painter& painter, const Size& available) {
    child_sizes_.clear();

    const float inner_width = std::max(0.0f, available.width - padding_.left - padding_.right);
    const float inner_height = std::max(0.0f, available.height - padding_.top - padding_.bottom);
    const Size inner_available{inner_width, inner_height};

    float main = 0.0f;
    float cross = 0.0f;
    for (const auto& child : children()) {
        const Size size = child->measure(painter, inner_available);
        child_sizes_.push_back(size);
        if (orientation_ == Orientation::vertical) {
            main += size.height;
            cross = std::max(cross, size.width);
        } else {
            main += size.width;
            cross = std::max(cross, size.height);
        }
    }

    if (!child_sizes_.empty()) {
        main += spacing_ * static_cast<float>(child_sizes_.size() - 1);
    }

    const Size desired = orientation_ == Orientation::vertical
                             ? Size{cross + padding_.left + padding_.right,
                                    main + padding_.top + padding_.bottom}
                             : Size{main + padding_.left + padding_.right,
                                    cross + padding_.top + padding_.bottom};
    set_desired_size(desired);
    return desired;
}

void StackPanel::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Rect inner = bounds.inset(padding_);
    const float inner_width = std::max(0.0f, inner.width);
    const float inner_height = std::max(0.0f, inner.height);

    float cursor = orientation_ == Orientation::vertical ? inner.y : inner.x;
    for (std::size_t i = 0; i < child_sizes_.size() && i < children().size(); ++i) {
        const Size size = child_sizes_[i];
        Widget* child = children()[i].get();

        if (orientation_ == Orientation::vertical) {
            float width = size.width;
            if (cross_align_ == CrossAlign::stretch) {
                width = inner_width;
            } else {
                width = std::min(width, inner_width);
            }

            float x = inner.x;
            if (cross_align_ == CrossAlign::center) {
                x += (inner_width - width) * 0.5f;
            }

            child->arrange(Rect{x, cursor, width, size.height});
            cursor += size.height + spacing_;
        } else {
            float height = size.height;
            if (cross_align_ == CrossAlign::stretch) {
                height = inner_height;
            } else {
                height = std::min(height, inner_height);
            }

            float y = inner.y;
            if (cross_align_ == CrossAlign::center) {
                y += (inner_height - height) * 0.5f;
            }

            child->arrange(Rect{cursor, y, size.width, height});
            cursor += size.width + spacing_;
        }
    }
}

void StackPanel::paint(Painter& painter) const {
    paint_children(painter);
}

Size CardPanel::measure(Painter& painter, const Size& available) {
    const float outer_width = std::min(preferred_width_, std::max(0.0f, available.width));
    const float inner_width = std::max(0.0f, outer_width - padding_.left - padding_.right);
    const Size inner_available{inner_width, std::max(0.0f, available.height - padding_.top - padding_.bottom)};

    float content_height = 0.0f;
    if (!children().empty()) {
        const Size content = children().front()->measure(painter, inner_available);
        content_height = content.height;
    }

    const Size desired{outer_width, content_height + padding_.top + padding_.bottom};
    set_desired_size(desired);
    return desired;
}

void CardPanel::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Rect inner = bounds.inset(padding_);
    if (!children().empty()) {
        children().front()->arrange(Rect{inner.x, inner.y, std::max(0.0f, inner.width),
                                         std::max(0.0f, inner.height)});
    }
}

void CardPanel::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    painter.fill_rounded_rect(bounds(), corner_radius_, tokens.surface);
    painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border, 1.0f);
    paint_children(painter);
}

Label::Label(std::wstring text, FontRole role, LabelStyle style)
    : text_(std::move(text)), role_(role), style_(style) {}

Size Label::measure(Painter& painter, const Size& available) {
    (void)available;
    const Size text_size = painter.measure_text(text_, role_);
    const Size desired{text_size.width, text_size.height};
    set_desired_size(desired);
    return desired;
}

void Label::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void Label::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    Color color = tokens.text_primary;
    switch (style_) {
        case LabelStyle::primary:
            color = tokens.text_primary;
            break;
        case LabelStyle::secondary:
            color = tokens.text_secondary;
            break;
        case LabelStyle::accent:
            color = tokens.accent;
            break;
        case LabelStyle::muted:
            color = tokens.text_tertiary;
            break;
    }

    painter.draw_text(text_, bounds(), role_, color, TextAlign::leading, TextVAlign::center);
}

Button::Button(std::wstring text, ButtonStyle style)
    : text_(std::move(text)), style_(style) {}

void Button::on_activate() {
    if (enabled_ && on_click_) {
        on_click_();
    }
}

bool Button::on_key_down(int key, bool shift) {
    (void)shift;
    if (!enabled_) {
        return false;
    }

    if (key == kKeySpace || key == kKeyEnter) {
        on_activate();
        return true;
    }
    return false;
}

Size Button::measure(Painter& painter, const Size& available) {
    (void)available;
    const Size text_size = painter.measure_text(text_, FontRole::label);
    const Size desired{std::max(76.0f, text_size.width + 28.0f), 34.0f};
    set_desired_size(desired);
    return desired;
}

void Button::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void Button::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();

    Color fill = tokens.surface;
    Color border = tokens.border;
    Color text = tokens.text_primary;

    if (!enabled_) {
        fill = tokens.surface_soft;
        border = tokens.border;
        text = tokens.text_tertiary;
    } else {
        switch (style_) {
            case ButtonStyle::primary:
                fill = pressed() ? tokens.accent : (hovered() ? tokens.primary_top : tokens.primary);
                border = fill;
                text = tokens.on_accent;
                break;
            case ButtonStyle::secondary:
                fill = pressed() ? tokens.accent_soft : (hovered() ? tokens.surface_hover : tokens.surface);
                border = hovered() ? tokens.accent : tokens.border;
                text = tokens.text_primary;
                break;
            case ButtonStyle::ghost:
                fill = pressed() ? tokens.accent_soft : (hovered() ? tokens.surface_hover : kTransparent);
                border = kTransparent;
                text = hovered() ? tokens.accent : tokens.accent;
                break;
        }
    }

    painter.fill_rounded_rect(bounds(), 7.0f, fill);
    if (border.a > 0) {
        painter.stroke_rounded_rect(bounds(), 7.0f, border, 1.0f);
    }

    painter.draw_text(text_,
                      bounds(),
                      FontRole::label,
                      text,
                      TextAlign::center,
                      TextVAlign::center);

    if (enabled_ && focused()) {
        const Rect focus_rect{bounds().x + 1.5f,
                              bounds().y + 1.5f,
                              std::max(0.0f, bounds().width - 3.0f),
                              std::max(0.0f, bounds().height - 3.0f)};
        painter.stroke_rounded_rect(focus_rect, 6.0f, tokens.accent, 1.5f);
    }
}

}  // namespace cppui
