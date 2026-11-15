#include <cppui/ui/widgets.hpp>

#include <algorithm>
#include <utility>

#include <cppui/ui/command.hpp>
#include <cppui/ui/image.hpp>

namespace cppui {
namespace {

constexpr int kKeyEnter = 0x0D;
constexpr int kKeySpace = 0x20;
constexpr Color kTransparent{0, 0, 0, 0};

std::vector<std::wstring> WrapLabelText(Painter& painter,
                                        std::wstring_view text,
                                        FontRole role,
                                        float max_width) {
    std::vector<std::wstring> lines;
    if (max_width <= 0.0f) {
        lines.emplace_back(text);
        return lines;
    }

    std::wstring current;
    std::size_t index = 0;
    while (index <= text.size()) {
        std::size_t end = text.find_first_of(L" \n", index);
        if (end == std::wstring_view::npos) {
            end = text.size();
        }
        const std::wstring word(text.substr(index, end - index));
        const wchar_t delimiter = end < text.size() ? text[end] : L'\0';

        if (!word.empty()) {
            const std::wstring candidate =
                current.empty() ? word : current + L" " + word;
            if (painter.measure_text(candidate, role).width <= max_width || current.empty()) {
                current = candidate;
            } else {
                lines.push_back(current);
                current = word;
            }
        }

        if (delimiter == L'\n') {
            lines.push_back(current);
            current.clear();
        }
        if (end == text.size()) {
            break;
        }
        index = end + 1;
    }
    if (!current.empty() || lines.empty()) {
        lines.push_back(current);
    }
    return lines;
}

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
        const Size size =
            child->visible() ? child->measure(painter, inner_available) : Size{};
        child_sizes_.push_back(size);
        if (!child->visible()) {
            continue;
        }
        if (orientation_ == Orientation::vertical) {
            main += size.height;
            cross = std::max(cross, size.width);
        } else {
            main += size.width;
            cross = std::max(cross, size.height);
        }
    }

    std::size_t visible_count = 0;
    for (const auto& child : children()) {
        if (child->visible()) {
            ++visible_count;
        }
    }
    if (visible_count > 0) {
        main += spacing_ * static_cast<float>(visible_count - 1);
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
        if (!child->visible()) {
            continue;
        }

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
    if (!children().empty() && children().front()->visible()) {
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
    if (!children().empty() && children().front()->visible()) {
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
    const float line_height = painter.measure_text(L"Ag", role_).height;
    if (!wrap_ || available.width <= 0.0f) {
        const Size text_size = painter.measure_text(text_, role_);
        const Size desired{text_size.width, std::max(text_size.height, line_height)};
        set_desired_size(desired);
        return desired;
    }

    const std::vector<std::wstring> lines =
        WrapLabelText(painter, text_, role_, available.width);
    set_desired_size(Size{available.width, line_height * static_cast<float>(lines.size())});
    return desired_size();
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
        case LabelStyle::danger:
            color = tokens.danger;
            break;
    }

    if (!wrap_) {
        painter.draw_text(text_, bounds(), role_, color, TextAlign::leading, TextVAlign::center);
        return;
    }

    const std::vector<std::wstring> lines =
        WrapLabelText(painter, text_, role_, bounds().width);
    const float line_height = painter.measure_text(L"Ag", role_).height;
    float y = bounds().y;
    for (const std::wstring& line : lines) {
        painter.draw_text(line,
                          Rect{bounds().x, y, bounds().width, line_height},
                          role_,
                          color,
                          TextAlign::leading,
                          TextVAlign::center);
        y += line_height;
    }
}

Button::Button(std::wstring text, ButtonStyle style)
    : text_(std::move(text)), style_(style) {}

bool Button::enabled() const noexcept {
    return EffectiveEnabled();
}

bool Button::EffectiveEnabled() const noexcept {
    return enabled_ && (command_ == nullptr || command_->enabled());
}

std::wstring Button::EffectiveText() const {
    if (command_ != nullptr && !command_->text().empty()) {
        return command_->text();
    }
    return text_;
}

void Button::on_activate() {
    if (!enabled()) {
        return;
    }
    if (command_ != nullptr) {
        (void)command_->Execute();
        return;
    }
    if (on_click_) {
        on_click_();
    }
}

bool Button::on_key_down(int key, KeyModifiers modifiers) {
    (void)modifiers;
    if (!enabled()) {
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
    const DensityMetrics metrics = density_metrics(painter.density());
    const Size text_size = painter.measure_text(EffectiveText(), FontRole::label);
    const float icon_width = icon_ != nullptr ? 18.0f + 8.0f : 0.0f;
    const Size desired{std::max(metrics.min_control_width,
                                text_size.width + icon_width + metrics.control_padding * 2.0f),
                       metrics.control_height};
    set_desired_size(desired);
    return desired;
}

void Button::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void Button::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const DensityMetrics metrics = density_metrics(painter.density());

    Color fill = tokens.surface;
    Color border = tokens.border;
    Color text = tokens.text_primary;

    if (!enabled()) {
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

    painter.fill_rounded_rect(bounds(), metrics.control_radius, fill);
    if (border.a > 0) {
        painter.stroke_rounded_rect(bounds(), metrics.control_radius, border, 1.0f);
    }

    const std::wstring button_text = EffectiveText();
    const bool has_icon = icon_ != nullptr && icon_->valid();
    if (has_icon) {
        const Size text_size = painter.measure_text(button_text, FontRole::label);
        const float icon_size = 18.0f;
        const float total = icon_size + 8.0f + text_size.width;
        const float start = bounds().x + (bounds().width - total) * 0.5f;
        const Rect icon_rect{start, bounds().y + (bounds().height - icon_size) * 0.5f, icon_size,
                             icon_size};
        painter.draw_image(icon_rect,
                           *icon_,
                           enabled() ? 1.0f : 0.45f);
        painter.draw_text(button_text,
                          Rect{icon_rect.right() + 8.0f,
                               bounds().y,
                               std::max(0.0f, bounds().right() - icon_rect.right() - 8.0f),
                               bounds().height},
                          FontRole::label,
                          text,
                          TextAlign::leading,
                          TextVAlign::center);
    } else {
        painter.draw_text(button_text,
                          bounds(),
                          FontRole::label,
                          text,
                          TextAlign::center,
                          TextVAlign::center);
    }

    if (enabled() && focused()) {
        const Rect focus_rect{bounds().x + 1.5f,
                              bounds().y + 1.5f,
                              std::max(0.0f, bounds().width - 3.0f),
                              std::max(0.0f, bounds().height - 3.0f)};
        painter.stroke_rounded_rect(focus_rect,
                                    std::max(1.0f, metrics.control_radius - 1.0f),
                                    tokens.accent,
                                    1.5f);
    }
}

}  // namespace cppui
