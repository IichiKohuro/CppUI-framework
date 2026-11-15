#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace cppui {

// Small command model shared by buttons and menu items. It deliberately does
// not own observers yet: callers invalidate the host after changing state.
class Command final {
public:
    Command(std::wstring text = {}, std::function<void()> execute = {})
        : text_(std::move(text)), execute_(std::move(execute)) {}

    void set_text(std::wstring text) { text_ = std::move(text); }
    [[nodiscard]] const std::wstring& text() const noexcept { return text_; }
    void set_shortcut(std::wstring shortcut) { shortcut_ = std::move(shortcut); }
    [[nodiscard]] const std::wstring& shortcut() const noexcept { return shortcut_; }
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    void set_checked(bool checked) noexcept { checked_ = checked; }
    [[nodiscard]] bool checked() const noexcept { return checked_; }
    void set_execute(std::function<void()> execute) { execute_ = std::move(execute); }

    [[nodiscard]] bool Execute() const {
        if (!enabled_ || !execute_) {
            return false;
        }
        execute_();
        return true;
    }

private:
    std::wstring text_;
    std::wstring shortcut_;
    std::function<void()> execute_;
    bool enabled_{true};
    bool checked_{false};
};

}  // namespace cppui
