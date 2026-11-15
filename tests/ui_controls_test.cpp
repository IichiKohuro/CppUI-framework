#include <cppui/theme/mode.hpp>
#include <cppui/ui/containers.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/image.hpp>
#include <cppui/ui/indicators.hpp>
#include <cppui/ui/input.hpp>
#include <cppui/ui/painter.hpp>
#include <cppui/ui/popup.hpp>
#include <cppui/ui/widget.hpp>
#include <cppui/ui/widgets.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

class TestPainter final : public cppui::Painter {
public:
    [[nodiscard]] const cppui::ThemeTokens& theme() const noexcept override {
        return cppui::light_theme();
    }
    [[nodiscard]] cppui::DensityMode density() const noexcept override {
        return cppui::DensityMode::cozy;
    }
    [[nodiscard]] cppui::Size measure_text(std::wstring_view text,
                                           cppui::FontRole role) const override {
        const float height = role == cppui::FontRole::title ? 24.0f : 16.0f;
        return cppui::Size{static_cast<float>(text.size()) * 8.0f, height};
    }
    void fill_rect(const cppui::Rect&, const cppui::Color&) override {}
    void fill_rounded_rect(const cppui::Rect&, float, const cppui::Color&) override {}
    void stroke_rounded_rect(const cppui::Rect&, float, const cppui::Color&, float) override {}
    void draw_line(cppui::Point, cppui::Point, const cppui::Color&, float) override {}
    void draw_image(const cppui::Rect&, const cppui::Image&, float) override {}
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

constexpr int kSpace = 0x20;
constexpr int kEnter = 0x0D;
constexpr int kRight = 0x27;
constexpr int kEnd = 0x23;
constexpr int kLeft = 0x25;

template <typename T>
T* Adopt(cppui::UiHost& host, std::unique_ptr<T> widget) {
    T* raw = widget.get();
    host.set_root(std::move(widget));
    return raw;
}

}  // namespace

int main() {
    TestPainter painter;
    bool passed = true;

    // Button: hover, pressed, activation, drag cancellation, keyboard.
    {
        cppui::UiHost host;
        int clicks = 0;
        auto widget = std::make_unique<cppui::Button>(L"Action", cppui::ButtonStyle::primary);
        widget->set_on_click([&clicks] { ++clicks; });
        cppui::Button* button = Adopt(host, std::move(widget));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 200.0f, 80.0f});

        host.mouse_move(cppui::Point{50.0f, 40.0f});
        passed &= Expect(button->hovered(), "button hover");
        host.mouse_down(cppui::Point{50.0f, 40.0f});
        passed &= Expect(button->pressed() && host.focused() == button, "button pressed/focused");
        host.mouse_up(cppui::Point{50.0f, 40.0f});
        passed &= Expect(clicks == 1, "button click");
        passed &= Expect(!button->pressed(), "button releases pressed state");

        host.mouse_down(cppui::Point{50.0f, 40.0f});
        host.mouse_move(cppui::Point{250.0f, 40.0f});
        host.mouse_up(cppui::Point{250.0f, 40.0f});
        passed &= Expect(clicks == 1, "button drag outside cancels click");

        host.mouse_down(cppui::Point{50.0f, 40.0f});
        host.mouse_up(cppui::Point{50.0f, 40.0f});
        const int before_keyboard = clicks;
        passed &= Expect(host.key_down(kSpace, false), "button consumes Space");
        passed &= Expect(clicks == before_keyboard + 1, "Space activates button");
        passed &= Expect(host.key_down(kEnter, false), "button consumes Enter");
        passed &= Expect(clicks == before_keyboard + 2, "Enter activates button");
    }

    // CheckBox: click and keyboard state transitions.
    {
        cppui::UiHost host;
        cppui::CheckState state = cppui::CheckState::unchecked;
        auto widget = std::make_unique<cppui::CheckBox>(L"Remember", false);
        widget->set_on_changed([&state](cppui::CheckState value) { state = value; });
        cppui::CheckBox* check = Adopt(host, std::move(widget));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 200.0f, 60.0f});

        host.mouse_down(cppui::Point{50.0f, 30.0f});
        host.mouse_up(cppui::Point{50.0f, 30.0f});
        passed &= Expect(check->checked() && state == cppui::CheckState::checked,
                         "checkbox toggles on click");
        passed &= Expect(host.key_down(kSpace, false), "checkbox consumes Space");
        passed &= Expect(!check->checked() && state == cppui::CheckState::unchecked,
                         "checkbox toggles on Space");
        host.key_down(kEnter, false);
        passed &= Expect(check->checked(), "checkbox toggles on Enter");
    }

    // RadioGroup: mouse selection + arrow movement.
    {
        cppui::UiHost host;
        auto group = std::make_unique<cppui::RadioGroup>();
        auto a = std::make_unique<cppui::RadioButton>(L"A", true);
        auto b = std::make_unique<cppui::RadioButton>(L"B", false);
        auto c = std::make_unique<cppui::RadioButton>(L"C", false);
        cppui::RadioButton* b_raw = b.get();
        cppui::RadioButton* c_raw = c.get();
        group->add_child(std::move(a));
        group->add_child(std::move(b));
        group->add_child(std::move(c));
        cppui::RadioGroup* group_raw = group.get();
        Adopt(host, std::move(group));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 220.0f, 120.0f});

        host.mouse_down(cppui::Point{40.0f, 51.0f});
        host.mouse_up(cppui::Point{40.0f, 51.0f});
        passed &= Expect(group_raw->selected() == b_raw && b_raw->checked(),
                         "radio click selects item");
        passed &= Expect(host.key_down(kRight, false), "radio consumes arrow");
        passed &= Expect(group_raw->selected() == c_raw && c_raw->checked(),
                         "radio arrow moves selection");
        passed &= Expect(host.focused() == c_raw, "radio arrow moves focus with selection");
    }

    // TextEdit: mouse focus/caret, typing, selection, deletion, undo.
    {
        cppui::UiHost host;
        auto edit = std::make_unique<cppui::TextEdit>(L"abcdef");
        cppui::TextEdit* text = Adopt(host, std::move(edit));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 240.0f, 34.0f});

        host.mouse_down(cppui::Point{180.0f, 17.0f});
        host.mouse_up(cppui::Point{180.0f, 17.0f});
        passed &= Expect(host.focused() == text, "text edit receives focus");
        passed &= Expect(text->caret() > 0, "text edit places caret from click");
        host.char_input(L'X');
        passed &= Expect(text->text().find(L'X') != std::wstring::npos,
                         "text edit accepts host character");

        host.key_down(kEnd, false);
        const std::size_t end = text->caret();
        cppui::KeyModifiers shift{};
        shift.shift = true;
        host.key_down(kLeft, shift);
        passed &= Expect(text->selection_end() == end && text->selection_begin() < end,
                         "shift+left creates selection");
        host.key_down(0x08, false);
        passed &= Expect(text->selection_begin() == text->selection_end(),
                         "backspace removes selection");
        host.key_down(0x5A, cppui::KeyModifiers{.control = true});
        passed &= Expect(text->text() == L"abcdefX", "undo restores selected deletion");

        host.mouse_leave();
        passed &= Expect(text->focused(), "mouse leave does not clear widget focus");
    }

    // ScrollViewer: wheel and scrollbar drag.
    {
        cppui::UiHost host;
        auto stack = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 0.0f);
        for (int index = 0; index < 10; ++index) {
            stack->add_child(std::make_unique<cppui::Label>(L"Row"));
        }
        auto viewer = std::make_unique<cppui::ScrollViewer>(std::move(stack));
        cppui::ScrollViewer* scroll = Adopt(host, std::move(viewer));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 200.0f, 100.0f});
        passed &= Expect(scroll->maximum_scroll() > 0.0f, "scroll viewer has range");
        passed &= Expect(host.mouse_wheel(cppui::Point{50.0f, 50.0f}, -1.0f),
                         "scroll viewer consumes wheel");
        passed &= Expect(scroll->scroll_offset() > 0.0f, "wheel changes scroll offset");

        scroll->scroll_to(0.0f);
        host.mouse_down(cppui::Point{194.0f, 10.0f});
        host.mouse_move(cppui::Point{194.0f, 80.0f});
        host.mouse_up(cppui::Point{194.0f, 80.0f});
        passed &= Expect(scroll->scroll_offset() > 0.0f, "scrollbar drag changes offset");
    }

    // Popup: open, focus trap, keyboard action, click-outside dismiss.
    {
        cppui::UiHost host;
        auto popup_host = std::make_unique<cppui::PopupHost>(
            std::make_unique<cppui::Label>(L"content"));
        cppui::PopupHost* host_raw = popup_host.get();
        Adopt(host, std::move(popup_host));
        host_raw->set_focus_bridge([&host] { return host.focused(); },
                                   [&host](cppui::Widget* widget) { host.set_focus(widget); });
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});

        int actions = 0;
        auto menu = std::make_unique<cppui::PopupMenu>();
        menu->add_item(L"First", [&actions] { ++actions; });
        menu->add_item(L"Second");
        cppui::PopupMenu* menu_raw = menu.get();
        host_raw->open(std::move(menu), cppui::Point{40.0f, 40.0f});
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});
        passed &= Expect(host.focused() == menu_raw, "popup receives focus");
        host.key_down(0x09, false);
        passed &= Expect(host.focused() == menu_raw, "popup traps Tab focus");
        host.key_down(kEnter, false);
        passed &= Expect(actions == 1, "popup keyboard activates item");
        passed &= Expect(!host_raw->is_open(), "popup closes after keyboard action");

        auto menu2 = std::make_unique<cppui::PopupMenu>();
        menu2->add_item(L"One");
        host_raw->open(std::move(menu2), cppui::Point{40.0f, 40.0f});
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});
        host.mouse_down(cppui::Point{350.0f, 250.0f});
        host.mouse_up(cppui::Point{350.0f, 250.0f});
        passed &= Expect(!host_raw->is_open(), "popup click-outside closes");
    }

    // BusyOverlay: blocks pointer and keyboard focus.
    {
        cppui::UiHost host;
        auto panel = std::make_unique<cppui::Panel>(cppui::PanelStyle::none);
        auto edit = std::make_unique<cppui::TextEdit>(L"base");
        cppui::TextEdit* base = edit.get();
        panel->add_child(std::move(edit));
        auto busy = std::make_unique<cppui::BusyOverlay>(L"Working");
        cppui::BusyOverlay* overlay = busy.get();
        overlay->set_visible(false);
        panel->add_child(std::move(busy));
        Adopt(host, std::move(panel));
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});
        host.mouse_down(cppui::Point{50.0f, 20.0f});
        host.mouse_up(cppui::Point{50.0f, 20.0f});
        passed &= Expect(host.focused() == base, "base text focused before overlay");
        overlay->set_visible(true);
        host.layout(painter, cppui::Rect{0.0f, 0.0f, 400.0f, 300.0f});
        host.key_down(0x09, false);
        passed &= Expect(host.focused() == nullptr, "overlay traps keyboard focus");
        host.char_input(L'Z');
        passed &= Expect(base->text() == L"base", "overlay blocks text input");
        host.mouse_down(cppui::Point{80.0f, 30.0f});
        host.mouse_up(cppui::Point{80.0f, 30.0f});
        passed &= Expect(base->text() == L"base", "overlay blocks mouse activation");
    }

    // Feedback and image controls still paint and tick without a window.
    {
        cppui::ProgressBar progress(0.4f);
        progress.set_value(2.0f);
        passed &= Expect(progress.value() == 1.0f, "progress clamp");
        (void)progress.measure(painter, cppui::Size{220.0f, 24.0f});
        progress.arrange(cppui::Rect{0.0f, 0.0f, 220.0f, 24.0f});
        progress.paint(painter);

        cppui::Loader loader(L"Loading");
        passed &= Expect(loader.animating(), "loader reports animation");
        (void)loader.measure(painter, cppui::Size{200.0f, 24.0f});
        loader.arrange(cppui::Rect{0.0f, 0.0f, 120.0f, 24.0f});
        loader.on_tick(0.016f);
        loader.paint(painter);

        const auto image = cppui::Image::FromPixels(
            2, 2, std::vector<std::uint8_t>(16, 255));
        cppui::PictureBox picture(image);
        (void)picture.measure(painter, cppui::Size{120.0f, 80.0f});
        picture.arrange(cppui::Rect{0.0f, 0.0f, 120.0f, 80.0f});
        picture.paint(painter);
    }

    if (passed) {
        std::cout << "CppUI control interaction tests passed.\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
