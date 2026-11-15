#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <cppui/ui/painter.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {

class Image;
class Command;

enum class CrossAlign {
    start,
    center,
    stretch,
};

class StackPanel final : public Widget {
public:
    explicit StackPanel(Orientation orientation = Orientation::vertical, float spacing = 0.0f);

    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_spacing(float spacing) noexcept { spacing_ = spacing; }
    void set_cross_align(CrossAlign align) noexcept { cross_align_ = align; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    Orientation orientation_;
    float spacing_{};
    Insets padding_{};
    CrossAlign cross_align_{CrossAlign::start};
    std::vector<Size> child_sizes_;
};

class CardPanel final : public Widget {
public:
    CardPanel() = default;

    void set_preferred_width(float width) noexcept { preferred_width_ = width; }
    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_corner_radius(float radius) noexcept { corner_radius_ = radius; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    float preferred_width_{560.0f};
    float corner_radius_{16.0f};
    Insets padding_{24.0f, 24.0f, 24.0f, 24.0f};
};

enum class LabelStyle {
    primary,
    secondary,
    accent,
    muted,
    danger,
};

class Label final : public Widget {
public:
    Label(std::wstring text, FontRole role = FontRole::body, LabelStyle style = LabelStyle::primary);

    void set_text(std::wstring text) { text_ = std::move(text); }
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }
    void set_wrap(bool wrap) noexcept { wrap_ = wrap; }
    [[nodiscard]] bool wrap() const noexcept { return wrap_; }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return !tooltip().empty(); }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::wstring text_;
    FontRole role_;
    LabelStyle style_;
    bool wrap_{false};
};

enum class ButtonStyle {
    primary,
    secondary,
    ghost,
};

class Button final : public Widget {
public:
    explicit Button(std::wstring text, ButtonStyle style = ButtonStyle::secondary);

    void set_text(std::wstring text) { text_ = std::move(text); }
    void set_icon(std::shared_ptr<Image> icon) { icon_ = std::move(icon); }
    [[nodiscard]] const std::shared_ptr<Image>& icon() const noexcept { return icon_; }
    void set_command(std::shared_ptr<Command> command) { command_ = std::move(command); }
    [[nodiscard]] const std::shared_ptr<Command>& command() const noexcept { return command_; }
    void set_on_click(std::function<void()> callback) { on_click_ = std::move(callback); }
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept;

    [[nodiscard]] bool focusable() const noexcept override { return enabled(); }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return enabled(); }
    void on_activate() override;
    bool on_key_down(int key, KeyModifiers modifiers) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    [[nodiscard]] std::wstring EffectiveText() const;
    [[nodiscard]] bool EffectiveEnabled() const noexcept;

    std::wstring text_;
    ButtonStyle style_;
    std::shared_ptr<Image> icon_;
    std::shared_ptr<Command> command_;
    bool enabled_{true};
    std::function<void()> on_click_;
};

}  // namespace cppui
