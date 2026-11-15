// Verifies the public DPI-awareness check used by Form and by the app harness.
// The test process starts without per-monitor awareness unless the host
// manifest already declares it, so it can prove both directions.

#include <cppui/form.hpp>

#include <windows.h>

#include <cstdlib>
#include <iostream>

int main() {
    if (cppui::IsPerMonitorDpiAware()) {
        std::cout << "Process is already per-monitor DPI aware; skipping the negative check.\n";
    } else if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ==
               FALSE) {
        std::cerr << "FAIL: SetProcessDpiAwarenessContext failed with error "
                  << GetLastError() << '\n';
        return EXIT_FAILURE;
    }

    if (!cppui::IsPerMonitorDpiAware()) {
        std::cerr << "FAIL: IsPerMonitorDpiAware() did not observe PMv2\n";
        return EXIT_FAILURE;
    }

    std::cout << "CppUI DPI awareness tests passed.\n";
    return EXIT_SUCCESS;
}
