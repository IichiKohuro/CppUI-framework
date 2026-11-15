#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cppui/ui/widget.hpp>

namespace cppui {

class Painter;
class Command;

class PopupMenu final : public Widget {
public:
    explicit PopupMenu(float minimum_width = 220.0f);

    void add_item(std::wstring text,
                  std::function<void()> action = {},
                  std::wstring shortcut = {},
                  bool enabled = true,
                  bool checked = false);
    void add_command(std::shared_ptr<Command> command);
    void add_separator();
    void clear();
    void set_checked(int index, bool checked);
    void set_on_dismiss(std::function<void()> callback) { on_dismiss_ = std::move(callback); }

    [[nodiscard]] std::size_t item_count() const noexcept { return items_.size(); }
    [[nodiscard]] int selected_index() const noexcept { return selected_; }
    void set_selected_index(int index);

    [[nodiscard]] bool focusable() const noexcept override { return true; }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return true; }
    [[nodiscard]] bool activate_on_click() const noexcept override { return false; }
    void on_focus_changed(bool focused) override;
    void on_mouse_down(Point point) override;
    void on_mouse_up(Point point) override;
    void on_mouse_move(Point point) override;
    bool on_key_down(int key, KeyModifiers modifiers) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    struct Item {
        std::wstring text;
        std::wstring shortcut;
        std::function<void()> action;
        bool separator{false};
        bool enabled{true};
        bool checked{false};
    };

    [[nodiscard]] int IndexAt(Point point) const noexcept;
    void MoveSelection(int delta);
    void Activate(int index);
    [[nodiscard]] float RowHeight(Painter& painter) const;

    std::vector<Item> items_;
    std::vector<Rect> row_rects_;
    float minimum_width_{220.0f};
    float row_height_{32.0f};
    int selected_{-1};
    std::function<void()> on_dismiss_;
};

class ToolTip final : public Widget {
public:
    explicit ToolTip(std::wstring text = {});

    void set_text(std::wstring text) { text_ = std::move(text); }
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::wstring text_;
};

// Hosts normal content plus a modal popup and a passive tooltip. A popup
// swallows clicks outside its rectangle, which is how click-outside dismissal
// is implemented without a second HWND.
class PopupHost final : public Widget {
public:
    explicit PopupHost(std::unique_ptr<Widget> content);

    [[nodiscard]] Widget* content() const noexcept { return content_; }
    bool open(std::unique_ptr<Widget> popup, Point position);
    void close();
    [[nodiscard]] bool is_open() const noexcept { return popup_ != nullptr; }
    [[nodiscard]] Widget* popup() const noexcept { return popup_; }

    void show_tooltip(std::wstring text, Point position);
    void hide_tooltip();
    [[nodiscard]] bool tooltip_visible() const noexcept { return tooltip_ != nullptr; }

    void set_focus_bridge(std::function<Widget*()> save_focus,
                          std::function<void(Widget*)> restore_focus);
    void set_on_layout_requested(std::function<void()> callback) {
        on_layout_requested_ = std::move(callback);
    }

    [[nodiscard]] bool accepts_mouse() const noexcept override { return popup_ != nullptr; }
    [[nodiscard]] bool focus_trap() const noexcept override { return popup_ != nullptr; }
    void on_mouse_down(Point point) override;
    [[nodiscard]] Widget* hit_test(Point point) const override;
    void collect_focusable(std::vector<Widget*>& output) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    void RequestLayout();

    Widget* content_{};
    Widget* popup_{};
    Widget* tooltip_{};
    std::vector<std::unique_ptr<Widget>> retired_;
    Point popup_position_{};
    Point tooltip_position_{};
    Size popup_size_{};
    Size tooltip_size_{};
    std::function<Widget*()> save_focus_;
    std::function<void(Widget*)> restore_focus_;
    std::function<void()> on_layout_requested_;
    Widget* previous_focus_{};
};

}  // namespace cppui
