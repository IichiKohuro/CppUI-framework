#include <cppui/ui/indicators.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include <cppui/ui/painter.hpp>

namespace cppui {
namespace {

constexpr float kPi = 3.14159265358979323846f;

void DrawArc(Painter& painter,
             Point center,
             float radius,
             float start_angle,
             float sweep,
             const Color& color,
             float width) {
    if (radius <= 0.0f || std::abs(sweep) < 0.001f) {
        return;
    }
    const int segments = std::max(6, static_cast<int>(std::ceil(std::abs(sweep) / 0.18f)));
    Point previous{center.x + std::cos(start_angle) * radius,
                   center.y + std::sin(start_angle) * radius};
    for (int segment = 1; segment <= segments; ++segment) {
        const float angle =
            start_angle + sweep * static_cast<float>(segment) / static_cast<float>(segments);
        const Point current{center.x + std::cos(angle) * radius,
                            center.y + std::sin(angle) * radius};
        painter.draw_line(previous, current, color, width);
        previous = current;
    }
}

}  // namespace

ProgressBar::ProgressBar(float value) : value_(std::clamp(value, 0.0f, 1.0f)) {}

void ProgressBar::set_value(float value) noexcept {
    value_ = std::clamp(value, 0.0f, 1.0f);
}

Size ProgressBar::measure(Painter& painter, const Size& available) {
    const DensityMetrics metrics = density_metrics(painter.density());
    const float width = available.width > 0.0f ? available.width : preferred_width_;
    const Size desired{std::max(80.0f, width),
                       show_text_ ? std::max(24.0f, metrics.control_height - 8.0f) : 8.0f};
    set_desired_size(desired);
    return desired;
}

void ProgressBar::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void ProgressBar::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const float label_width = show_text_ ? 48.0f : 0.0f;
    const float bar_height = std::min(8.0f, std::max(4.0f, bounds().height));
    const Rect track{bounds().x,
                     bounds().y + (bounds().height - bar_height) * 0.5f,
                     std::max(0.0f, bounds().width - label_width),
                     bar_height};
    painter.fill_rounded_rect(track, bar_height * 0.5f, tokens.surface_soft);
    const float fill_width = track.width * std::clamp(value_, 0.0f, 1.0f);
    if (fill_width > 0.5f) {
        painter.fill_rounded_rect(Rect{track.x, track.y, fill_width, track.height},
                                  bar_height * 0.5f,
                                  tokens.accent);
    }

    if (show_text_) {
        const int percent = static_cast<int>(std::lround(std::clamp(value_, 0.0f, 1.0f) * 100.0f));
        const std::wstring label = std::to_wstring(percent) + L"%";
        painter.draw_text(label,
                          Rect{bounds().right() - label_width,
                               bounds().y,
                               label_width,
                               bounds().height},
                          FontRole::caption,
                          tokens.text_secondary,
                          TextAlign::trailing,
                          TextVAlign::center);
    }
}

ProgressRing::ProgressRing(float progress) : progress_(progress) {}

void ProgressRing::on_tick(float seconds) {
    phase_ += seconds * 1.6f;
    if (phase_ > 1.0f) {
        phase_ -= std::floor(phase_);
    }
}

Size ProgressRing::measure(Painter& painter, const Size& available) {
    (void)painter;
    (void)available;
    const Size desired{diameter_, diameter_};
    set_desired_size(desired);
    return desired;
}

void ProgressRing::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void ProgressRing::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const float radius = std::max(1.0f,
                                  std::min(bounds().width, bounds().height) * 0.5f - 2.0f);
    const Point center{bounds().x + bounds().width * 0.5f, bounds().y + bounds().height * 0.5f};
    DrawArc(painter, center, radius, 0.0f, 2.0f * kPi, tokens.surface_soft, 2.0f);

    if (progress_ >= 0.0f) {
        const float value = std::clamp(progress_, 0.0f, 1.0f);
        if (value > 0.0f) {
            DrawArc(painter, center, radius, -kPi * 0.5f, 2.0f * kPi * value, tokens.accent, 2.4f);
        }
    } else {
        const float start = -kPi * 0.5f + phase_ * 2.0f * kPi;
        DrawArc(painter, center, radius, start, kPi * 0.7f, tokens.accent, 2.4f);
    }
}

Loader::Loader(std::wstring text) : text_(std::move(text)) {
    ring_.set_diameter(18.0f);
}

void Loader::on_tick(float seconds) {
    ring_.on_tick(seconds);
}

Size Loader::measure(Painter& painter, const Size& available) {
    (void)available;
    const Size text_size = painter.measure_text(text_, FontRole::body);
    const float ring = 18.0f;
    const Size desired{ring + (text_.empty() ? 0.0f : 10.0f) + text_size.width,
                       std::max(ring, text_size.height)};
    set_desired_size(desired);
    return desired;
}

void Loader::arrange(const Rect& bounds) {
    set_bounds(bounds);
    if (ring_visible_) {
        ring_.arrange(Rect{bounds.x,
                           bounds.y + (bounds.height - 18.0f) * 0.5f,
                           18.0f,
                           18.0f});
    }
}

void Loader::paint(Painter& painter) const {
    float text_x = bounds().x;
    if (ring_visible_) {
        ring_.paint(painter);
        text_x += 18.0f + 10.0f;
    }
    if (!text_.empty()) {
        painter.draw_text(text_,
                          Rect{text_x,
                               bounds().y,
                               std::max(0.0f, bounds().right() - text_x),
                               bounds().height},
                          FontRole::body,
                          painter.theme().text_secondary,
                          TextAlign::leading,
                          TextVAlign::center);
    }
}

BusyOverlay::BusyOverlay(std::wstring message) : message_(std::move(message)) {}

void BusyOverlay::on_tick(float seconds) {
    phase_ += seconds * 1.6f;
    if (phase_ > 1.0f) {
        phase_ -= std::floor(phase_);
    }
}

void BusyOverlay::on_mouse_down(Point point) {
    if (!cancel_text_.empty() && cancel_rect().contains(point) && on_cancel_) {
        on_cancel_();
    }
}

Size BusyOverlay::measure(Painter& painter, const Size& available) {
    (void)painter;
    set_desired_size(available);
    return available;
}

void BusyOverlay::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

Rect BusyOverlay::card_rect() const noexcept {
    const float width = std::min(380.0f, std::max(0.0f, bounds().width - 32.0f));
    const float height = std::min(150.0f, std::max(0.0f, bounds().height - 32.0f));
    return Rect{bounds().x + (bounds().width - width) * 0.5f,
                bounds().y + (bounds().height - height) * 0.5f,
                width,
                height};
}

Rect BusyOverlay::cancel_rect() const noexcept {
    const Rect card = card_rect();
    return Rect{card.right() - 104.0f, card.bottom() - 46.0f, 88.0f, 30.0f};
}

void BusyOverlay::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    Color veil = tokens.window_bg;
    veil.a = 190;
    painter.fill_rect(bounds(), veil);

    const Rect card = card_rect();
    painter.fill_rounded_rect(card, 12.0f, tokens.surface);
    painter.stroke_rounded_rect(card, 12.0f, tokens.border_strong, 1.0f);

    const Point center{card.x + 36.0f, card.y + 48.0f};
    const float radius = 12.0f;
    DrawArc(painter, center, radius, 0.0f, 2.0f * kPi, tokens.surface_soft, 2.0f);
    if (progress_ >= 0.0f) {
        DrawArc(painter,
                center,
                radius,
                -kPi * 0.5f,
                2.0f * kPi * std::clamp(progress_, 0.0f, 1.0f),
                tokens.accent,
                2.6f);
    } else {
        DrawArc(painter,
                center,
                radius,
                -kPi * 0.5f + phase_ * 2.0f * kPi,
                kPi * 0.7f,
                tokens.accent,
                2.6f);
    }

    painter.draw_text(message_,
                      Rect{card.x + 64.0f, card.y + 24.0f, card.width - 88.0f, 48.0f},
                      FontRole::heading,
                      tokens.text_primary,
                      TextAlign::leading,
                      TextVAlign::center);

    if (progress_ >= 0.0f) {
        const int percent = static_cast<int>(std::lround(std::clamp(progress_, 0.0f, 1.0f) * 100.0f));
        painter.draw_text(std::to_wstring(percent) + L"%",
                          Rect{card.x + 64.0f, card.y + 68.0f, card.width - 88.0f, 24.0f},
                          FontRole::caption,
                          tokens.text_secondary,
                          TextAlign::leading,
                          TextVAlign::center);
    }

    if (!cancel_text_.empty()) {
        const Rect cancel = cancel_rect();
        painter.fill_rounded_rect(cancel, 7.0f, tokens.surface_soft);
        painter.stroke_rounded_rect(cancel, 7.0f, tokens.border, 1.0f);
        painter.draw_text(cancel_text_,
                          cancel,
                          FontRole::label,
                          tokens.text_primary,
                          TextAlign::center,
                          TextVAlign::center);
    }
}

}  // namespace cppui
