#include <cppui/ui/image.hpp>

#include <algorithm>
#include <utility>

#include <cppui/ui/painter.hpp>

namespace cppui {

PictureBox::PictureBox(std::shared_ptr<Image> image) : image_(std::move(image)) {}

Size PictureBox::measure(Painter& painter, const Size& available) {
    (void)painter;
    const float width = available.width > 0.0f
                            ? std::min(preferred_size_.width, available.width)
                            : preferred_size_.width;
    const float height = available.height > 0.0f
                             ? std::min(preferred_size_.height, available.height)
                             : preferred_size_.height;
    const Size desired{std::max(32.0f, width), std::max(32.0f, height)};
    set_desired_size(desired);
    return desired;
}

void PictureBox::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void PictureBox::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    painter.fill_rounded_rect(bounds(), corner_radius_, tokens.surface_soft);
    if (image_ == nullptr || !image_->valid() || bounds().width <= 1.0f || bounds().height <= 1.0f) {
        if (!placeholder_.empty()) {
            painter.draw_text(placeholder_,
                              bounds(),
                              FontRole::caption,
                              tokens.text_tertiary,
                              TextAlign::center,
                              TextVAlign::center);
        }
        painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border, 1.0f);
        return;
    }

    Rect destination = bounds();
    if (fit_ != ImageFit::stretch) {
        const float available_width = std::max(0.0f, bounds().width);
        const float available_height = std::max(0.0f, bounds().height);
        const float image_width = static_cast<float>(image_->width());
        const float image_height = static_cast<float>(image_->height());
        const float scale_x = available_width / image_width;
        const float scale_y = available_height / image_height;
        const float scale =
            fit_ == ImageFit::cover ? std::max(scale_x, scale_y) : std::min(scale_x, scale_y);
        const float draw_width = image_width * scale;
        const float draw_height = image_height * scale;
        destination = Rect{bounds().x + (bounds().width - draw_width) * 0.5f,
                           bounds().y + (bounds().height - draw_height) * 0.5f,
                           draw_width,
                           draw_height};
    }

    painter.push_clip(bounds());
    painter.draw_image(destination, *image_, 1.0f);
    painter.pop_clip();
    painter.stroke_rounded_rect(bounds(), corner_radius_, tokens.border, 1.0f);
}

}  // namespace cppui
