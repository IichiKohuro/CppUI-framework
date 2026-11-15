#include <cppui/theme/mode.hpp>
#include <cppui/ui/command.hpp>
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

    [[nodiscard]] cppui::DensityMode density() const noexcept override { return density_; }
    void set_density(cppui::DensityMode density) noexcept { density_ = density; }

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

private:
    cppui::DensityMode density_{cppui::DensityMode::cozy};
};

bool Expect(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
    }
    return condition;
}

constexpr int kLeft = 0x25;
constexpr int kRight = 0x27;
constexpr int kBack = 0x08;
constexpr int kEnter = 0x0D;
constexpr int kA = 0x41;
constexpr int kZ = 0x5A;
constexpr int kY = 0x59;

}  // namespace

int main() {
    TestPainter painter;
    bool passed = true;

    // TextEdit: editing, selection, undo/redo, validation and density.
    cppui::TextEdit edit(L"Hello");
    (void)edit.measure(painter, cppui::Size{220.0f, 40.0f});
    edit.arrange(cppui::Rect{0.0f, 0.0f, 220.0f, 34.0f});
    passed &= Expect(edit.on_char(L'!'), "text edit accepts a character");
    passed &= Expect(edit.text() == L"Hello!", "text edit inserts a character");

    edit.on_key_down(kBack, {});
    passed &= Expect(edit.text() == L"Hello", "backspace removes a character");
    edit.on_key_down(kZ, cppui::KeyModifiers{.control = true});
    passed &= Expect(edit.text() == L"Hello!", "undo restores the edit");
    edit.on_key_down(kY, cppui::KeyModifiers{.control = true});
    passed &= Expect(edit.text() == L"Hello", "redo re-applies the edit");

    edit.select_all();
    passed &= Expect(edit.selection_begin() == 0 && edit.selection_end() == 5,
                     "select all covers the value");
    edit.on_key_down(kBack, {});
    passed &= Expect(edit.text().empty(), "backspace deletes the selection");

    edit.on_char(L'Ж');
    edit.on_char(L'!');
    passed &= Expect(edit.text() == L"Ж!", "text edit accepts non-ASCII input");

    edit.set_validation_error(L"Required field");
    passed &= Expect(edit.has_validation_error(), "text edit exposes validation state");
    edit.paint(painter);
    passed &= Expect(edit.ime_caret_rect().width > 0.0f, "text edit reports IME caret");
    edit.clear_validation_error();
    passed &= Expect(!edit.has_validation_error(), "validation state can be cleared");

    const cppui::Size cozy_edit = edit.measure(painter, cppui::Size{220.0f, 40.0f});
    painter.set_density(cppui::DensityMode::compact);
    const cppui::Size compact_edit = edit.measure(painter, cppui::Size{220.0f, 40.0f});
    painter.set_density(cppui::DensityMode::cozy);
    passed &= Expect(cozy_edit.height == 34.0f && compact_edit.height == 28.0f,
                     "text edit follows density metrics");

    // CheckBox including the indeterminate state.
    int checkbox_changes = 0;
    cppui::CheckBox check(L"Remember", false);
    check.set_on_changed([&checkbox_changes](cppui::CheckState) { ++checkbox_changes; });
    check.on_activate();
    passed &= Expect(check.checked(), "checkbox toggles on activation");
    check.set_tristate(true);
    check.on_activate();
    passed &= Expect(check.state() == cppui::CheckState::indeterminate,
                     "checkbox supports indeterminate state");
    check.on_activate();
    passed &= Expect(check.state() == cppui::CheckState::unchecked,
                     "checkbox cycles back to unchecked");
    passed &= Expect(checkbox_changes == 3, "checkbox reports every change");

    // Radio group selection and keyboard movement.
    cppui::RadioGroup radios;
    auto radio_a = std::make_unique<cppui::RadioButton>(L"A", true);
    auto radio_b = std::make_unique<cppui::RadioButton>(L"B", false);
    auto radio_c = std::make_unique<cppui::RadioButton>(L"C", false);
    cppui::RadioButton* radio_b_raw = radio_b.get();
    cppui::RadioButton* radio_c_raw = radio_c.get();
    radios.add_child(std::move(radio_a));
    radios.add_child(std::move(radio_b));
    radios.add_child(std::move(radio_c));
    passed &= Expect(radios.selected() != nullptr, "radio group selects an initial item");
    radio_b_raw->set_checked(true);
    passed &= Expect(radios.selected() == radio_b_raw && radio_b_raw->checked(),
                     "radio group selects a clicked item exclusively");
    (void)radios.measure(painter, cppui::Size{200.0f, 200.0f});
    radios.arrange(cppui::Rect{0.0f, 0.0f, 200.0f, 160.0f});
    passed &= Expect(radio_b_raw->on_key_down(kRight, {}), "radio consumes arrow key");
    passed &= Expect(radios.selected() == radio_c_raw && radio_c_raw->checked(),
                     "radio arrow moves selection");

    // Grid layout uses weighted columns.
    cppui::GridPanel grid;
    grid.set_columns({1.0f, 2.0f});
    grid.set_gap(0.0f, 0.0f);
    auto first = std::make_unique<cppui::Label>(L"First");
    auto second = std::make_unique<cppui::Label>(L"Second");
    cppui::Label* first_raw = first.get();
    cppui::Label* second_raw = second.get();
    grid.add_cell(std::move(first), 0, 0);
    grid.add_cell(std::move(second), 1, 0);
    (void)grid.measure(painter, cppui::Size{420.0f, 100.0f});
    grid.arrange(cppui::Rect{0.0f, 0.0f, 420.0f, 100.0f});
    passed &= Expect(first_raw->bounds().width == 140.0f, "grid first column weight");
    passed &= Expect(second_raw->bounds().x == 140.0f && second_raw->bounds().width == 280.0f,
                     "grid second column weight and offset");

    cppui::GridPanel header;
    header.set_columns({1.0f, 0.0f});
    header.set_gap(0.0f, 0.0f);
    auto left = std::make_unique<cppui::Label>(L"Left");
    auto action = std::make_unique<cppui::Button>(L"Action", cppui::ButtonStyle::secondary);
    cppui::Label* left_raw = left.get();
    cppui::Button* action_raw = action.get();
    header.add_cell(std::move(left), 0, 0);
    header.add_cell(std::move(action), 1, 0);
    (void)header.measure(painter, cppui::Size{400.0f, 80.0f});
    header.arrange(cppui::Rect{0.0f, 0.0f, 400.0f, 80.0f});
    passed &= Expect(action_raw->bounds().right() == 400.0f &&
                         action_raw->bounds().width < 200.0f,
                     "grid weight 0 is an auto-sized column");
    passed &= Expect(left_raw->bounds().width > action_raw->bounds().width,
                     "grid star column receives remaining width");

    // ScrollViewer wheel scrolling and clamping.
    auto stack = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 0.0f);
    for (int index = 0; index < 10; ++index) {
        stack->add_child(std::make_unique<cppui::Label>(L"Row"));
    }
    cppui::ScrollViewer scroll(std::move(stack));
    (void)scroll.measure(painter, cppui::Size{200.0f, 100.0f});
    scroll.arrange(cppui::Rect{0.0f, 0.0f, 200.0f, 100.0f});
    passed &= Expect(scroll.maximum_scroll() > 0.0f, "scroll viewer detects overflow");
    passed &= Expect(scroll.on_mouse_wheel(-1.0f), "scroll viewer consumes wheel");
    passed &= Expect(scroll.scroll_offset() > 0.0f, "wheel moves scroll offset");
    scroll.scroll_to(10000.0f);
    passed &= Expect(scroll.scroll_offset() == scroll.maximum_scroll(),
                     "scroll offset clamps to the maximum");

    // Feedback controls.
    int command_runs = 0;
    auto command = std::make_shared<cppui::Command>(L"Command", [&command_runs] { ++command_runs; });
    cppui::Button command_button(L"", cppui::ButtonStyle::primary);
    command_button.set_command(command);
    command_button.on_activate();
    passed &= Expect(command_runs == 1, "button executes its command");
    command->set_enabled(false);
    passed &= Expect(!command_button.enabled(), "command disabled state reaches button");
    command_button.on_activate();
    passed &= Expect(command_runs == 1, "disabled command does not execute");

    cppui::Label wrapped(L"one two three four five six", cppui::FontRole::body);
    wrapped.set_wrap(true);
    const cppui::Size wrapped_size =
        wrapped.measure(painter, cppui::Size{64.0f, 200.0f});
    passed &= Expect(wrapped_size.height > 16.0f, "label wraps into multiple lines");
    wrapped.arrange(cppui::Rect{0.0f, 0.0f, 64.0f, wrapped_size.height});
    wrapped.paint(painter);

    cppui::ProgressBar progress(1.7f);
    passed &= Expect(progress.value() == 1.0f, "progress value clamps");
    (void)progress.measure(painter, cppui::Size{300.0f, 40.0f});
    progress.arrange(cppui::Rect{0.0f, 0.0f, 300.0f, 24.0f});
    progress.paint(painter);
    cppui::Loader loader(L"Loading");
    (void)loader.measure(painter, cppui::Size{200.0f, 40.0f});
    loader.arrange(cppui::Rect{0.0f, 0.0f, 120.0f, 24.0f});
    loader.on_tick(0.016f);
    loader.paint(painter);
    cppui::BusyOverlay busy(L"Working");
    (void)busy.measure(painter, cppui::Size{500.0f, 400.0f});
    busy.arrange(cppui::Rect{0.0f, 0.0f, 500.0f, 400.0f});
    busy.paint(painter);

    // Images and PictureBox.
    const auto image = cppui::Image::FromPixels(
        2, 2, std::vector<std::uint8_t>(16, 255));
    passed &= Expect(image != nullptr && image->valid(), "image is created from pixels");
    cppui::PictureBox picture(image);
    (void)picture.measure(painter, cppui::Size{200.0f, 120.0f});
    picture.arrange(cppui::Rect{0.0f, 0.0f, 120.0f, 80.0f});
    picture.paint(painter);

    // Popup host: menu activation, click-outside dismiss and passive tooltip.
    auto host = std::make_unique<cppui::PopupHost>(
        std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 0.0f));
    cppui::PopupHost* host_raw = host.get();
    cppui::UiHost ui;
    ui.set_root(std::move(host));
    host_raw->set_focus_bridge([&ui]() -> cppui::Widget* { return ui.focused(); },
                               [&ui](cppui::Widget* widget) { ui.set_focus(widget); });
    ui.layout(painter, cppui::Rect{0.0f, 0.0f, 500.0f, 400.0f});

    int actions = 0;
    auto menu = std::make_unique<cppui::PopupMenu>();
    menu->add_item(L"Action", [&actions] { ++actions; });
    menu->add_item(L"Disabled", {}, {}, false);
    menu->add_separator();
    menu->add_item(L"Other");
    cppui::PopupMenu* menu_raw = menu.get();
    passed &= Expect(host_raw->open(std::move(menu), cppui::Point{40.0f, 50.0f}),
                     "popup opens");
    ui.layout(painter, cppui::Rect{0.0f, 0.0f, 500.0f, 400.0f});
    passed &= Expect(host_raw->is_open() && ui.focused() == menu_raw,
                     "popup receives focus");
    const cppui::Point first_item{menu_raw->bounds().x + 20.0f,
                                  menu_raw->bounds().y + 16.0f};
    ui.mouse_down(first_item);
    ui.mouse_up(first_item);
    passed &= Expect(actions == 1, "popup item activates");
    passed &= Expect(!host_raw->is_open(), "popup closes after activation");

    auto menu2 = std::make_unique<cppui::PopupMenu>();
    menu2->add_item(L"One");
    menu2->add_item(L"Two");
    passed &= Expect(host_raw->open(std::move(menu2), cppui::Point{40.0f, 50.0f}),
                     "second popup opens");
    ui.layout(painter, cppui::Rect{0.0f, 0.0f, 500.0f, 400.0f});
    ui.mouse_down(cppui::Point{300.0f, 300.0f});
    ui.mouse_up(cppui::Point{300.0f, 300.0f});
    passed &= Expect(!host_raw->is_open(), "click outside closes popup");

    host_raw->show_tooltip(L"Tip", cppui::Point{80.0f, 80.0f});
    ui.layout(painter, cppui::Rect{0.0f, 0.0f, 500.0f, 400.0f});
    passed &= Expect(host_raw->tooltip_visible(), "tooltip is shown");
    passed &= Expect(host_raw->hit_test(cppui::Point{80.0f, 80.0f}) == nullptr,
                     "tooltip is passive for hit testing");
    host_raw->hide_tooltip();
    passed &= Expect(!host_raw->tooltip_visible(), "tooltip hides");

    // Layout containers.
    cppui::StackPanel hidden_stack(cppui::Orientation::vertical, 6.0f);
    hidden_stack.add_child(std::make_unique<cppui::Label>(L"Visible"));
    auto hidden = std::make_unique<cppui::Label>(L"Hidden");
    hidden->set_visible(false);
    hidden_stack.add_child(std::move(hidden));
    const cppui::Size hidden_size =
        hidden_stack.measure(painter, cppui::Size{200.0f, 100.0f});
    passed &= Expect(hidden_size.height == 16.0f, "hidden child does not consume layout");

    cppui::GroupPanel group(L"Group");
    group.add_child(std::make_unique<cppui::Label>(L"Value"));
    (void)group.measure(painter, cppui::Size{300.0f, 200.0f});
    group.arrange(cppui::Rect{0.0f, 0.0f, 300.0f, 200.0f});
    group.paint(painter);

    if (passed) {
        std::cout << "CppUI v0.1 control tests passed.\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
