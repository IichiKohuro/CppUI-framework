#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <cppui/ui/widget.hpp>

namespace cppui {

class Painter;
class RadioGroup;
struct DensityMetrics;

// Single-line editor with selection, clipboard, undo/redo and validation.
// IME text arrives through the host's WM_CHAR/WM_IME_CHAR bridge.
class TextEdit final : public Widget {
public:
    explicit TextEdit(std::wstring text = {}, std::wstring placeholder = {});

    void set_text(std::wstring text);
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }
    void set_placeholder(std::wstring placeholder) {
        placeholder_ = std::move(placeholder);
        set_accessible_name(placeholder_);
    }
    [[nodiscard]] const std::wstring& placeholder() const noexcept { return placeholder_; }
    void set_read_only(bool read_only) noexcept { read_only_ = read_only; }
    [[nodiscard]] bool read_only() const noexcept { return read_only_; }
    void set_max_length(std::size_t length) noexcept { max_length_ = length; }
    void set_on_text_changed(std::function<void(const std::wstring&)> callback) {
        on_text_changed_ = std::move(callback);
    }
    void set_on_enter(std::function<void(const std::wstring&)> callback) {
        on_enter_ = std::move(callback);
    }

    void select_all();
    void set_caret(std::size_t index, bool extend_selection = false);
    [[nodiscard]] std::size_t caret() const noexcept { return caret_; }
    [[nodiscard]] std::size_t selection_begin() const noexcept;
    [[nodiscard]] std::size_t selection_end() const noexcept;
    [[nodiscard]] std::wstring selected_text() const;

    [[nodiscard]] bool focusable() const noexcept override { return true; }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return true; }
    [[nodiscard]] bool activate_on_click() const noexcept override { return false; }
    void on_focus_changed(bool focused) override;
    void on_mouse_down(Point point) override;
    void on_mouse_up(Point point) override;
    void on_mouse_move(Point point) override;
    void on_mouse_leave() override;
    bool on_key_down(int key, KeyModifiers modifiers) override;
    bool on_char(wchar_t character) override;
    [[nodiscard]] Rect ime_caret_rect() const noexcept override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    struct Snapshot {
        std::wstring text;
        std::size_t caret{};
        std::size_t anchor{};
    };

    void PushUndo();
    void Undo();
    void Redo();
    void DeleteSelection();
    void InsertText(std::wstring_view text);
    void NotifyChanged();
    void ClampSelection();
    void EnsureCaretVisible(Painter& painter) const;
    void MoveCaret(int delta, bool extend);
    void MoveCaretTo(std::size_t index, bool extend);
    void MoveWord(int direction, bool extend);
    [[nodiscard]] std::size_t IndexFromPoint(Point point) const;
    [[nodiscard]] float CaretX(Painter& painter, std::size_t index) const;
    [[nodiscard]] Rect TextArea() const noexcept;

    std::wstring text_;
    std::wstring placeholder_;
    std::size_t caret_{};
    std::size_t anchor_{};
    std::size_t max_length_{};
    bool read_only_{false};
    bool selecting_{false};
    mutable float scroll_x_{};
    mutable float painted_caret_x_{};
    mutable float text_width_{};
    float text_height_{};
    float text_padding_{12.0f};
    float average_character_width_{8.0f};
    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;
    std::function<void(const std::wstring&)> on_text_changed_;
    std::function<void(const std::wstring&)> on_enter_;
};

enum class CheckState {
    unchecked,
    checked,
    indeterminate,
};

class CheckBox final : public Widget {
public:
    explicit CheckBox(std::wstring text = {}, bool checked = false);

    void set_text(std::wstring text) {
        text_ = std::move(text);
        set_accessible_name(text_);
    }
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }
    void set_checked(bool checked);
    [[nodiscard]] bool checked() const noexcept { return state_ == CheckState::checked; }
    void set_state(CheckState state) noexcept;
    [[nodiscard]] CheckState state() const noexcept { return state_; }
    void set_tristate(bool enabled) noexcept { tristate_ = enabled; }
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    void set_on_changed(std::function<void(CheckState)> callback) {
        on_changed_ = std::move(callback);
    }

    [[nodiscard]] bool focusable() const noexcept override { return enabled_; }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return enabled_; }
    void on_activate() override;
    bool on_key_down(int key, KeyModifiers modifiers) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    void Toggle();

    std::wstring text_;
    CheckState state_{CheckState::unchecked};
    bool tristate_{false};
    bool enabled_{true};
    std::function<void(CheckState)> on_changed_;
};

class RadioButton final : public Widget {
public:
    explicit RadioButton(std::wstring text = {}, bool checked = false);

    void set_text(std::wstring text) {
        text_ = std::move(text);
        set_accessible_name(text_);
    }
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }
    void set_checked(bool checked);
    [[nodiscard]] bool checked() const noexcept { return checked_; }
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    void set_on_changed(std::function<void(bool)> callback) {
        on_changed_ = std::move(callback);
    }

    [[nodiscard]] bool focusable() const noexcept override { return enabled_; }
    [[nodiscard]] bool accepts_mouse() const noexcept override { return enabled_; }
    void on_activate() override;
    bool on_key_down(int key, KeyModifiers modifiers) override;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    friend class RadioGroup;
    void ApplyChecked(bool checked, bool notify);
    void set_group_notifier(std::function<void(RadioButton&)> notifier) {
        group_notifier_ = std::move(notifier);
    }

    std::wstring text_;
    bool checked_{false};
    bool enabled_{true};
    std::function<void(bool)> on_changed_;
    std::function<void(RadioButton&)> group_notifier_;
};

// Exposes a single selection for a set of direct RadioButton children.
class RadioGroup final : public Widget {
public:
    explicit RadioGroup(Orientation orientation = Orientation::vertical, float spacing = 8.0f);

    void set_padding(const Insets& padding) noexcept { padding_ = padding; }
    void set_spacing(float spacing) noexcept { spacing_ = spacing; }
    void select(RadioButton* button);
    [[nodiscard]] RadioButton* selected() const noexcept { return selected_; }
    void select_adjacent(RadioButton& current, int delta);

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

protected:
    void on_child_added(Widget& child) override;

private:
    std::vector<RadioButton*> Buttons() const;

    Orientation orientation_;
    float spacing_{};
    Insets padding_{};
    RadioButton* selected_{};
    std::vector<Size> child_sizes_;
};

}  // namespace cppui
