#include <windows.h>

#undef MessageBox

#include <objbase.h>

#include <cppui/form.hpp>
#include <cppui/ui/dialog.hpp>
#include <cppui/ui/widgets.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {

HWND WaitForWindow(const wchar_t* title, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (HWND window = FindWindowW(nullptr, title); window != nullptr) {
            return window;
        }
        Sleep(20);
    }
    return nullptr;
}

}  // namespace

int main() {
    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        std::cerr << "FAIL: CoInitializeEx\n";
        return EXIT_FAILURE;
    }

    cppui::Form owner(L"CppUI Dialog owner", 900, 580);
    if (!owner.Create()) {
        std::cerr << "FAIL: owner form was not created\n";
        return EXIT_FAILURE;
    }

    bool passed = true;

    {
        cppui::Dialog dialog(L"CppUI v0.1 Dialog Test", 440, 200);
        auto content = std::make_unique<cppui::Label>(L"Dialog content");
        dialog.SetContent(std::move(content));
        dialog.AddButton(L"OK", cppui::DialogResult::ok, cppui::ButtonStyle::primary);
        dialog.SetDefaultResult(cppui::DialogResult::ok);

        std::thread closer([] {
            HWND window = WaitForWindow(L"CppUI v0.1 Dialog Test", std::chrono::seconds(8));
            for (int attempt = 0; attempt < 50 && window != nullptr &&
                                  IsWindow(window) != FALSE;
                 ++attempt) {
                Sleep(100);
                PostMessageW(window, WM_KEYDOWN, VK_RETURN, 0);
            }
        });
        const cppui::DialogResult result = dialog.ShowModal(owner);
        closer.join();
        if (result != cppui::DialogResult::ok) {
            std::cerr << "FAIL: dialog result was not OK\n";
            passed = false;
        }
        if (dialog.IsCreated()) {
            std::cerr << "FAIL: dialog still reports created\n";
            passed = false;
        }
        if (IsWindowEnabled(static_cast<HWND>(owner.NativeHandle())) == FALSE) {
            std::cerr << "FAIL: owner stayed disabled after dialog\n";
            passed = false;
        }
    }

    {
        cppui::Dialog dialog(L"CppUI v0.1 Mouse Test", 440, 200);
        dialog.AddButton(L"OK", cppui::DialogResult::ok, cppui::ButtonStyle::primary);
        dialog.SetDefaultResult(cppui::DialogResult::ok);

        std::thread clicker([] {
            HWND window = WaitForWindow(L"CppUI v0.1 Mouse Test", std::chrono::seconds(8));
            const LPARAM point = MAKELPARAM(378, 159);
            // The window exists at WM_CREATE, before the first layout. Repeat
            // the click until the dialog is gone so the test does not depend
            // on how fast the first frame is rendered.
            for (int attempt = 0; attempt < 50 && window != nullptr &&
                                  IsWindow(window) != FALSE;
                 ++attempt) {
                Sleep(150);
                PostMessageW(window, WM_MOUSEMOVE, 0, point);
                Sleep(30);
                PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, point);
                Sleep(30);
                PostMessageW(window, WM_LBUTTONUP, 0, point);
            }
        });
        const cppui::DialogResult result = dialog.ShowModal(owner);
        clicker.join();
        if (result != cppui::DialogResult::ok) {
            std::cerr << "FAIL: mouse click on dialog button did not return OK\n";
            passed = false;
        }
    }

    {
        std::thread closer([] {
            HWND window = WaitForWindow(L"CppUI v0.1 MessageBox Test", std::chrono::seconds(8));
            for (int attempt = 0; attempt < 50 && window != nullptr &&
                                  IsWindow(window) != FALSE;
                 ++attempt) {
                Sleep(100);
                PostMessageW(window, WM_CLOSE, 0, 0);
            }
        });
        const cppui::DialogResult result = cppui::MessageBox::Show(
            owner, L"CppUI v0.1 MessageBox Test", L"MessageBox uses the Dialog surface.",
            cppui::MessageBoxButtons::ok_cancel);
        closer.join();
        if (result != cppui::DialogResult::cancel) {
            std::cerr << "FAIL: message box close did not return cancel\n";
            passed = false;
        }
    }

    owner.Close();
    CoUninitialize();

    if (!passed) {
        return EXIT_FAILURE;
    }
    std::cout << "CppUI dialog tests passed.\n";
    return EXIT_SUCCESS;
}
