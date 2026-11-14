cmake_minimum_required(VERSION 3.28)

project(CppUI VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "CppUI is Windows-first. A Windows host is required.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "CppUI targets MSVC 2022/2026. Configure from a Visual Studio developer shell.")
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CppUI does not use C++20 modules yet. Keep module scanning out of the build
# until modules are actually needed.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CPPUI_BUILD_SAMPLES "Build the bootstrap sample" ON)
option(CPPUI_BUILD_TESTS "Build smoke tests" ON)
option(CPPUI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(cppui_configure_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
        if(CPPUI_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    endif()
endfunction()

add_library(cppui STATIC
    src/theme/theme.cpp
    src/theme/mode.cpp
    src/platform/win32/win32_window.cpp
    src/render/d2d/d2d_renderer.cpp
    src/ui/widget.cpp
    src/ui/widgets.cpp
    src/ui/form.cpp
    src/ui/host.cpp
    src/form.cpp
)
add_library(cppui::cppui ALIAS cppui)

target_compile_features(cppui PUBLIC cxx_std_20)
target_include_directories(cppui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(cppui
    PUBLIC
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00
)
target_link_libraries(cppui
    PUBLIC
        d3d11
        dxgi
        d2d1
        dwrite
        dwmapi
        user32
        gdi32
        ole32
        advapi32
        windowscodecs
)
cppui_configure_warnings(cppui)

if(CPPUI_BUILD_SAMPLES)
    add_executable(cppui_bootstrap WIN32
        samples/bootstrap/main.cpp
    )
    target_link_libraries(cppui_bootstrap PRIVATE cppui::cppui)
    target_include_directories(cppui_bootstrap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(cppui_bootstrap PROPERTIES
        OUTPUT_NAME "cppui-bootstrap"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:cppui_bootstrap>"
    )
    cppui_configure_warnings(cppui_bootstrap)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT cppui_bootstrap)
endif()

if(CPPUI_BUILD_TESTS)
    enable_testing()

    add_executable(cppui_tests tests/theme_tests.cpp)
    target_link_libraries(cppui_tests PRIVATE cppui::cppui)
    cppui_configure_warnings(cppui_tests)

    add_test(NAME cppui.theme COMMAND cppui_tests)

    add_executable(cppui_ui_tests tests/ui_tests.cpp)
    target_link_libraries(cppui_ui_tests PRIVATE cppui::cppui)
    cppui_configure_warnings(cppui_ui_tests)

    add_test(NAME cppui.ui COMMAND cppui_ui_tests)

    add_executable(cppui_form_tests tests/form_tests.cpp)
    target_link_libraries(cppui_form_tests PRIVATE cppui::cppui)
    cppui_configure_warnings(cppui_form_tests)

    add_test(NAME cppui.form COMMAND cppui_form_tests)
    set_tests_properties(cppui.form PROPERTIES TIMEOUT 30)

    if(CPPUI_BUILD_SAMPLES)
        add_test(NAME cppui.render_smoke COMMAND cppui_bootstrap --self-test)
        set_tests_properties(cppui.render_smoke PROPERTIES LABELS "render")

        add_test(NAME cppui.benchmark COMMAND cppui_bootstrap --benchmark)
        set_tests_properties(cppui.benchmark PROPERTIES LABELS "perf" TIMEOUT 120)

        add_test(
            NAME cppui.visual
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_test.ps1"
                    -Exe "$<TARGET_FILE:cppui_bootstrap>"
                    -Baseline "${CMAKE_CURRENT_SOURCE_DIR}/tests/baselines"
        )
        set_tests_properties(cppui.visual PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "visual"
            TIMEOUT 120
        )

        add_test(
            NAME cppui.dpi
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/dpi_matrix_test.ps1"
                    -Exe "$<TARGET_FILE:cppui_bootstrap>"
        )
        set_tests_properties(cppui.dpi PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "dpi"
            TIMEOUT 60
        )
    endif()
endif()
