#pragma once

#include <memory>
#include <string>
#include <utility>

#include <cppui/theme/mode.hpp>
#include <cppui/ui/density.hpp>
#include <cppui/ui/widget.hpp>
#include <cppui/ui/widgets.hpp>

// windows.h defines MessageBox as a macro, which would otherwise break the
// MessageBox class below in every consumer that includes both headers.
#ifdef MessageBox
#undef MessageBox
#endif

namespace cppui {

class Form;

enum class DialogResult {
    none,
    ok,
    cancel,
    yes,
    no,
};

// Modal top-level window with the same custom chrome as Form. Buttons close
// the dialog with their DialogResult; the X button behaves as cancel.
class Dialog final {
public:
    Dialog(std::wstring title, int width_dip = 460, int height_dip = 220);
    ~Dialog();

    Dialog(const Dialog&) = delete;
    Dialog& operator=(const Dialog&) = delete;
    Dialog(Dialog&&) = delete;
    Dialog& operator=(Dialog&&) = delete;

    void SetContent(std::unique_ptr<Widget> content);
    void AddButton(std::wstring text,
                   DialogResult result,
                   ButtonStyle style = ButtonStyle::secondary);
    void SetDefaultResult(DialogResult result) noexcept { default_result_ = result; }

    void SetThemeMode(ThemeMode mode) noexcept { theme_mode_ = mode; }
    void SetDensityMode(DensityMode density) noexcept { density_mode_ = density; }

    DialogResult ShowModal(Form& owner);
    DialogResult ShowModal(void* owner_handle = nullptr);
    void Close(DialogResult result = DialogResult::cancel);

    [[nodiscard]] DialogResult result() const noexcept;
    [[nodiscard]] bool IsCreated() const noexcept;
    [[nodiscard]] void* NativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    DialogResult default_result_{DialogResult::none};
    ThemeMode theme_mode_{ThemeMode::light};
    DensityMode density_mode_{DensityMode::cozy};
};

enum class MessageBoxButtons {
    ok,
    ok_cancel,
    yes_no,
    yes_no_cancel,
};

struct MessageBoxLabels {
    std::wstring ok{L"OK"};
    std::wstring cancel{L"Cancel"};
    std::wstring yes{L"Yes"};
    std::wstring no{L"No"};
};

class MessageBox final {
public:
    [[nodiscard]] static DialogResult Show(
        Form& owner,
        std::wstring title,
        std::wstring message,
        MessageBoxButtons buttons = MessageBoxButtons::ok,
        const MessageBoxLabels& labels = {});
};

}  // namespace cppui
