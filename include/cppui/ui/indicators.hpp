#pragma once

#include <functional>
#include <string>
#include <utility>

#include <cppui/ui/widget.hpp>

namespace cppui {

class Painter;

class ProgressBar final : public Widget {
public:
    explicit ProgressBar(float value = 0.0f);

    void set_value(float value) noexcept;
    [[nodiscard]] float value() const noexcept { return value_; }
    void set_show_text(bool show) noexcept { show_text_ = show; }
    [[nodiscard]] bool show_text() const noexcept { return show_text_; }
    void set_preferred_width(float width) noexcept { preferred_width_ = width; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    float value_{};
    float preferred_width_{240.0f};
    bool show_text_{true};
};

class ProgressRing final : public Widget {
public:
    explicit ProgressRing(float progress = -1.0f);

    void set_progress(float progress) noexcept { progress_ = progress; }
    [[nodiscard]] float progress() const noexcept { return progress_; }
    void set_diameter(float diameter) noexcept { diameter_ = diameter; }
    [[nodiscard]] bool animating() const noexcept override { return progress_ < 0.0f; }
    void on_tick(float seconds) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    float progress_{-1.0f};
    float diameter_{24.0f};
    float phase_{};
};

class Loader final : public Widget {
public:
    explicit Loader(std::wstring text = {});

    void set_text(std::wstring text) { text_ = std::move(text); }
    void set_diameter(float diameter) noexcept { ring_.set_diameter(diameter); }
    void set_ring_visible(bool visible) noexcept { ring_visible_ = visible; }

    [[nodiscard]] bool animating() const noexcept override { return ring_.animating(); }
    void on_tick(float seconds) override;
    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::wstring text_;
    ProgressRing ring_;
    bool ring_visible_{true};
};

// Full-surface blocking overlay. Add it as the last child of a Panel so it
// paints above the form and captures all mouse input.
class BusyOverlay final : public Widget {
public:
    BusyOverlay() = default;
    explicit BusyOverlay(std::wstring message);

    void set_message(std::wstring message) { message_ = std::move(message); }
    void set_cancel_text(std::wstring text) { cancel_text_ = std::move(text); }
    void set_on_cancel(std::function<void()> callback) { on_cancel_ = std::move(callback); }
    void set_progress(float progress) noexcept { progress_ = progress; }

    [[nodiscard]] bool accepts_mouse() const noexcept override { return true; }
    [[nodiscard]] bool focus_trap() const noexcept override { return true; }
    [[nodiscard]] bool animating() const noexcept override { return progress_ < 0.0f; }
    void on_tick(float seconds) override;
    void on_mouse_down(Point point) override;
    [[nodiscard]] bool activate_on_click() const noexcept override { return false; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    [[nodiscard]] Rect card_rect() const noexcept;
    [[nodiscard]] Rect cancel_rect() const noexcept;

    std::wstring message_{L"Working..."};
    std::wstring cancel_text_;
    std::function<void()> on_cancel_;
    float progress_{-1.0f};
    float phase_{};
};

}  // namespace cppui
