#include <cppui/ui/dialog.hpp>

#include <windows.h>

#undef MessageBox

#include <algorithm>
#include <cmath>
#include <utility>

#include <cppui/form.hpp>
#include <cppui/ui/host.hpp>
#include <cppui/ui/painter.hpp>
#include <cppui/ui/popup.hpp>

#include "platform/win32/win32_window.hpp"
#include "render/d2d/d2d_renderer.hpp"

namespace cppui {
namespace {

constexpr float kDialogPadding = 24.0f;
constexpr float kDialogGap = 16.0f;

class DialogSurface final : public Widget {
public:
    explicit DialogSurface(std::unique_ptr<Widget> content) {
        if (content) {
            content_ = content.get();
            add_child(std::move(content));
        }
    }

    void AddButton(std::unique_ptr<Button> button) {
        if (!button) {
            return;
        }
        buttons_.push_back(button.get());
        add_child(std::move(button));
    }

    [[nodiscard]] Size measure(Painter& painter, const Size& available) override {
        const float inner_width =
            std::max(0.0f, available.width - kDialogPadding * 2.0f);
        const float inner_height =
            std::max(0.0f, available.height - kDialogPadding * 2.0f);

        footer_height_ = 0.0f;
        button_sizes_.clear();
        for (Button* button : buttons_) {
            const Size size = button->measure(painter, Size{inner_width, inner_height});
            button_sizes_.push_back(size);
            footer_height_ = std::max(footer_height_, size.height);
        }

        float content_height = 0.0f;
        if (content_ != nullptr) {
            content_height = content_
                                 ->measure(painter,
                                           Size{inner_width,
                                                std::max(0.0f,
                                                         inner_height - footer_height_ -
                                                             (buttons_.empty() ? 0.0f
                                                                              : kDialogGap))})
                                 .height;
        }
        (void)content_height;
        set_desired_size(available);
        return available;
    }

    void arrange(const Rect& bounds) override {
        set_bounds(bounds);
        const Rect inner = bounds.inset(Insets{kDialogPadding, kDialogPadding, kDialogPadding,
                                               kDialogPadding});
        const float footer_space =
            buttons_.empty() ? 0.0f : footer_height_ + kDialogGap;
        const Rect content_rect{inner.x,
                                inner.y,
                                inner.width,
                                std::max(0.0f, inner.height - footer_space)};
        if (content_ != nullptr) {
            content_->arrange(content_rect);
        }

        footer_top_ = inner.bottom() - footer_height_;
        float x = inner.right();
        for (std::size_t index = buttons_.size(); index > 0; --index) {
            Button* button = buttons_[index - 1];
            const Size size = button_sizes_[index - 1];
            x -= size.width;
            button->arrange(Rect{x, footer_top_, size.width, size.height});
            x -= 8.0f;
        }
    }

    void paint(Painter& painter) const override {
        const ThemeTokens& tokens = painter.theme();
        painter.fill_rect(bounds(), tokens.surface);
        if (!buttons_.empty()) {
            painter.fill_rect(Rect{bounds().x + kDialogPadding,
                                   footer_top_ - kDialogGap * 0.5f,
                                   std::max(0.0f, bounds().width - kDialogPadding * 2.0f),
                                   1.0f},
                              tokens.border);
        }
        paint_children(painter);
    }

private:
    Widget* content_{};
    std::vector<Button*> buttons_;
    std::vector<Size> button_sizes_;
    float footer_height_{};
    float footer_top_{};
};

}  // namespace

struct Dialog::Impl {
    struct ButtonSpec {
        std::wstring text;
        DialogResult result{DialogResult::cancel};
        ButtonStyle style{ButtonStyle::secondary};
    };

    Impl(std::wstring dialog_title, int width, int height)
        : title(std::move(dialog_title)), width_dip(width), height_dip(height) {}

    bool Create();
    void BuildRoot();
    void Layout();
    void Paint();
    void SetTheme(ThemeMode mode);
    void SetDensity(DensityMode mode);
    bool OnKeyDown(WPARAM key);
    bool OnChar(wchar_t character);
    void OnResize(std::uint32_t width, std::uint32_t height, float new_dpi);
    void OnDpiChanged(float new_dpi);
    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnMouseUp(int x, int y);
    void OnMouseLeave();
    void OnMouseWheel(int x, int y, float delta);
    void UpdateImePosition();
    void Finish(DialogResult value);
    void Shutdown();
    [[nodiscard]] Point ToDip(int x, int y) const;

    std::wstring title;
    int width_dip{};
    int height_dip{};
    std::unique_ptr<Win32Window> window;
    std::unique_ptr<D2DRenderer> renderer;
    UiHost ui;
    std::unique_ptr<Widget> pending_content;
    std::vector<ButtonSpec> buttons;
    DialogResult result{DialogResult::none};
    DialogResult default_result{DialogResult::none};
    ThemeMode theme_mode{ThemeMode::light};
    DensityMode density_mode{DensityMode::cozy};
    float dpi{96.0f};
    bool created{false};
    HWND owner{};
    Button* default_button{};
    Button* first_button{};
};

bool Dialog::Impl::Create() {
    if (created) {
        return true;
    }

    window = std::make_unique<Win32Window>(GetModuleHandleW(nullptr),
                                           title,
                                           width_dip,
                                           height_dip,
                                           360,
                                           170);
    window->SetQuitOnDestroy(false);
    window->SetThemeMode(theme_mode);
    window->SetChromeExtras(false, false, false);

    Win32Window::Callbacks callbacks{};
    callbacks.paint = [this] { Paint(); };
    callbacks.resized = [this](std::uint32_t width, std::uint32_t height, float new_dpi) {
        OnResize(width, height, new_dpi);
    };
    callbacks.dpi_changed = [this](float new_dpi) { OnDpiChanged(new_dpi); };
    callbacks.key_down = [this](WPARAM key) { return OnKeyDown(key); };
    callbacks.char_input = [this](wchar_t character) { return OnChar(character); };
    callbacks.mouse_move = [this](int x, int y) { OnMouseMove(x, y); };
    callbacks.mouse_down = [this](int x, int y) { OnMouseDown(x, y); };
    callbacks.mouse_up = [this](int x, int y) { OnMouseUp(x, y); };
    callbacks.mouse_wheel = [this](int x, int y, float delta) { OnMouseWheel(x, y, delta); };
    callbacks.mouse_leave = [this] { OnMouseLeave(); };
    callbacks.theme_selected = [this](ThemeMode mode) { SetTheme(mode); };
    callbacks.closing = [this] {
        if (result == DialogResult::none) {
            result = DialogResult::cancel;
        }
    };

    if (!window->Create(std::move(callbacks))) {
        window.reset();
        return false;
    }

    renderer = std::make_unique<D2DRenderer>();
    if (!renderer->Initialize(window->Handle(), resolve_theme(theme_mode))) {
        window->Destroy();
        renderer.reset();
        window.reset();
        return false;
    }
    renderer->SetDensity(density_mode);
    dpi = window->Dpi();
    created = true;
    BuildRoot();
    Layout();
    return true;
}

void Dialog::Impl::BuildRoot() {
    std::unique_ptr<Widget> content;
    if (pending_content) {
        content = std::move(pending_content);
    } else {
        content = std::make_unique<StackPanel>();
    }

    auto surface = std::make_unique<DialogSurface>(std::move(content));
    default_button = nullptr;
    first_button = nullptr;
    for (const ButtonSpec& spec : buttons) {
        auto button = std::make_unique<Button>(spec.text, spec.style);
        Button* raw = button.get();
        const DialogResult button_result = spec.result;
        button->set_on_click([this, button_result] { Finish(button_result); });
        if (first_button == nullptr) {
            first_button = raw;
        }
        if (spec.result == default_result) {
            default_button = raw;
        }
        surface->AddButton(std::move(button));
    }
    ui.set_root(std::move(surface));
    ui.set_focus(default_button != nullptr ? default_button : first_button);
}

void Dialog::Impl::Layout() {
    if (!created || !window || !renderer || ui.root() == nullptr) {
        return;
    }
    RECT client{};
    if (!GetClientRect(window->Handle(), &client)) {
        return;
    }
    const UINT width = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
    const UINT height = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));
    dpi = window->Dpi();
    if (!renderer->Resize(width, height, dpi)) {
        return;
    }
    const float dip_width = static_cast<float>(width) * 96.0f / dpi;
    const float dip_height = static_cast<float>(height) * 96.0f / dpi;
    ui.layout(*renderer,
              Rect{0.0f, kTitleBarHeightDip, std::max(0.0f, dip_width),
                   std::max(0.0f, dip_height - kTitleBarHeightDip)});
    UpdateImePosition();
}

void Dialog::Impl::UpdateImePosition() {
    if (!window) {
        return;
    }
    Widget* const focused = ui.focused();
    if (focused == nullptr) {
        return;
    }
    const Rect caret = focused->ime_caret_rect();
    if (caret.width <= 0.0f || caret.height <= 0.0f) {
        return;
    }
    const float scale = dpi / 96.0f;
    window->SetImeCaretRect(static_cast<int>(std::lround(caret.x * scale)),
                            static_cast<int>(std::lround(caret.y * scale)),
                            static_cast<int>(std::lround(caret.height * scale)));
}

void Dialog::Impl::Paint() {
    if (!created || !renderer || !window) {
        return;
    }
    (void)renderer->Render(window->GetChromeState(), ui.root());
}

void Dialog::Impl::SetTheme(ThemeMode mode) {
    theme_mode = mode;
    if (!created || !renderer || !window) {
        return;
    }
    if (!renderer->SetTheme(resolve_theme(mode))) {
        return;
    }
    window->SetThemeMode(mode);
    Layout();
    window->Invalidate();
}

void Dialog::Impl::SetDensity(DensityMode mode) {
    density_mode = mode;
    if (renderer) {
        renderer->SetDensity(mode);
    }
    if (created) {
        Layout();
        if (window) {
            window->Invalidate();
        }
    }
}

Point Dialog::Impl::ToDip(int x, int y) const {
    return Point{static_cast<float>(x) * 96.0f / dpi,
                 static_cast<float>(y) * 96.0f / dpi};
}

void Dialog::Impl::OnResize(std::uint32_t width, std::uint32_t height, float new_dpi) {
    (void)width;
    (void)height;
    dpi = new_dpi;
    Layout();
}

void Dialog::Impl::OnDpiChanged(float new_dpi) {
    dpi = new_dpi;
    Layout();
}

void Dialog::Impl::OnMouseMove(int x, int y) {
    ui.mouse_move(ToDip(x, y));
    if (window) {
        window->Invalidate();
    }
}

void Dialog::Impl::OnMouseDown(int x, int y) {
    ui.mouse_down(ToDip(x, y));
    UpdateImePosition();
    if (window) {
        window->Invalidate();
    }
}

void Dialog::Impl::OnMouseUp(int x, int y) {
    ui.mouse_up(ToDip(x, y));
    if (window) {
        window->Invalidate();
    }
}

void Dialog::Impl::OnMouseLeave() {
    ui.mouse_leave();
    if (window) {
        window->Invalidate();
    }
}

void Dialog::Impl::OnMouseWheel(int x, int y, float delta) {
    if (ui.mouse_wheel(ToDip(x, y), delta) && window) {
        window->Invalidate();
    }
}

bool Dialog::Impl::OnKeyDown(WPARAM key) {
    KeyModifiers modifiers{};
    modifiers.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    modifiers.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (ui.key_down(static_cast<int>(key), modifiers)) {
        UpdateImePosition();
        return true;
    }
    if (key == VK_ESCAPE) {
        Finish(DialogResult::cancel);
        return true;
    }
    if (key == VK_RETURN && default_button != nullptr) {
        default_button->on_activate();
        return true;
    }
    return false;
}

bool Dialog::Impl::OnChar(wchar_t character) {
    if (!created || !ui.char_input(character)) {
        return false;
    }
    UpdateImePosition();
    if (window) {
        window->Invalidate();
    }
    return true;
}

void Dialog::Impl::Finish(DialogResult value) {
    result = value;
    if (window) {
        window->Destroy();
    }
}

void Dialog::Impl::Shutdown() {
    if (renderer) {
        renderer->Shutdown();
    }
    if (window) {
        window->Destroy();
    }
    ui.set_root(nullptr);
    pending_content.reset();
    buttons.clear();
    default_button = nullptr;
    first_button = nullptr;
    window.reset();
    renderer.reset();
    created = false;
    owner = nullptr;
}

Dialog::Dialog(std::wstring title, int width_dip, int height_dip)
    : impl_(std::make_unique<Impl>(std::move(title), width_dip, height_dip)) {}

Dialog::~Dialog() {
    if (impl_) {
        impl_->Shutdown();
    }
}

void Dialog::SetContent(std::unique_ptr<Widget> content) {
    if (impl_ && !impl_->created) {
        impl_->pending_content = std::move(content);
    }
}

void Dialog::AddButton(std::wstring text, DialogResult result, ButtonStyle style) {
    if (impl_ && !impl_->created) {
        impl_->buttons.push_back(Impl::ButtonSpec{std::move(text), result, style});
    }
}

DialogResult Dialog::ShowModal(Form& owner) {
    theme_mode_ = owner.theme_mode();
    density_mode_ = owner.density_mode();
    return ShowModal(owner.NativeHandle());
}

DialogResult Dialog::ShowModal(void* owner_handle) {
    if (!impl_) {
        return DialogResult::none;
    }
    impl_->theme_mode = theme_mode_;
    impl_->density_mode = density_mode_;
    impl_->default_result = default_result_;
    impl_->owner = static_cast<HWND>(owner_handle);
    if (!impl_->Create()) {
        return DialogResult::none;
    }

    if (impl_->owner != nullptr && IsWindow(impl_->owner) != FALSE) {
        EnableWindow(impl_->owner, FALSE);
        impl_->window->CenterOnWindow(impl_->owner);
    }
    impl_->window->Show(SW_SHOWNORMAL);
    impl_->window->Activate();
    (void)impl_->window->RunModal();

    if (impl_->owner != nullptr && IsWindow(impl_->owner) != FALSE) {
        EnableWindow(impl_->owner, TRUE);
        SetForegroundWindow(impl_->owner);
    }
    impl_->owner = nullptr;

    const DialogResult value =
        impl_->result == DialogResult::none ? DialogResult::cancel : impl_->result;
    impl_->Shutdown();
    return value;
}

void Dialog::Close(DialogResult result) {
    if (impl_) {
        impl_->Finish(result);
    }
}

DialogResult Dialog::result() const noexcept {
    return impl_ != nullptr ? impl_->result : DialogResult::none;
}

bool Dialog::IsCreated() const noexcept {
    return impl_ != nullptr && impl_->created;
}

void* Dialog::NativeHandle() const noexcept {
    return impl_ != nullptr && impl_->window ? impl_->window->Handle() : nullptr;
}

DialogResult MessageBox::Show(Form& owner,
                              std::wstring title,
                              std::wstring message,
                              MessageBoxButtons buttons,
                              const MessageBoxLabels& labels) {
    Dialog dialog(std::move(title), 480, 210);
    auto content = std::make_unique<StackPanel>(Orientation::vertical, 10.0f);
    content->set_cross_align(CrossAlign::stretch);
    auto message_label =
        std::make_unique<Label>(std::move(message), FontRole::body, LabelStyle::secondary);
    message_label->set_wrap(true);
    content->add_child(std::move(message_label));
    dialog.SetContent(std::move(content));

    const auto add = [&dialog](const std::wstring& text, DialogResult result, ButtonStyle style) {
        dialog.AddButton(text, result, style);
    };

    switch (buttons) {
        case MessageBoxButtons::ok:
            add(labels.ok, DialogResult::ok, ButtonStyle::primary);
            dialog.SetDefaultResult(DialogResult::ok);
            break;
        case MessageBoxButtons::ok_cancel:
            add(labels.cancel, DialogResult::cancel, ButtonStyle::secondary);
            add(labels.ok, DialogResult::ok, ButtonStyle::primary);
            dialog.SetDefaultResult(DialogResult::ok);
            break;
        case MessageBoxButtons::yes_no:
            add(labels.no, DialogResult::no, ButtonStyle::secondary);
            add(labels.yes, DialogResult::yes, ButtonStyle::primary);
            dialog.SetDefaultResult(DialogResult::yes);
            break;
        case MessageBoxButtons::yes_no_cancel:
            add(labels.cancel, DialogResult::cancel, ButtonStyle::secondary);
            add(labels.no, DialogResult::no, ButtonStyle::secondary);
            add(labels.yes, DialogResult::yes, ButtonStyle::primary);
            dialog.SetDefaultResult(DialogResult::yes);
            break;
    }
    return dialog.ShowModal(owner);
}

}  // namespace cppui
