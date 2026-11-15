#include <cppui/ui/clipboard.hpp>

#include <windows.h>

#include <algorithm>
#include <string>

namespace cppui {

std::wstring ReadClipboardText() {
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) == FALSE) {
        return {};
    }
    if (OpenClipboard(nullptr) == FALSE) {
        return {};
    }

    std::wstring result;
    if (HANDLE handle = GetClipboardData(CF_UNICODETEXT); handle != nullptr) {
        if (const auto* data = static_cast<const wchar_t*>(GlobalLock(handle)); data != nullptr) {
            result.assign(data);
            GlobalUnlock(handle);
        }
    }
    CloseClipboard();
    return result;
}

bool WriteClipboardText(std::wstring_view text) {
    if (OpenClipboard(nullptr) == FALSE) {
        return false;
    }
    if (EmptyClipboard() == FALSE) {
        CloseClipboard();
        return false;
    }

    const std::size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) {
        CloseClipboard();
        return false;
    }

    void* destination = GlobalLock(memory);
    if (destination == nullptr) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    std::copy(text.begin(), text.end(), static_cast<wchar_t*>(destination));
    static_cast<wchar_t*>(destination)[text.size()] = L'\0';
    GlobalUnlock(memory);

    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true;
}

}  // namespace cppui
