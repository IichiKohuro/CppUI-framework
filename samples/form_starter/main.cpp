#include <windows.h>

#include <objbase.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <cppui/form.hpp>
#include <cppui/theme/mode.hpp>
#include <cppui/ui/containers.hpp>
#include <cppui/ui/dialog.hpp>
#include <cppui/ui/image.hpp>
#include <cppui/ui/indicators.hpp>
#include <cppui/ui/input.hpp>
#include <cppui/ui/popup.hpp>
#include <cppui/ui/widgets.hpp>

namespace {

constexpr wchar_t kWindowTitle[] = L"CppUI - Form Starter";
constexpr wchar_t kErrorTitle[] = L"CppUI - Form Starter";
constexpr int kInitialWidthDip = 1120;
constexpr int kInitialHeightDip = 800;
constexpr int kMinWidthDip = 960;
constexpr int kMinHeightDip = 640;

std::filesystem::path ExecutableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, ARRAYSIZE(buffer));
    if (length == 0) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer).parent_path();
}

std::unique_ptr<cppui::Label> Caption(std::wstring text) {
    return std::make_unique<cppui::Label>(std::move(text),
                                          cppui::FontRole::caption,
                                          cppui::LabelStyle::muted);
}

}  // namespace

class FormStarterApp final {
public:
    FormStarterApp() : form_(kWindowTitle, kInitialWidthDip, kInitialHeightDip, kMinWidthDip,
                             kMinHeightDip) {}

    int Run(HINSTANCE instance, int show_command, bool capture_visual) {
        (void)instance;
        (void)show_command;
        LoadAssets();

        form_.SetOnChromeAction([this] { ResetForm(); });
        if (!form_.Create()) {
            MessageBoxW(nullptr, L"Form Starter window creation failed.", kErrorTitle,
                        MB_OK | MB_ICONERROR);
            return 1;
        }

        form_.SetContent(BuildPage());
        form_.SetStatusText(L"Готово. Заполните профиль и проверьте валидацию.");

        if (capture_visual) {
            return CaptureVisual();
        }

        form_.Show();
        return form_.Run();
    }

private:
    void LoadAssets() {
        const std::filesystem::path assets = ExecutableDirectory() / L"assets";
        mark_ = cppui::Image::LoadFromFile((assets / L"cppui-mark.png").wstring());
        banner_ = cppui::Image::LoadFromFile((assets / L"form-banner.png").wstring());
    }

    std::unique_ptr<cppui::Widget> BuildPage() {
        auto page = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 18.0f);
        page->set_padding(cppui::Insets{28.0f, 24.0f, 28.0f, 24.0f});
        page->set_cross_align(cppui::CrossAlign::stretch);

        page->add_child(BuildHeader());

        auto body = std::make_unique<cppui::GridPanel>();
        body->set_columns({1.04f, 0.96f});
        body->set_gap(18.0f, 18.0f);
        body->add_cell(BuildProfile(), 0, 0);
        body->add_cell(BuildActivity(), 1, 0);
        page->add_child(std::move(body));

        auto footer = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 12.0f);
        footer->set_cross_align(cppui::CrossAlign::center);
        footer->add_child(std::make_unique<cppui::Label>(
            L"Tab / Shift+Tab — фокус · Enter / Space — действие · Esc закрывает popup",
            cppui::FontRole::caption,
            cppui::LabelStyle::muted));
        page->add_child(std::move(footer));

        auto scroll = std::make_unique<cppui::ScrollViewer>(std::move(page));
        scroll->set_padding(cppui::Insets{0.0f, 0.0f, 0.0f, 0.0f});
        return scroll;
    }

    std::unique_ptr<cppui::Widget> BuildHeader() {
        auto header = std::make_unique<cppui::GridPanel>();
        header->set_columns({1.0f, 0.0f});
        header->set_gap(12.0f, 0.0f);

        auto brand = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 14.0f);
        brand->set_cross_align(cppui::CrossAlign::center);

        auto logo = std::make_unique<cppui::PictureBox>(mark_);
        logo->set_preferred_size(cppui::Size{52.0f, 52.0f});
        logo->set_corner_radius(12.0f);
        brand->add_child(std::move(logo));

        auto title = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 2.0f);
        title->set_cross_align(cppui::CrossAlign::start);
        title->add_child(std::make_unique<cppui::Label>(L"Form Starter",
                                                        cppui::FontRole::title,
                                                        cppui::LabelStyle::primary));
        auto subtitle = std::make_unique<cppui::Label>(
            L"v0.1 · базовые контролы, диалоги и общий popup",
            cppui::FontRole::body,
            cppui::LabelStyle::secondary);
        subtitle->set_tooltip(L"Подсказка появляется при наведении через 600 мс");
        title->add_child(std::move(subtitle));
        brand->add_child(std::move(title));
        header->add_cell(std::move(brand), 0, 0);

        auto action_holder =
            std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 0.0f);
        action_holder->set_cross_align(cppui::CrossAlign::center);
        auto actions = std::make_unique<cppui::Button>(L"Действия", cppui::ButtonStyle::secondary);
        actions->set_icon(mark_);
        actions->set_tooltip(L"Popup-меню с командами профиля");
        actions_ = actions.get();
        actions->set_on_click([this] { ShowActionsMenu(); });
        action_holder->add_child(std::move(actions));
        header->add_cell(std::move(action_holder), 1, 0);

        return header;
    }

    std::unique_ptr<cppui::Widget> BuildProfile() {
        auto group = std::make_unique<cppui::GroupPanel>(L"Профиль");
        group->set_padding(cppui::Insets{18.0f, 16.0f, 18.0f, 18.0f});

        auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 7.0f);
        content->set_cross_align(cppui::CrossAlign::stretch);

        content->add_child(Caption(L"ИМЯ"));
        auto name = std::make_unique<cppui::TextEdit>(L"", L"Например, Анна Соколова");
        name_ = name.get();
        name_->set_tooltip(L"Обязательное поле. Ошибка показывается под полем.");
        name_->set_on_text_changed([this](const std::wstring&) {
            if (name_error_ != nullptr) {
                name_error_->set_text(L" ");
            }
            if (name_ != nullptr) {
                name_->clear_validation_error();
            }
        });
        content->add_child(std::move(name));
        auto name_error =
            std::make_unique<cppui::Label>(L" ", cppui::FontRole::caption, cppui::LabelStyle::danger);
        name_error_ = name_error.get();
        content->add_child(std::move(name_error));

        content->add_child(Caption(L"EMAIL"));
        auto email = std::make_unique<cppui::TextEdit>(L"", L"name@example.com");
        email_ = email.get();
        email_->set_tooltip(L"Должен содержать символ @ и точку в домене.");
        email_->set_on_text_changed([this](const std::wstring&) {
            if (email_error_ != nullptr) {
                email_error_->set_text(L" ");
            }
            if (email_ != nullptr) {
                email_->clear_validation_error();
            }
        });
        content->add_child(std::move(email));
        auto email_error =
            std::make_unique<cppui::Label>(L" ", cppui::FontRole::caption, cppui::LabelStyle::danger);
        email_error_ = email_error.get();
        content->add_child(std::move(email_error));

        content->add_child(Caption(L"УВЕДОМЛЕНИЯ"));
        auto checks = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 4.0f);
        auto notify = std::make_unique<cppui::CheckBox>(L"Присылать уведомления", true);
        notify->set_on_changed([this](cppui::CheckState state) {
            form_.SetStatusText(state == cppui::CheckState::checked
                                    ? L"Уведомления включены"
                                    : L"Уведомления выключены");
        });
        checks->add_child(std::move(notify));
        auto digest = std::make_unique<cppui::CheckBox>(L"Еженедельный дайджест", false);
        checks->add_child(std::move(digest));
        auto hints = std::make_unique<cppui::CheckBox>(L"Показывать подсказки", true);
        hints->set_tooltip(L"ToolTip включён для полей и кнопок");
        checks->add_child(std::move(hints));
        content->add_child(std::move(checks));

        content->add_child(Caption(L"ПЛОТНОСТЬ ИНТЕРФЕЙСА"));
        auto radios = std::make_unique<cppui::RadioGroup>(cppui::Orientation::vertical, 4.0f);
        auto compact = std::make_unique<cppui::RadioButton>(L"Компактная", false);
        compact_radio_ = compact.get();
        compact->set_on_changed([this](bool checked) {
            if (checked && !syncing_density_) {
                form_.SetDensityMode(cppui::DensityMode::compact);
                form_.SetStatusText(L"Плотность: Compact");
            }
        });
        auto cozy = std::make_unique<cppui::RadioButton>(L"Обычная", true);
        cozy_radio_ = cozy.get();
        cozy->set_on_changed([this](bool checked) {
            if (checked && !syncing_density_) {
                form_.SetDensityMode(cppui::DensityMode::cozy);
                form_.SetStatusText(L"Плотность: Cozy");
            }
        });
        auto comfortable = std::make_unique<cppui::RadioButton>(L"Свободная", false);
        comfortable_radio_ = comfortable.get();
        comfortable->set_on_changed([this](bool checked) {
            if (checked && !syncing_density_) {
                form_.SetDensityMode(cppui::DensityMode::comfortable);
                form_.SetStatusText(L"Плотность: Comfortable");
            }
        });
        radios->add_child(std::move(compact));
        radios->add_child(std::move(cozy));
        radios->add_child(std::move(comfortable));
        content->add_child(std::move(radios));

        content->add_child(Caption(L"ДЕЙСТВИЯ ПРОФИЛЯ"));
        auto actions = std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 8.0f);
        actions->set_cross_align(cppui::CrossAlign::center);
        auto save = std::make_unique<cppui::Button>(L"Проверить и сохранить",
                                                    cppui::ButtonStyle::primary);
        save->set_on_click([this] { ValidateAndSave(); });
        actions->add_child(std::move(save));
        auto dialog = std::make_unique<cppui::Button>(L"Диалог", cppui::ButtonStyle::secondary);
        dialog->set_on_click([this] { ShowCommentDialog(); });
        actions->add_child(std::move(dialog));
        auto reset = std::make_unique<cppui::Button>(L"Сброс", cppui::ButtonStyle::ghost);
        reset->set_on_click([this] { ResetForm(); });
        actions->add_child(std::move(reset));
        content->add_child(std::move(actions));

        content->add_child(std::make_unique<cppui::Label>(
            L"Валидация не сдвигает layout: строка ошибки всегда занимает место.",
            cppui::FontRole::caption,
            cppui::LabelStyle::muted));

        group->add_child(std::move(content));
        return group;
    }

    std::unique_ptr<cppui::Widget> BuildActivity() {
        auto group = std::make_unique<cppui::GroupPanel>(L"Активность");
        group->set_padding(cppui::Insets{18.0f, 16.0f, 18.0f, 18.0f});

        auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 10.0f);
        content->set_cross_align(cppui::CrossAlign::stretch);

        content->add_child(Caption(L"СИНХРОНИЗАЦИЯ"));
        auto progress = std::make_unique<cppui::ProgressBar>(0.68f);
        progress->set_show_text(true);
        content->add_child(std::move(progress));

        auto loader = std::make_unique<cppui::Loader>(L"Проверка обновлений…");
        loader->set_tooltip(L"Loader анимируется только пока видим");
        content->add_child(std::move(loader));

        content->add_child(Caption(L"ПОСЛЕДНИЙ РЕЛИЗ"));
        content->add_child(std::make_unique<cppui::Label>(
            L"v0.1 Form Starter", cppui::FontRole::heading, cppui::LabelStyle::primary));
        content->add_child(std::make_unique<cppui::Label>(
            L"TextEdit · CheckBox · RadioButton · ProgressBar · Loader",
            cppui::FontRole::caption,
            cppui::LabelStyle::secondary));

        auto banner = std::make_unique<cppui::PictureBox>(banner_);
        banner->set_preferred_size(cppui::Size{380.0f, 142.0f});
        banner->set_fit(cppui::ImageFit::cover);
        banner->set_corner_radius(10.0f);
        banner->set_tooltip(L"PictureBox: PNG, contain/cover/stretch, WIC loader");
        content->add_child(std::move(banner));

        content->add_child(Caption(L"ПОДСКАЗКА"));
        auto tip_actions =
            std::make_unique<cppui::StackPanel>(cppui::Orientation::horizontal, 8.0f);
        tip_actions->set_cross_align(cppui::CrossAlign::center);
        auto tip_button =
            std::make_unique<cppui::Button>(L"Наведите на меня", cppui::ButtonStyle::secondary);
        tip_button->set_tooltip(L"Это общий ToolTip поверх контента, без перехвата мыши.");
        tip_actions->add_child(std::move(tip_button));

        auto message = std::make_unique<cppui::Button>(L"MessageBox", cppui::ButtonStyle::ghost);
        message->set_on_click([this] { ShowMessageBox(); });
        tip_actions->add_child(std::move(message));
        content->add_child(std::move(tip_actions));

        content->add_child(std::make_unique<cppui::Label>(
            L"Изображения и диалоги используют тот же Painter и theme tokens.",
            cppui::FontRole::caption,
            cppui::LabelStyle::muted));

        group->add_child(std::move(content));
        return group;
    }

    void SetStatus(std::wstring text) {
        form_.SetStatusText(std::move(text));
        form_.Invalidate();
    }

    void ShowActionsMenu() {
        if (form_.popups() == nullptr) {
            return;
        }
        if (form_.popups()->is_open()) {
            form_.popups()->close();
            return;
        }

        auto menu = std::make_unique<cppui::PopupMenu>(250.0f);
        menu->add_item(L"Заполнить пример", [this] {
            if (name_ != nullptr) {
                name_->set_text(L"Анна Соколова");
            }
            if (email_ != nullptr) {
                email_->set_text(L"anna@cppui.dev");
            }
            SetStatus(L"Пример профиля заполнен");
        }, L"Ctrl+E");
        menu->add_item(L"Сбросить профиль", [this] { ResetForm(); }, L"Ctrl+R");
        menu->add_separator();
        menu->add_item(L"О программе", [this] {
            SetStatus(L"CppUI v0.1 Form Starter");
        });
        menu->add_item(L"Недоступная команда", {}, {}, false);

        const cppui::Rect anchor =
            actions_ != nullptr ? actions_->bounds() : cppui::Rect{320.0f, 90.0f, 0.0f, 0.0f};
        form_.popups()->open(std::move(menu),
                             cppui::Point{anchor.x, anchor.bottom() + 4.0f});
    }

    void ShowCommentDialog() {
        cppui::Dialog dialog(L"Новый комментарий", 520, 310);
        auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 8.0f);
        content->set_cross_align(cppui::CrossAlign::stretch);
        content->add_child(std::make_unique<cppui::Label>(
            L"Комментарий к профилю", cppui::FontRole::body, cppui::LabelStyle::primary));
        auto edit = std::make_unique<cppui::TextEdit>(L"", L"Введите текст…");
        edit->set_on_text_changed([this](const std::wstring& value) { dialog_text_ = value; });
        content->add_child(std::move(edit));
        content->add_child(std::make_unique<cppui::Label>(
            L"Диалог блокирует владельца и возвращает DialogResult.",
            cppui::FontRole::caption,
            cppui::LabelStyle::muted));

        dialog.SetContent(std::move(content));
        dialog.AddButton(L"Отмена", cppui::DialogResult::cancel, cppui::ButtonStyle::secondary);
        dialog.AddButton(L"Сохранить", cppui::DialogResult::ok, cppui::ButtonStyle::primary);
        dialog.SetDefaultResult(cppui::DialogResult::ok);

        const cppui::DialogResult result = dialog.ShowModal(form_);
        if (result == cppui::DialogResult::ok) {
            SetStatus(dialog_text_.empty() ? L"Диалог закрыт без текста" : L"Комментарий сохранён");
        } else {
            SetStatus(L"Диалог отменён");
        }
    }

    void ShowMessageBox() {
        const cppui::MessageBoxLabels labels{L"OK", L"Отмена", L"Да", L"Нет"};
        const cppui::DialogResult result = cppui::MessageBox::Show(
            form_,
            L"Подтверждение",
            L"Сохранить изменения профиля?",
            cppui::MessageBoxButtons::yes_no_cancel,
            labels);
        switch (result) {
            case cppui::DialogResult::yes:
                SetStatus(L"MessageBox: Да");
                break;
            case cppui::DialogResult::no:
                SetStatus(L"MessageBox: Нет");
                break;
            default:
                SetStatus(L"MessageBox: отменено");
                break;
        }
    }

    void ValidateAndSave() {
        bool valid = true;
        if (name_ == nullptr || name_->text().empty()) {
            if (name_ != nullptr) {
                name_->set_validation_error(L"Укажите имя");
            }
            if (name_error_ != nullptr) {
                name_error_->set_text(L"Укажите имя");
            }
            valid = false;
        }
        const std::wstring email = email_ != nullptr ? email_->text() : std::wstring{};
        if (email.empty() || email.find(L'@') == std::wstring::npos ||
            email.find(L'.') == std::wstring::npos) {
            if (email_ != nullptr) {
                email_->set_validation_error(L"Укажите корректный email");
            }
            if (email_error_ != nullptr) {
                email_error_->set_text(L"Укажите корректный email");
            }
            valid = false;
        }

        if (!valid) {
            SetStatus(L"Проверьте поля: валидация не меняет размер формы");
            return;
        }

        SetStatus(L"Профиль сохранён");
        const cppui::MessageBoxLabels labels{L"OK", L"Отмена", L"Да", L"Нет"};
        (void)cppui::MessageBox::Show(form_,
                                      L"Профиль сохранён",
                                      L"Данные прошли локальную валидацию.",
                                      cppui::MessageBoxButtons::ok,
                                      labels);
    }

    void ResetForm() {
        if (name_ != nullptr) {
            name_->set_text({});
            name_->clear_validation_error();
        }
        if (email_ != nullptr) {
            email_->set_text({});
            email_->clear_validation_error();
        }
        if (name_error_ != nullptr) {
            name_error_->set_text(L" ");
        }
        if (email_error_ != nullptr) {
            email_error_->set_text(L" ");
        }
        SetStatus(L"Форма сброшена");
    }

    void SyncDensityRadios(cppui::DensityMode density) {
        syncing_density_ = true;
        cppui::RadioButton* target = cozy_radio_;
        if (density == cppui::DensityMode::compact) {
            target = compact_radio_;
        } else if (density == cppui::DensityMode::comfortable) {
            target = comfortable_radio_;
        }
        if (target != nullptr) {
            target->set_checked(true);
        }
        syncing_density_ = false;
    }

    int CaptureVisual() {
        std::error_code error;
        std::filesystem::create_directories(L"visual-output", error);

        const auto capture = [this](cppui::ThemeMode theme,
                                    cppui::DensityMode density,
                                    const wchar_t* name) {
            form_.SetThemeMode(theme);
            form_.SetDensityMode(density);
            SyncDensityRadios(density);
            form_.SetStatusText(L"Готово: Form Starter");
            return form_.CapturePng(name);
        };

        if (!capture(cppui::ThemeMode::light, cppui::DensityMode::cozy,
                     L"visual-output/v01-light-cozy.png") ||
            !capture(cppui::ThemeMode::dark, cppui::DensityMode::cozy,
                     L"visual-output/v01-dark-cozy.png") ||
            !capture(cppui::ThemeMode::light, cppui::DensityMode::compact,
                     L"visual-output/v01-light-compact.png")) {
            MessageBoxW(nullptr, L"Form Starter visual capture failed.", kErrorTitle,
                        MB_OK | MB_ICONERROR);
            return 1;
        }

        form_.SetThemeMode(cppui::ThemeMode::light);
        form_.SetDensityMode(cppui::DensityMode::cozy);
        SyncDensityRadios(cppui::DensityMode::cozy);
        form_.SetStatusText(L"Готово: Form Starter");
        ShowActionsMenu();
        if (!form_.CapturePng(L"visual-output/v01-light-popup.png")) {
            MessageBoxW(nullptr, L"Popup visual capture failed.", kErrorTitle,
                        MB_OK | MB_ICONERROR);
            return 1;
        }
        if (form_.popups() != nullptr) {
            form_.popups()->close();
        }
        return 0;
    }

    cppui::Form form_;
    std::shared_ptr<cppui::Image> mark_;
    std::shared_ptr<cppui::Image> banner_;
    cppui::TextEdit* name_{};
    cppui::TextEdit* email_{};
    cppui::Label* name_error_{};
    cppui::Label* email_error_{};
    cppui::Button* actions_{};
    cppui::RadioButton* compact_radio_{};
    cppui::RadioButton* cozy_radio_{};
    cppui::RadioButton* comfortable_radio_{};
    bool syncing_density_{false};
    std::wstring dialog_text_;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        MessageBoxW(nullptr, L"COM initialization failed.", kErrorTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    const bool capture = std::wstring_view(GetCommandLineW()).find(L"--capture-visual") !=
                         std::wstring_view::npos;

    FormStarterApp app;
    const int result = app.Run(instance, show_command, capture);

    CoUninitialize();
    return result;
}
