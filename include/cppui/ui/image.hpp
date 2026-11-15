#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <cppui/ui/widget.hpp>

namespace cppui {

class Painter;

// Immutable premultiplied BGRA image. Loading is platform-backed (WIC), while
// the class itself and its pixels stay independent of COM.
class Image final {
public:
    [[nodiscard]] static std::shared_ptr<Image> LoadFromFile(const std::wstring& path,
                                                             std::wstring* error = nullptr);
    [[nodiscard]] static std::shared_ptr<Image> FromPixels(int width,
                                                           int height,
                                                           std::vector<std::uint8_t> bgra_premultiplied);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] bool valid() const noexcept { return width_ > 0 && height_ > 0 && !pixels_.empty(); }
    [[nodiscard]] const std::vector<std::uint8_t>& pixels() const noexcept { return pixels_; }

private:
    Image(int width, int height, std::vector<std::uint8_t> pixels);

    std::uint64_t id_{};
    int width_{};
    int height_{};
    std::vector<std::uint8_t> pixels_;
};

enum class ImageFit {
    contain,
    cover,
    stretch,
};

class PictureBox final : public Widget {
public:
    PictureBox() = default;
    explicit PictureBox(std::shared_ptr<Image> image);

    void set_image(std::shared_ptr<Image> image) { image_ = std::move(image); }
    [[nodiscard]] const std::shared_ptr<Image>& image() const noexcept { return image_; }
    void set_fit(ImageFit fit) noexcept { fit_ = fit; }
    void set_preferred_size(Size size) noexcept { preferred_size_ = size; }
    void set_corner_radius(float radius) noexcept { corner_radius_ = radius; }
    void set_placeholder(std::wstring text) {
        placeholder_ = std::move(text);
        set_accessible_name(placeholder_);
    }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return !tooltip().empty(); }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::shared_ptr<Image> image_;
    ImageFit fit_{ImageFit::contain};
    Size preferred_size_{180.0f, 120.0f};
    float corner_radius_{8.0f};
    std::wstring placeholder_{L"Image"};
};

}  // namespace cppui
