#include <cppui/ui/input.hpp>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <utility>

#include <cppui/ui/clipboard.hpp>
#include <cppui/ui/painter.hpp>

namespace cppui {
namespace {

constexpr int kKeyBack = 0x08;
constexpr int kKeyTab = 0x09;
constexpr int kKeyEnter = 0x0D;
constexpr int kKeyEscape = 0x1B;
constexpr int kKeyEnd = 0x23;
constexpr int kKeyHome = 0x24;
constexpr int kKeyLeft = 0x25;
constexpr int kKeyUp = 0x26;
constexpr int kKeyRight = 0x27;
constexpr int kKeyDown = 0x28;
constexpr int kKeyDelete = 0x2E;
constexpr int kKeyA = 0x41;
constexpr int kKeyC = 0x43;
constexpr int kKeyV = 0x56;
constexpr int kKeyX = 0x58;
constexpr int kKeyY = 0x59;
constexpr int kKeyZ = 0x5A;

bool IsWordCharacter(wchar_t character) noexcept {
    return std::iswalnum(static_cast<wint_t>(character)) != 0 || character == L'_';
}

float ClampCaretX(float value, const Rect& area) noexcept {
    return std::clamp(value, area.x, area.right());
}

}  // namespace

TextEdit::TextEdit(std::wstring text, std::wstring placeholder)
    : text_(std::move(text)), placeholder_(std::move(placeholder)), caret_(text_.size()),
      anchor_(text_.size()) {}

void TextEdit::set_text(std::wstring text) {
    if (text_ == text) {
        return;
    }
    text_ = std::move(text);
    caret_ = text_.size();
    anchor_ = caret_;
    undo_.clear();
    redo_.clear();
    scroll_x_ = 0.0f;
    NotifyChanged();
}

void TextEdit::select_all() {
    anchor_ = 0;
    caret_ = text_.size();
}

void TextEdit::set_caret(std::size_t index, bool extend_selection) {
    const std::size_t clamped = std::min(index, text_.size());
    if (!extend_selection) {
        anchor_ = clamped;
    }
    caret_ = clamped;
}

std::size_t TextEdit::selection_begin() const noexcept {
    return std::min(caret_, anchor_);
}

std::size_t TextEdit::selection_end() const noexcept {
    return std::max(caret_, anchor_);
}

std::wstring TextEdit::selected_text() const {
    const std::size_t begin = selection_begin();
    const std::size_t end = selection_end();
    if (begin == end) {
        return {};
    }
    return text_.substr(begin, end - begin);
}

void TextEdit::PushUndo() {
    undo_.push_back(Snapshot{text_, caret_, anchor_});
    if (undo_.size() > 100) {
        undo_.erase(undo_.begin());
    }
    redo_.clear();
}

void TextEdit::Undo() {
    if (undo_.empty()) {
        return;
    }
    redo_.push_back(Snapshot{text_, caret_, anchor_});
    Snapshot snapshot = std::move(undo_.back());
    undo_.pop_back();
    text_ = std::move(snapshot.text);
    caret_ = std::min(snapshot.caret, text_.size());
    anchor_ = std::min(snapshot.anchor, text_.size());
    scroll_x_ = 0.0f;
    NotifyChanged();
}

void TextEdit::Redo() {
    if (redo_.empty()) {
        return;
    }
    undo_.push_back(Snapshot{text_, caret_, anchor_});
    Snapshot snapshot = std::move(redo_.back());
    redo_.pop_back();
    text_ = std::move(snapshot.text);
    caret_ = std::min(snapshot.caret, text_.size());
    anchor_ = std::min(snapshot.anchor, text_.size());
    scroll_x_ = 0.0f;
    NotifyChanged();
}

void TextEdit::DeleteSelection() {
    const std::size_t begin = selection_begin();
    const std::size_t end = selection_end();
    if (begin == end) {
        return;
    }
    text_.erase(begin, end - begin);
    caret_ = begin;
    anchor_ = begin;
}

void TextEdit::InsertText(std::wstring_view text) {
    if (read_only_ || text.empty()) {
        return;
    }

    std::wstring filtered;
    filtered.reserve(text.size());
    for (wchar_t character : text) {
        if (character >= L' ') {
            filtered.push_back(character);
        }
    }
    if (filtered.empty()) {
        return;
    }

    const bool has_selection = selection_begin() != selection_end();
    if (max_length_ > 0) {
        const std::size_t selected = has_selection ? selection_end() - selection_begin() : 0;
        const std::size_t available = max_length_ - std::min(max_length_, text_.size() - selected);
        if (available == 0) {
            return;
        }
        if (filtered.size() > available) {
            filtered.resize(available);
        }
    }

    PushUndo();
    DeleteSelection();
    text_.insert(caret_, filtered);
    caret_ += filtered.size();
    anchor_ = caret_;
    NotifyChanged();
}

void TextEdit::NotifyChanged() {
    ClampSelection();
    if (on_text_changed_) {
        on_text_changed_(text_);
    }
}

void TextEdit::ClampSelection() {
    caret_ = std::min(caret_, text_.size());
    anchor_ = std::min(anchor_, text_.size());
}

void TextEdit::MoveCaretTo(std::size_t index, bool extend) {
    const std::size_t clamped = std::min(index, text_.size());
    if (!extend) {
        anchor_ = clamped;
    }
    caret_ = clamped;
}

void TextEdit::MoveCaret(int delta, bool extend) {
    if (delta < 0) {
        if (!extend && selection_begin() != selection_end()) {
            MoveCaretTo(selection_begin(), false);
            return;
        }
        const std::size_t step = static_cast<std::size_t>(-delta);
        MoveCaretTo(caret_ > step ? caret_ - step : 0, extend);
    } else {
        if (!extend && selection_begin() != selection_end()) {
            MoveCaretTo(selection_end(), false);
            return;
        }
        MoveCaretTo(caret_ + static_cast<std::size_t>(delta), extend);
    }
}

void TextEdit::MoveWord(int direction, bool extend) {
    std::size_t index = caret_;
    if (direction < 0) {
        while (index > 0 && !IsWordCharacter(text_[index - 1])) {
            --index;
        }
        while (index > 0 && IsWordCharacter(text_[index - 1])) {
            --index;
        }
    } else {
        while (index < text_.size() && IsWordCharacter(text_[index])) {
            ++index;
        }
        while (index < text_.size() && !IsWordCharacter(text_[index])) {
            ++index;
        }
    }
    MoveCaretTo(index, extend);
}

void TextEdit::on_focus_changed(bool focused) {
    if (focused && text_.size() > 0 && selection_begin() == selection_end() &&
        caret_ == text_.size()) {
        select_all();
    }
}

void TextEdit::on_mouse_down(Point point) {
    const std::size_t index = IndexFromPoint(point);
    anchor_ = index;
    caret_ = index;
    selecting_ = true;
}

void TextEdit::on_mouse_up(Point point) {
    if (selecting_) {
        caret_ = IndexFromPoint(point);
    }
    selecting_ = false;
}

void TextEdit::on_mouse_move(Point point) {
    if (selecting_) {
        caret_ = IndexFromPoint(point);
    }
}

void TextEdit::on_mouse_leave() {
    selecting_ = false;
}

bool TextEdit::on_key_down(int key, KeyModifiers modifiers) {
    if (key == kKeyTab || key == kKeyEscape || key == kKeyUp || key == kKeyDown) {
        return false;
    }
    if (key == kKeyEnter) {
        if (on_enter_) {
            on_enter_(text_);
        }
        return true;
    }

    if (modifiers.control) {
        switch (key) {
            case kKeyA:
                select_all();
                return true;
            case kKeyC:
                if (selection_begin() != selection_end()) {
                    (void)WriteClipboardText(selected_text());
                }
                return true;
            case kKeyX: {
                if (read_only_) {
                    return true;
                }
                const std::wstring selected = selected_text();
                if (!selected.empty()) {
                    (void)WriteClipboardText(selected);
                    PushUndo();
                    DeleteSelection();
                    NotifyChanged();
                }
                return true;
            }
            case kKeyV:
                if (!read_only_) {
                    InsertText(ReadClipboardText());
                }
                return true;
            case kKeyZ:
                if (!read_only_) {
                    Undo();
                }
                return true;
            case kKeyY:
                if (!read_only_) {
                    Redo();
                }
                return true;
            default:
                break;
        }
    }

    if (read_only_) {
        return false;
    }

    switch (key) {
        case kKeyBack:
            if (selection_begin() != selection_end()) {
                PushUndo();
                DeleteSelection();
                NotifyChanged();
            } else if (caret_ > 0) {
                PushUndo();
                if (modifiers.control) {
                    MoveWord(-1, false);
                }
                const std::size_t start = modifiers.control ? caret_ : caret_ - 1;
                text_.erase(start, caret_ - start);
                caret_ = start;
                anchor_ = start;
                NotifyChanged();
            }
            return true;
        case kKeyDelete:
            if (selection_begin() != selection_end()) {
                PushUndo();
                DeleteSelection();
                NotifyChanged();
            } else if (caret_ < text_.size()) {
                PushUndo();
                if (modifiers.control) {
                    const std::size_t original = caret_;
                    MoveWord(1, false);
                    const std::size_t end = caret_;
                    caret_ = original;
                    anchor_ = original;
                    text_.erase(original, end - original);
                } else {
                    text_.erase(caret_, 1);
                }
                anchor_ = caret_;
                NotifyChanged();
            }
            return true;
        case kKeyHome:
            MoveCaretTo(0, modifiers.shift);
            return true;
        case kKeyEnd:
            MoveCaretTo(text_.size(), modifiers.shift);
            return true;
        case kKeyLeft:
            if (modifiers.control) {
                MoveWord(-1, modifiers.shift);
            } else {
                MoveCaret(-1, modifiers.shift);
            }
            return true;
        case kKeyRight:
            if (modifiers.control) {
                MoveWord(1, modifiers.shift);
            } else {
                MoveCaret(1, modifiers.shift);
            }
            return true;
        default:
            break;
    }
    return false;
}

bool TextEdit::on_char(wchar_t character) {
    if (read_only_ || character < L' ' || character == 0x7F) {
        return false;
    }
    InsertText(std::wstring_view(&character, 1));
    return true;
}

Size TextEdit::measure(Painter& painter, const Size& available) {
    const DensityMetrics metrics = density_metrics(painter.density());
    text_padding_ = std::max(8.0f, metrics.control_padding * 0.85f);
    const float preferred_width =
        available.width > 0.0f ? std::max(metrics.min_control_width, available.width) : 240.0f;
    const Size text_size = painter.measure_text(text_.empty() ? placeholder_ : text_, FontRole::body);
    text_height_ = text_size.height;
    text_width_ = painter.measure_text(text_, FontRole::body).width;
    average_character_width_ =
        text_.empty() ? 8.0f : std::max(4.0f, text_width_ / static_cast<float>(text_.size()));
    const Size desired{preferred_width, metrics.control_height};
    set_desired_size(desired);
    return desired;
}

void TextEdit::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

Rect TextEdit::TextArea() const noexcept {
    return Rect{bounds().x + text_padding_,
                bounds().y + 2.0f,
                std::max(0.0f, bounds().width - text_padding_ * 2.0f),
                std::max(0.0f, bounds().height - 4.0f)};
}

float TextEdit::CaretX(Painter& painter, std::size_t index) const {
    const std::size_t clamped = std::min(index, text_.size());
    const Size prefix = painter.measure_text(std::wstring_view(text_).substr(0, clamped),
                                             FontRole::body);
    return TextArea().x + prefix.width;
}

void TextEdit::EnsureCaretVisible(Painter& painter) const {
    text_width_ = painter.measure_text(text_, FontRole::body).width;
    const Rect area = TextArea();
    if (area.width <= 2.0f) {
        scroll_x_ = 0.0f;
        return;
    }
    const float caret_x = CaretX(painter, caret_) - area.x;
    if (caret_x - scroll_x_ > area.width - 2.0f) {
        scroll_x_ = caret_x - area.width + 2.0f;
    }
    if (caret_x - scroll_x_ < 0.0f) {
        scroll_x_ = caret_x;
    }
    scroll_x_ = std::max(0.0f, scroll_x_);
}

std::size_t TextEdit::IndexFromPoint(Point point) const {
    const Rect area = TextArea();
    const float x = point.x - area.x + scroll_x_;
    if (x <= 0.0f || text_.empty()) {
        return 0;
    }
    const float width =
        text_width_ > 0.0f ? text_width_ / static_cast<float>(text_.size())
                           : average_character_width_;
    const float step = std::max(3.0f, width);
    const std::size_t index =
        static_cast<std::size_t>(std::max(0.0f, std::round(x / step)));
    return std::min(index, text_.size());
}

Rect TextEdit::ime_caret_rect() const noexcept {
    if (bounds().width <= 1.0f || bounds().height <= 1.0f) {
        return {};
    }
    const Rect area = TextArea();
    const float x = painted_caret_x_ > 0.0f ? painted_caret_x_ : area.x;
    return Rect{x, area.y + 2.0f, 2.0f, std::max(1.0f, area.height - 4.0f)};
}

void TextEdit::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const DensityMetrics metrics = density_metrics(painter.density());
    EnsureCaretVisible(painter);

    const Color border = has_validation_error()
                             ? tokens.danger
                             : (focused() ? tokens.accent : tokens.border);
    painter.fill_rounded_rect(bounds(), metrics.control_radius, tokens.surface);
    painter.stroke_rounded_rect(bounds(), metrics.control_radius, border,
                                has_validation_error() || focused() ? 1.5f : 1.0f);

    const Rect area = TextArea();
    painter.push_clip(area);

    if (text_.empty()) {
        painter.draw_text(placeholder_,
                          area,
                          FontRole::body,
                          tokens.text_tertiary,
                          TextAlign::leading,
                          TextVAlign::center);
    } else {
        const std::size_t begin = selection_begin();
        const std::size_t end = selection_end();
        if (focused() && begin != end) {
            const float left = ClampCaretX(CaretX(painter, begin) - scroll_x_, area);
            const float right = ClampCaretX(CaretX(painter, end) - scroll_x_, area);
            painter.fill_rect(Rect{left, area.y + 1.0f, std::max(0.0f, right - left),
                                   std::max(0.0f, area.height - 2.0f)},
                              tokens.accent_soft);
        }

        const Rect text_rect{area.x - scroll_x_,
                             area.y,
                             std::max(area.width, text_width_ + 8.0f),
                             area.height};
        painter.draw_text(text_,
                          text_rect,
                          FontRole::body,
                          tokens.text_primary,
                          TextAlign::leading,
                          TextVAlign::center);
    }

    if (focused()) {
        const float caret_x =
            ClampCaretX(CaretX(painter, caret_) - scroll_x_, area);
        painted_caret_x_ = caret_x;
        painter.draw_line(Point{caret_x, area.y + 2.0f},
                          Point{caret_x, area.bottom() - 2.0f},
                          tokens.accent,
                          1.4f);
    }

    painter.pop_clip();
}

CheckBox::CheckBox(std::wstring text, bool checked)
    : text_(std::move(text)), state_(checked ? CheckState::checked : CheckState::unchecked) {}

void CheckBox::set_checked(bool checked) {
    const CheckState next = checked ? CheckState::checked : CheckState::unchecked;
    if (state_ == next) {
        return;
    }
    state_ = next;
    if (on_changed_) {
        on_changed_(state_);
    }
}

void CheckBox::Toggle() {
    if (!enabled_) {
        return;
    }
    if (tristate_) {
        switch (state_) {
            case CheckState::unchecked:
                state_ = CheckState::checked;
                break;
            case CheckState::checked:
                state_ = CheckState::indeterminate;
                break;
            case CheckState::indeterminate:
                state_ = CheckState::unchecked;
                break;
        }
    } else {
        state_ = state_ == CheckState::checked ? CheckState::unchecked : CheckState::checked;
    }
    if (on_changed_) {
        on_changed_(state_);
    }
}

void CheckBox::on_activate() {
    Toggle();
}

bool CheckBox::on_key_down(int key, KeyModifiers modifiers) {
    if (!enabled_) {
        return false;
    }
    if (key == 0x20 || key == kKeyEnter) {
        (void)modifiers;
        Toggle();
        return true;
    }
    return false;
}

Size CheckBox::measure(Painter& painter, const Size& available) {
    (void)available;
    const DensityMetrics metrics = density_metrics(painter.density());
    const Size text_size = painter.measure_text(text_, FontRole::body);
    const float box = 18.0f;
    const Size desired{box + 10.0f + text_size.width,
                       std::max(24.0f, std::max(metrics.control_height, text_size.height + 4.0f))};
    set_desired_size(desired);
    return desired;
}

void CheckBox::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void CheckBox::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const float box = 18.0f;
    const Rect box_rect{bounds().x, bounds().y + (bounds().height - box) * 0.5f, box, box};
    const Color text_color = enabled_ ? tokens.text_primary : tokens.text_tertiary;

    if (focused() && enabled_) {
        painter.stroke_rounded_rect(
            Rect{box_rect.x - 2.0f, box_rect.y - 2.0f, box_rect.width + 4.0f, box_rect.height + 4.0f},
            5.0f,
            tokens.accent,
            1.2f);
    }

    if (state_ == CheckState::checked) {
        const Color fill = enabled_ ? tokens.accent : tokens.border_strong;
        painter.fill_rounded_rect(box_rect, 4.0f, fill);
        painter.draw_line(Point{box_rect.x + 4.0f, box_rect.y + 9.0f},
                          Point{box_rect.x + 7.5f, box_rect.y + 12.5f},
                          enabled_ ? tokens.on_accent : tokens.surface,
                          1.8f);
        painter.draw_line(Point{box_rect.x + 7.5f, box_rect.y + 12.5f},
                          Point{box_rect.right() - 4.0f, box_rect.y + 5.0f},
                          enabled_ ? tokens.on_accent : tokens.surface,
                          1.8f);
    } else {
        painter.fill_rounded_rect(box_rect, 4.0f, tokens.surface);
        painter.stroke_rounded_rect(box_rect, 4.0f,
                                    hovered() && enabled_ ? tokens.accent : tokens.border_strong,
                                    1.2f);
        if (state_ == CheckState::indeterminate) {
            painter.draw_line(Point{box_rect.x + 4.0f, box_rect.y + 9.0f},
                              Point{box_rect.right() - 4.0f, box_rect.y + 9.0f},
                              enabled_ ? tokens.accent : tokens.text_tertiary,
                              1.8f);
        }
    }

    const Rect text_rect{box_rect.right() + 10.0f,
                         bounds().y,
                         std::max(0.0f, bounds().width - box - 10.0f),
                         bounds().height};
    painter.draw_text(text_,
                      text_rect,
                      FontRole::body,
                      text_color,
                      TextAlign::leading,
                      TextVAlign::center);
}

RadioButton::RadioButton(std::wstring text, bool checked)
    : text_(std::move(text)), checked_(checked) {}

void RadioButton::ApplyChecked(bool checked, bool notify) {
    if (checked_ == checked) {
        return;
    }
    checked_ = checked;
    if (notify && on_changed_) {
        on_changed_(checked_);
    }
}

void RadioButton::set_checked(bool checked) {
    if (checked == checked_) {
        return;
    }
    if (checked && group_notifier_) {
        group_notifier_(*this);
        return;
    }
    ApplyChecked(checked, true);
}

void RadioButton::on_activate() {
    if (!enabled_ || checked_) {
        return;
    }
    set_checked(true);
}

bool RadioButton::on_key_down(int key, KeyModifiers modifiers) {
    (void)modifiers;
    if (!enabled_) {
        return false;
    }
    if (key == 0x20 || key == kKeyEnter) {
        on_activate();
        return true;
    }
    if (key == kKeyLeft || key == kKeyUp || key == kKeyRight || key == kKeyDown) {
        auto* group = dynamic_cast<RadioGroup*>(parent());
        if (group != nullptr) {
            const int delta = (key == kKeyLeft || key == kKeyUp) ? -1 : 1;
            group->select_adjacent(*this, delta);
            return true;
        }
    }
    return false;
}

Size RadioButton::measure(Painter& painter, const Size& available) {
    (void)available;
    const DensityMetrics metrics = density_metrics(painter.density());
    const Size text_size = painter.measure_text(text_, FontRole::body);
    const float diameter = 18.0f;
    const Size desired{diameter + 10.0f + text_size.width,
                       std::max(24.0f, std::max(metrics.control_height, text_size.height + 4.0f))};
    set_desired_size(desired);
    return desired;
}

void RadioButton::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void RadioButton::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    const float diameter = 18.0f;
    const Rect circle{bounds().x,
                      bounds().y + (bounds().height - diameter) * 0.5f,
                      diameter,
                      diameter};
    const Color text_color = enabled_ ? tokens.text_primary : tokens.text_tertiary;

    if (focused() && enabled_) {
        painter.stroke_rounded_rect(
            Rect{circle.x - 2.0f, circle.y - 2.0f, circle.width + 4.0f, circle.height + 4.0f},
            10.0f,
            tokens.accent,
            1.2f);
    }

    painter.fill_rounded_rect(circle, 9.0f, tokens.surface);
    painter.stroke_rounded_rect(circle, 9.0f,
                                checked_ ? tokens.accent
                                         : (hovered() && enabled_ ? tokens.accent
                                                                  : tokens.border_strong),
                                checked_ ? 1.8f : 1.2f);
    if (checked_) {
        const float inset = 5.0f;
        painter.fill_rounded_rect(Rect{circle.x + inset,
                                       circle.y + inset,
                                       circle.width - inset * 2.0f,
                                       circle.height - inset * 2.0f},
                                  4.0f,
                                  enabled_ ? tokens.accent : tokens.border_strong);
    }

    const Rect text_rect{circle.right() + 10.0f,
                         bounds().y,
                         std::max(0.0f, bounds().width - diameter - 10.0f),
                         bounds().height};
    painter.draw_text(text_,
                      text_rect,
                      FontRole::body,
                      text_color,
                      TextAlign::leading,
                      TextVAlign::center);
}

RadioGroup::RadioGroup(Orientation orientation, float spacing)
    : orientation_(orientation), spacing_(spacing) {}

void RadioGroup::on_child_added(Widget& child) {
    auto* radio = dynamic_cast<RadioButton*>(&child);
    if (radio == nullptr) {
        return;
    }
    radio->set_group_notifier([this](RadioButton& source) { select(&source); });
    if (radio->checked()) {
        if (selected_ == nullptr) {
            selected_ = radio;
        } else {
            radio->ApplyChecked(false, false);
        }
    }
}

void RadioGroup::select(RadioButton* button) {
    if (button == nullptr || !button->enabled_) {
        return;
    }
    selected_ = button;
    for (RadioButton* candidate : Buttons()) {
        candidate->ApplyChecked(candidate == button, true);
    }
}

std::vector<RadioButton*> RadioGroup::Buttons() const {
    std::vector<RadioButton*> buttons;
    for (const auto& child : children()) {
        if (auto* radio = dynamic_cast<RadioButton*>(child.get())) {
            buttons.push_back(radio);
        }
    }
    return buttons;
}

void RadioGroup::select_adjacent(RadioButton& current, int delta) {
    const std::vector<RadioButton*> buttons = Buttons();
    if (buttons.empty()) {
        return;
    }
    auto position = std::find(buttons.begin(), buttons.end(), &current);
    if (position == buttons.end()) {
        return;
    }
    int index = static_cast<int>(position - buttons.begin());
    const int count = static_cast<int>(buttons.size());
    for (int step = 0; step < count; ++step) {
        index = (index + delta + count) % count;
        if (buttons[static_cast<std::size_t>(index)]->enabled()) {
            select(buttons[static_cast<std::size_t>(index)]);
            return;
        }
    }
}

Size RadioGroup::measure(Painter& painter, const Size& available) {
    child_sizes_.clear();
    const float inner_width = std::max(0.0f, available.width - padding_.left - padding_.right);
    const float inner_height = std::max(0.0f, available.height - padding_.top - padding_.bottom);
    float main = 0.0f;
    float cross = 0.0f;
    for (const auto& child : children()) {
        const Size size = child->visible() ? child->measure(painter, Size{inner_width, inner_height})
                                           : Size{};
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

void RadioGroup::arrange(const Rect& bounds) {
    set_bounds(bounds);
    const Rect inner = bounds.inset(padding_);
    float cursor = orientation_ == Orientation::vertical ? inner.y : inner.x;
    for (std::size_t index = 0; index < children().size() && index < child_sizes_.size(); ++index) {
        Widget* child = children()[index].get();
        if (!child->visible()) {
            continue;
        }
        const Size size = child_sizes_[index];
        if (orientation_ == Orientation::vertical) {
            child->arrange(Rect{inner.x, cursor, std::max(0.0f, inner.width), size.height});
            cursor += size.height + spacing_;
        } else {
            child->arrange(Rect{cursor, inner.y, size.width, std::max(0.0f, inner.height)});
            cursor += size.width + spacing_;
        }
    }
}

void RadioGroup::paint(Painter& painter) const {
    paint_children(painter);
}

}  // namespace cppui
