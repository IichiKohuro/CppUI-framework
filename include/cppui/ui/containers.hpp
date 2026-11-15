#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cppui/ui/widget.hpp>

namespace cppui {

enum class PanelStyle {
    none,
    surface,
    soft,
    card,
};

// Basic surface with a single content child filling its padded area.
// Multiple children are arranged on top of each other, which is useful for
// overlays; use StackPanel/GridPanel for normal composition.
class Panel : public Widget {
public:
    explicit Panel(PanelStyle style = PanelStyle::surface, float corner_radius = 10.0f);

    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_style(PanelStyle style) noexcept { style_ = style; }
    void set_corner_radius(float radius) noexcept { corner_radius_ = radius; }
    void set_border(bool enabled) noexcept { border_ = enabled; }
    void set_clip_children(bool enabled) noexcept { clip_children_ = enabled; }
    void set_preferred_height(float height) noexcept { preferred_height_ = height; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

    [[nodiscard]] bool accepts_mouse() const noexcept override { return style_ != PanelStyle::none; }

protected:
    [[nodiscard]] Rect content_rect() const noexcept { return content_rect_; }
    [[nodiscard]] const Insets& padding_value() const noexcept { return padding_; }
    [[nodiscard]] PanelStyle style_value() const noexcept { return style_; }
    [[nodiscard]] float corner_radius_value() const noexcept { return corner_radius_; }
    [[nodiscard]] bool border_value() const noexcept { return border_; }

private:
    PanelStyle style_;
    float corner_radius_{};
    float preferred_height_{};
    Insets padding_{16.0f, 16.0f, 16.0f, 16.0f};
    bool border_{true};
    bool clip_children_{false};
    Rect content_rect_{};
};

class GroupPanel final : public Panel {
public:
    explicit GroupPanel(std::wstring title,
                        PanelStyle style = PanelStyle::surface,
                        float corner_radius = 10.0f);

    void set_title(std::wstring title) { title_ = std::move(title); }
    [[nodiscard]] const std::wstring& title() const noexcept { return title_; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::wstring title_;
    float header_height_{};
};

// Lightweight form grid. Column/row sizes are weights; row measure is content
// driven unless there is spare height and stretch_rows is enabled.
class GridPanel final : public Widget {
public:
    GridPanel() = default;

    void set_columns(std::vector<float> weights);
    void set_rows(std::vector<float> weights);
    void set_gap(float horizontal, float vertical) noexcept;
    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_stretch_rows(bool enabled) noexcept { stretch_rows_ = enabled; }

    void add_cell(std::unique_ptr<Widget> child,
                  int column,
                  int row,
                  int column_span = 1,
                  int row_span = 1);

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    struct Cell {
        int column{};
        int row{};
        int column_span{1};
        int row_span{1};
    };

    Insets padding_{};
    std::vector<float> column_weights_{1.0f};
    std::vector<float> row_weights_{};
    std::vector<Cell> cells_;
    std::vector<float> column_widths_;
    std::vector<float> row_heights_;
    float gap_x_{12.0f};
    float gap_y_{10.0f};
    bool stretch_rows_{false};
};

class ScrollBar final : public Widget {
public:
    explicit ScrollBar(Orientation orientation = Orientation::vertical);

    void set_range(float content_size, float viewport_size);
    void set_value(float value);
    [[nodiscard]] float value() const noexcept { return value_; }
    [[nodiscard]] float maximum() const noexcept;
    void set_on_scroll(std::function<void(float)> callback) { on_scroll_ = std::move(callback); }

    [[nodiscard]] bool accepts_mouse() const noexcept override { return true; }
    [[nodiscard]] bool activate_on_click() const noexcept override { return false; }
    void on_mouse_down(Point point) override;
    void on_mouse_up(Point point) override;
    void on_mouse_move(Point point) override;
    void on_mouse_leave() override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    [[nodiscard]] Rect thumb_rect() const noexcept;
    void SetValueInternal(float value, bool notify);

    Orientation orientation_;
    float content_size_{};
    float viewport_size_{};
    float value_{};
    bool dragging_{false};
    float drag_offset_{};
    std::function<void(float)> on_scroll_;
};

class ScrollViewer final : public Widget {
public:
    explicit ScrollViewer(std::unique_ptr<Widget> content);

    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_background(PanelStyle style) noexcept { background_ = style; }
    void scroll_by(float delta);
    void scroll_to(float offset);
    [[nodiscard]] float scroll_offset() const noexcept { return offset_; }
    [[nodiscard]] float maximum_scroll() const noexcept;

    [[nodiscard]] Widget* content() const noexcept { return content_; }
    [[nodiscard]] bool focusable() const noexcept override;
    [[nodiscard]] bool accepts_mouse() const noexcept override { return scrollable_; }

    void on_mouse_down(Point point) override { (void)point; }
    [[nodiscard]] bool on_mouse_wheel(float delta) override;
    [[nodiscard]] Widget* hit_test(Point point) const override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    void ApplyScroll();
    void UpdateBar();

    Widget* content_{};
    ScrollBar* bar_{};
    Insets padding_{};
    PanelStyle background_{PanelStyle::none};
    Rect viewport_{};
    float content_height_{};
    float offset_{};
    bool scrollable_{false};
};

}  // namespace cppui
