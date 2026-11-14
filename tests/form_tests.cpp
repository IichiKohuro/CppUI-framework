#include <cppui/form.hpp>
#include <cppui/theme/mode.hpp>
#include <cppui/ui/density.hpp>
#include <cppui/ui/form.hpp>
#include <cppui/ui/widgets.hpp>

#include <windows.h>
#include <objbase.h>

#include <cstdlib>
#include <iostream>
#include <thread>

int main() {
    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        std::cerr << "FAIL: CoInitializeEx\n";
        return EXIT_FAILURE;
    }

    cppui::Form form(L"CppUI Form API test", 900, 580);
    if (!form.Create()) {
        std::cerr << "FAIL: Form::Create\n";
        return EXIT_FAILURE;
    }

    auto content = std::make_unique<cppui::StackPanel>(cppui::Orientation::vertical, 10.0f);
    content->set_padding(cppui::Insets{20.0f, 20.0f, 20.0f, 20.0f});
    content->add_child(std::make_unique<cppui::Label>(L"Form API",
                                                      cppui::FontRole::heading,
                                                      cppui::LabelStyle::primary));
    content->add_child(std::make_unique<cppui::Button>(L"Action",
                                                       cppui::ButtonStyle::primary));
    form.SetContent(std::move(content));

    if (form.status_bar() == nullptr) {
        std::cerr << "FAIL: status bar was not created\n";
        return EXIT_FAILURE;
    }

    bool theme_called = false;
    bool density_called = false;
    form.SetOnThemeChanged([&theme_called](cppui::ThemeMode mode) {
        theme_called = mode == cppui::ThemeMode::dark;
    });
    form.SetOnDensityChanged([&density_called](cppui::DensityMode mode) {
        density_called = mode == cppui::DensityMode::compact;
    });

    form.SetThemeMode(cppui::ThemeMode::dark);
    form.SetDensityMode(cppui::DensityMode::compact);
    if (!theme_called || !density_called) {
        std::cerr << "FAIL: theme/density callbacks were not fired\n";
        return EXIT_FAILURE;
    }

    const HWND hwnd = static_cast<HWND>(form.NativeHandle());
    if (hwnd == nullptr) {
        std::cerr << "FAIL: native handle\n";
        return EXIT_FAILURE;
    }

    std::thread closer([hwnd] {
        Sleep(200);
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    });

    form.Show();
    const int result = form.Run();
    closer.join();

    if (result != 0) {
        std::cerr << "FAIL: message loop result " << result << '\n';
        return EXIT_FAILURE;
    }
    if (form.IsCreated()) {
        std::cerr << "FAIL: Form still reports created after Run\n";
        return EXIT_FAILURE;
    }

    CoUninitialize();
    std::cout << "CppUI Form API tests passed.\n";
    return EXIT_SUCCESS;
}
