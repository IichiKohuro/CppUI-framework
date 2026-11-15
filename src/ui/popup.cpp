#include <cppui/ui/popup.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

#include <cppui/ui/command.hpp>
#include <cppui/ui/painter.hpp>

namespace cppui {
namespace {

constexpr int kKeyEnter = 0x0D;
constexpr int kKeyEscape = 0x1B;
constexpr int kKeySpace = 0x20;
constexpr int kKeyEnd = 0x23;
constexpr int kKeyHome = 0x24;
constexpr int kKeyLeft = 0x25;
constexpr int kKeyUp = 0x26;
constexpr int kKeyRight = 0x27;
constexpr int kKeyDown = 0x28;
constexpr float kPopupPadding = 6.0f;
constexpr float kSeparatorHeight = 9.0f;

float ClampOrCenter(float value, float start, float size, float limit) {
    if (size >= limit) {
        return start;
    }
    return std::clamp(value, start, start + limit - size);
}

}  // namespace

PopupMenu::PopupMenu(float minimum_width) : minimum_width_(minimum_width) {}

void PopupMenu::add_item(std::wstring text,
                         std::function<void()> action,
                         std::wstring shortcut,
                         bool enabled,
                         bool checked) {
    Item item;
    item.text = std::move(text);
    item.action = std::move(action);
    item.shortcut = std::move(shortcut);
    item.enabled = enabled;
    item.checked = checked;
    items_.push_back(std::move(item));
    selected_ = -1;
}

void PopupMenu::add_command(std::shared_ptr<Command> command) {
    if (!command) {
        return;
    }
    add_item(command->text(),
             [command] { (void)command->Execute(); },
             command->shortcut(),
             command->enabled(),
             command->checked());
}

void PopupMenu::add_separator() {
    Item item;
    item.separator = true;
    item.enabled = false;
    items_.push_back(std::move(item));
}

void PopupMenu::clear() {
    items_.clear();
    row_rects_.clear();
    selected_ = -1;
}

void PopupMenu::set_checked(int index, bool checked) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_[static_cast<std::size_t>(index)].checked = checked;
    }
}

void PopupMenu::set_selected_index(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        selected_ = -1;
        return;
    }
    if (items_[static_cast<std::size_t>(index)].separator ||
        !items_[static_cast<std::size_t>(index)].enabled) {
        selected_ = -1;
        return;
    }
    selected_ = index;
}

float PopupMenu::RowHeight(Painter& painter) const {
    return std::max(28.0f, density_metrics(painter.density()).control_height);
}

Size PopupMenu::measure(Painter& painter, const Size& available) {
    row_height_ = RowHeight(painter);
    float width = minimum_width_;
    float height = kPopupPadding * 2.0f;
    for (const Item& item : items_) {
        if (item.separator) {
            height += kSeparatorHeight;
            continue;
        }
        const Size text_size = painter.measure_text(item.text, FontRole::body);
        const Size shortcut_size = painter.measure_text(item.shortcut, FontRole::caption);
        width = std::max(width, text_size.width + shortcut_size.width + 62.0f);
        height += row_height_;
    }
    if (available.width > 0.0f) {
        width = std::min(width, available.width);
    }
    if (available.height > 0.0f) {
        height = std::min(height, available.height);
    }
    const Size desired{width, height};
    set_desired_size(desired);
    return desired;
}

void PopupMenu::arrange(const Rect& bounds) {
    set_bounds(bounds);
    row_rects_.clear();
    float y = bounds.y + kPopupPadding;
    for (const Item& item : items_) {
        if (item.separator) {
            row_rects_.push_back(Rect{bounds.x + 6.0f, y, std::max(0.0f, bounds.width - 12.0f),
                                      kSeparatorHeight});
            y += kSeparatorHeight;
            continue;
        }
        row_rects_.push_back(
            Rect{bounds.x + kPopupPadding, y, std::max(0.0f, bounds.width - kPopupPadding * 2.0f),
                 row_height_});
        y += row_height_;
    }
}

void PopupMenu::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    painter.fill_rounded_rect(bounds(), 8.0f, tokens.surface);
    painter.stroke_rounded_rect(bounds(), 8.0f, tokens.border_strong, 1.0f);

    for (std::size_t index = 0; index < items_.size() && index < row_rects_.size(); ++index) {
        const Item& item = items_[index];
        const Rect row = row_rects_[index];
        if (item.separator) {
            painter.fill_rect(Rect{row.x, row.y + kSeparatorHeight * 0.5f, row.width, 1.0f},
                              tokens.border);
            continue;
        }

        if (static_cast<int>(index) == selected_ && item.enabled) {
            painter.fill_rounded_rect(row, 6.0f, tokens.accent_soft);
        }
        if (item.checked) {
            const float center_y = row.y + row.height * 0.5f;
            painter.draw_line(Point{row.x + 9.0f, center_y},
                              Point{row.x + 13.0f, center_y + 4.0f},
                              item.enabled ? tokens.accent : tokens.text_tertiary,
                              1.7f);
            painter.draw_line(Point{row.x + 13.0f, center_y + 4.0f},
                              Point{row.x + 20.0f, center_y - 5.0f},
                              item.enabled ? tokens.accent : tokens.text_tertiary,
                              1.7f);
        }

        const Color text_color = item.enabled ? tokens.text_primary : tokens.text_tertiary;
        const Rect text_rect{row.x + 30.0f,
                             row.y,
                             std::max(0.0f, row.width - 100.0f),
                             row.height};
        painter.draw_text(item.text,
                          text_rect,
                          FontRole::body,
                          text_color,
                          TextAlign::leading,
                          TextVAlign::center);
        if (!item.shortcut.empty()) {
            painter.draw_text(item.shortcut,
                              Rect{row.right() - 78.0f, row.y, 70.0f, row.height},
                              FontRole::caption,
                              tokens.text_tertiary,
                              TextAlign::trailing,
                              TextVAlign::center);
        }
    }
}

int PopupMenu::IndexAt(Point point) const noexcept {
    for (std::size_t index = 0; index < row_rects_.size(); ++index) {
        if (row_rects_[index].contains(point) && items_[index].enabled &&
            !items_[index].separator) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void PopupMenu::on_focus_changed(bool focused) {
    if (focused && selected_ < 0) {
        MoveSelection(1);
    }
}

void PopupMenu::on_mouse_down(Point point) {
    const int index = IndexAt(point);
    if (index >= 0) {
        selected_ = index;
    }
}

void PopupMenu::on_mouse_up(Point point) {
    const int index = IndexAt(point);
    if (index >= 0) {
        Activate(index);
    }
}

void PopupMenu::on_mouse_move(Point point) {
    const int index = IndexAt(point);
    if (index >= 0) {
        selected_ = index;
    }
}

void PopupMenu::MoveSelection(int delta) {
    if (items_.empty()) {
        return;
    }
    int index = selected_;
    if (index < 0) {
        index = delta >= 0 ? -1 : static_cast<int>(items_.size());
    }
    for (std::size_t step = 0; step < items_.size(); ++step) {
        index = (index + delta + static_cast<int>(items_.size())) %
                static_cast<int>(items_.size());
        const Item& item = items_[static_cast<std::size_t>(index)];
        if (item.enabled && !item.separator) {
            selected_ = index;
            return;
        }
    }
}

void PopupMenu::Activate(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    const Item& item = items_[static_cast<std::size_t>(index)];
    if (!item.enabled || item.separator) {
        return;
    }
    const std::function<void()> action = item.action;
    if (action) {
        action();
    }
    if (on_dismiss_) {
        on_dismiss_();
    }
}

bool PopupMenu::on_key_down(int key, KeyModifiers modifiers) {
    (void)modifiers;
    switch (key) {
        case kKeyUp:
            MoveSelection(-1);
            return true;
        case kKeyDown:
            MoveSelection(1);
            return true;
        case kKeyHome:
            selected_ = -1;
            MoveSelection(1);
            return true;
        case kKeyEnd:
            selected_ = 0;
            MoveSelection(-1);
            return true;
        case kKeyLeft:
        case kKeyRight:
            return true;
        case kKeyEnter:
        case kKeySpace:
            if (selected_ >= 0) {
                Activate(selected_);
            }
            return true;
        case kKeyEscape:
            if (on_dismiss_) {
                on_dismiss_();
            }
            return true;
        default:
            return false;
    }
}

ToolTip::ToolTip(std::wstring text) : text_(std::move(text)) {}

Size ToolTip::measure(Painter& painter, const Size& available) {
    const Size text = painter.measure_text(text_, FontRole::caption);
    const Size desired{std::min(available.width > 0.0f ? available.width : 360.0f,
                                text.width + 22.0f),
                       text.height + 12.0f};
    set_desired_size(desired);
    return desired;
}

void ToolTip::arrange(const Rect& bounds) {
    set_bounds(bounds);
}

void ToolTip::paint(Painter& painter) const {
    const ThemeTokens& tokens = painter.theme();
    painter.fill_rounded_rect(bounds(), 6.0f, tokens.surface);
    painter.stroke_rounded_rect(bounds(), 6.0f, tokens.border_strong, 1.0f);
    painter.draw_text(text_,
                      bounds().inset(Insets{10.0f, 4.0f, 10.0f, 4.0f}),
                      FontRole::caption,
                      tokens.text_primary,
                      TextAlign::leading,
                      TextVAlign::center);
}

PopupHost::PopupHost(std::unique_ptr<Widget> content) {
    if (content) {
        content_ = content.get();
        add_child(std::move(content));
    }
}

void PopupHost::set_focus_bridge(std::function<Widget*()> save_focus,
                                 std::function<void(Widget*)> restore_focus) {
    save_focus_ = std::move(save_focus);
    restore_focus_ = std::move(restore_focus);
}

void PopupHost::RequestLayout() {
    if (on_layout_requested_) {
        on_layout_requested_();
    }
}

bool PopupHost::open(std::unique_ptr<Widget> popup, Point position) {
    if (!popup) {
        return false;
    }
    close();
    hide_tooltip();

    popup_ = popup.get();
    popup_position_ = position;
    add_child(std::move(popup));
    if (auto* menu = dynamic_cast<PopupMenu*>(popup_)) {
        menu->set_on_dismiss([this] { close(); });
    }

    if (save_focus_) {
        previous_focus_ = save_focus_();
    }
    if (restore_focus_ && popup_->focusable()) {
        restore_focus_(popup_);
    }
    RequestLayout();
    return true;
}

void PopupHost::close() {
    if (popup_ == nullptr) {
        return;
    }
    Widget* closing = popup_;
    popup_ = nullptr;
    std::unique_ptr<Widget> owned = remove_child(closing);
    if (owned) {
        retired_.push_back(std::move(owned));
    }
    if (restore_focus_) {
        restore_focus_(previous_focus_);
    }
    previous_focus_ = nullptr;
    RequestLayout();
}

void PopupHost::show_tooltip(std::wstring text, Point position) {
    if (popup_ != nullptr || text.empty()) {
        return;
    }
    hide_tooltip();
    auto tooltip = std::make_unique<ToolTip>(std::move(text));
    tooltip_ = tooltip.get();
    tooltip_position_ = position;
    add_child(std::move(tooltip));
    RequestLayout();
}

void PopupHost::hide_tooltip() {
    if (tooltip_ == nullptr) {
        return;
    }
    std::unique_ptr<Widget> owned = remove_child(tooltip_);
    tooltip_ = nullptr;
    (void)owned;
    RequestLayout();
}

void PopupHost::on_mouse_down(Point point) {
    (void)point;
    hide_tooltip();
    if (popup_ != nullptr) {
        close();
    }
}

void PopupHost::collect_focusable(std::vector<Widget*>& output) {
    if (!visible()) {
        return;
    }
    if (popup_ != nullptr) {
        popup_->collect_focusable(output);
        return;
    }
    if (content_ != nullptr) {
        content_->collect_focusable(output);
    }
}

Widget* PopupHost::hit_test(Point point) const {
    if (!visible() || !bounds().contains(point)) {
        return nullptr;
    }
    if (popup_ != nullptr) {
        if (popup_->bounds().contains(point)) {
            if (Widget* hit = popup_->hit_test(point)) {
                return hit;
            }
        }
        return const_cast<PopupHost*>(this);
    }
    if (content_ != nullptr) {
        return content_->hit_test(point);
    }
    return nullptr;
}

Size PopupHost::measure(Painter& painter, const Size& available) {
    if (content_ != nullptr) {
        (void)content_->measure(painter, available);
    }
    if (popup_ != nullptr) {
        popup_size_ = popup_->measure(painter, available);
    }
    if (tooltip_ != nullptr) {
        tooltip_size_ = tooltip_->measure(painter, available);
    }
    set_desired_size(available);
    return available;
}

void PopupHost::arrange(const Rect& bounds) {
    set_bounds(bounds);
    if (content_ != nullptr) {
        content_->arrange(bounds);
    }
    if (popup_ != nullptr) {
        const float x = ClampOrCenter(popup_position_.x, bounds.x, popup_size_.width, bounds.width);
        const float y = ClampOrCenter(popup_position_.y, bounds.y, popup_size_.height, bounds.height);
        popup_->arrange(
            Rect{x, y, std::min(popup_size_.width, bounds.width),
                 std::min(popup_size_.height, bounds.height)});
    }
    if (tooltip_ != nullptr) {
        const float preferred_x = tooltip_position_.x + 14.0f;
        const float preferred_y = tooltip_position_.y + 20.0f;
        const float x =
            ClampOrCenter(preferred_x, bounds.x + 4.0f, tooltip_size_.width, bounds.width - 8.0f);
        const float y =
            ClampOrCenter(preferred_y, bounds.y + 4.0f, tooltip_size_.height, bounds.height - 8.0f);
        tooltip_->arrange(Rect{x, y, tooltip_size_.width, tooltip_size_.height});
    }
}

void PopupHost::paint(Painter& painter) const {
    if (content_ != nullptr) {
        content_->paint(painter);
    }
    if (popup_ != nullptr) {
        const Rect shadow{popup_->bounds().x + 2.0f,
                          popup_->bounds().y + 3.0f,
                          popup_->bounds().width,
                          popup_->bounds().height};
        painter.fill_rounded_rect(shadow, 9.0f, Color{0, 0, 0, 42});
        popup_->paint(painter);
    }
    if (tooltip_ != nullptr) {
        tooltip_->paint(painter);
    }
}

}  // namespace cppui
