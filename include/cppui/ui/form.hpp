#pragma once

#include <memory>
#include <string>
#include <utility>

#include <cppui/ui/painter.hpp>
#include <cppui/ui/widget.hpp>

namespace cppui {

class StatusBar final : public Widget {
public:
    StatusBar() = default;

    void set_left_text(std::wstring text) { left_text_ = std::move(text); }
    void set_right_text(std::wstring text) { right_text_ = std::move(text); }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    std::wstring left_text_;
    std::wstring right_text_;
};

class FormPanel final : public Widget {
public:
    FormPanel(std::unique_ptr<Widget> content, std::unique_ptr<Widget> status_bar);

    [[nodiscard]] Widget* content() const noexcept;
    [[nodiscard]] Widget* status_bar() const noexcept;

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override;
    void arrange(const Rect& bounds) override;
    void paint(Painter& painter) const override;

private:
    Size content_size_{};
    Size status_size_{};
};

}  // namespace cppui
