#include <cppui/theme/mode.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/painter.hpp>
#include <cppui/ui/widget.hpp>
#include <cppui/ui/widgets.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

class TestPainter final : public cppui::Painter {
public:
    [[nodiscard]] const cppui::ThemeTokens& theme() const noexcept override {
        return cppui::light_theme();
    }

    [[nodiscard]] cppui::Size measure_text(std::wstring_view text,
                                           cppui::FontRole role) const override {
        const float size = role == cppui::FontRole::title ? 24.0f : 16.0f;
        return cppui::Size{static_cast<float>(text.size()) * 8.0f, size};
    }

    void fill_rect(const cppui::Rect&, const cppui::Color&) override {}
    void fill_rounded_rect(const cppui::Rect&, float, const cppui::Color&) override {}
    void stroke_rounded_rect(const cppui::Rect&,
                             float,
                             const cppui::Color&,
                             float) override {}
    void draw_text(std::wstring_view,
                   const cppui::Rect&,
                   cppui::FontRole,
                   const cppui::Color&,
                   cppui::TextAlign,
                   cppui::TextVAlign) override {}
};

bool Expect(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    TestPainter painter;
    bool passed = true;

    passed &= Expect(cppui::resolve_theme(cppui::ThemeMode::light).accent ==
                         cppui::light_theme().accent,
                     "light theme resolves");
    passed &= Expect(cppui::resolve_theme(cppui::ThemeMode::dark).accent ==
                         cppui::dark_theme().accent,
                     "dark theme resolves");

    int activations = 0;
    cppui::UiHost host;

    auto root = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 0.0f);
    root->set_padding(cppui::Insets{10.0f, 10.0f, 10.0f, 10.0f});
    root->set_cross_align(cppui::CrossAlign::start);

    auto button = std::make_unique<cppui::Button>(L"Press", cppui::ButtonStyle::primary);
    button->set_on_click([&activations] { ++activations; });
    root->add_child(std::move(button));

    auto disabled = std::make_unique<cppui::Button>(L"Disabled", cppui::ButtonStyle::secondary);
    disabled->set_enabled(false);
    root->add_child(std::move(disabled));
    host.set_root(std::move(root));
    host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});

    passed &= Expect(host.root() != nullptr, "root exists");
    passed &= Expect(host.root()->hit_test(cppui::Point{20.0f, 20.0f}) != nullptr,
                     "button is hit-testable");

    host.mouse_down(cppui::Point{20.0f, 20.0f});
    host.mouse_up(cppui::Point{20.0f, 20.0f});
    passed &= Expect(activations == 1, "mouse click activates button");
    passed &= Expect(host.focused() != nullptr, "click focuses button");

    passed &= Expect(host.key_down(0x20, false), "space is consumed by focused button");
    passed &= Expect(activations == 2, "space activates button");

    host.mouse_leave();
    passed &= Expect(activations == 2, "mouse leave does not activate");

    passed &= Expect(host.root()->hit_test(cppui::Point{20.0f, 60.0f}) == nullptr,
                     "disabled button is not hit-testable");
    host.mouse_down(cppui::Point{20.0f, 60.0f});
    host.mouse_up(cppui::Point{20.0f, 60.0f});
    passed &= Expect(activations == 2, "disabled button does not activate");

    if (passed) {
        std::cout << "CppUI UI tests passed.\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
