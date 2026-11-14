#include <cppui/theme/mode.hpp>

#include <windows.h>

namespace cppui {
namespace {

constexpr wchar_t kPersonalizeKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
constexpr wchar_t kAppsUseLightTheme[] = L"AppsUseLightTheme";

}  // namespace

bool system_prefers_dark() noexcept {
    DWORD use_light_theme = 1;
    DWORD size = sizeof(use_light_theme);
    const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
                                        kPersonalizeKey,
                                        kAppsUseLightTheme,
                                        RRF_RT_REG_DWORD,
                                        nullptr,
                                        &use_light_theme,
                                        &size);
    return status == ERROR_SUCCESS && use_light_theme == 0;
}

const ThemeTokens& resolve_theme(ThemeMode mode) noexcept {
    switch (mode) {
        case ThemeMode::dark:
            return dark_theme();
        case ThemeMode::system:
            return system_prefers_dark() ? dark_theme() : light_theme();
        case ThemeMode::light:
        default:
            return light_theme();
    }
}

}  // namespace cppui
