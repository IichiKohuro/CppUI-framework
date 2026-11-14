# ARCHITECTURE — техническая архитектура CppUI

> Статус: Draft · Phase 5 hardening · 14.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md), [ROADMAP.md](ROADMAP.md), [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), [QUALITY.md](QUALITY.md), [PHASE0.md](PHASE0.md), [PHASE1.md](PHASE1.md), [PHASE2.md](PHASE2.md), [PHASE3.md](PHASE3.md), [PHASE4.md](PHASE4.md), [PHASE5.md](PHASE5.md).

## 1. Ограничения и цель

CppUI создаётся как **Windows-first** фреймворк. Главная цель архитектуры — узкое, предсказуемое ядро, на котором форма, кнопки и будущие контролы работают без дублирования платформенной и рендер-логики.

Ограничения:

- Windows 11 x64 — приоритет; Windows 10 — best effort.
- C++20, MSVC 2022/2026.
- Один UI-поток. Рендер изначально в UI-потоке.
- Без сторонних runtime-зависимостей, кроме Windows SDK.
- Публичный ABI не стабилизируется до 1.0.
- Cross-platform и docking не входят в первый этап.

## 2. Стек

| Слой | Выбор | Причина |
|---|---|---|
| Окно | Win32 + DWM | Нужен полный контроль над chrome, Snap Layouts и DPI |
| Графика | D3D11 + DXGI flip-model | Низкая задержка, контроль resize и vsync |
| 2D | Direct2D | Векторная графика, скругления, градиенты, аппаратное ускорение |
| Текст | DirectWrite | Качественный текст, fallback-шрифты, DPI |
| Сборка | CMake; Ninja в CI, VS generator локально | Быстрые сборки и корректный dep tracking на локализованной MSVC |
| Тесты | smoke CTest сейчас, Catch2 позже | Не тянет runtime; Catch2 остаётся test-only |
| Компилятор | MSVC 2022/2026 | Основная платформа; clang-cl позже |

## 3. Слои

```text
┌─────────────────────────────────────────────────────┐
│ App / demo / user code                              │
├─────────────────────────────────────────────────────┤
│ Widgets: Form, Label, Button, TextEdit, Panel, ...  │
├─────────────────────────────────────────────────────┤
│ Services: Command, DataSource, Validation, Theme    │
│           Focus, Popup, Scroll, Accessibility       │
├──────────────────────────┬──────────────────────────┤
│ Platform abstraction     │ Render abstraction       │
│ Win32 / DWM / DPI        │ D3D11 / DXGI / D2D       │
├──────────────────────────┴──────────────────────────┤
│ Core: geometry, layout, events, painting contracts   │
└─────────────────────────────────────────────────────┘
```

Правила:

- `core` не включает `windows.h` и не знает про HWND.
- `platform` не занимается отрисовкой виджетов.
- `render` не знает про бизнес-логику контролов.
- Виджеты не работают с HWND напрямую; доступ к платформе — через сервисы.
- Публичные заголовки не содержат COM-типов и внутренних структур.

## 4. Публичный API

Принципы:

- RAII и явное владение.
- `Pimpl` для публичных классов, чтобы не светить реализацию и уменьшить пересборку.
- Callback-и через `std::function` с документированным lifetime.
- Строки на границе API — UTF-8; внутри Win32/DirectWrite — UTF-16.
- Ошибки инициализации возвращаются явно; в paint/event path исключения не используются как штатный механизм.
- Escape hatch вроде `nativeHandle()` допускается, но не обязателен для обычного кода.
- Пространство имён: `cppui`. Публичные заголовки: `include/cppui/...`.

Эскиз API (не финальные сигнатуры):

```cpp
#include <cppui/cppui.hpp>

int main() {
    cppui::Application app;

    auto& form = app.createForm({
        .title   = "CppUI Gallery",
        .size    = {1120, 720},
        .minSize = {720, 480},
        .theme   = cppui::ThemeMode::System,
    });

    form.titleBar().addAction({
        .icon    = cppui::Icon::Theme,
        .tooltip = "Переключить тему",
        .onClick = [&] { form.toggleTheme(); },
    });

    form.content().emplace<cppui::Label>("Оформление", cppui::TextStyle::Heading);
    form.content().emplace<cppui::Label>(
        "Светлая и тёмная темы для формы и контролов.");

    auto& save = form.content().emplace<cppui::Button>("Сохранить");
    save.setKind(cppui::ButtonKind::Primary);
    save.onClick([] { /* ... */ });

    form.statusBar().setLeft("Готово");
    form.show();
    return app.run();
}
```

В реальном API будут варианты layout-контейнеров, стили и более удобный builder. Главное — не терять владение и явные зависимости.

## 5. Окно и Form

### 5.1 Модель

- `Win32Window` — текущий внутренний слой top-level окна с message loop и callbacks.
- Кастомный chrome: `WM_NCCALCSIZE` отдаёт клиенту всю площадь, `WM_NCHITTEST` размечает caption, кнопки и resize-border; состояние hover/pressed передаётся в renderer. Кнопки завершаются по `WM_NCLBUTTONUP` без `SetCapture`, с fallback на `WM_LBUTTONUP`.
- При maximized клиентская область вписывается в `rcWork` с учётом DPI-метрик системной рамки.
- DWM: rounded corners и frame extension для тени.
- `Window` как публичная база для top-level окна появится позже.
- `Form` — публичное главное окно.
- `Dialog` — модальное/немодальное окно с owner, focus return и отдельной политикой закрытия.
- `MessageBox` — готовый диалог поверх `Dialog`.
- `Form` владеет тремя зонами: `TitleBar`, `ContentHost`, `StatusBar`.
- Дополнительные действия title bar — конфигурируемые команды, а не жёстко зашитые кнопки.

### 5.2 Custom chrome

Планируемый подход:

- `WS_OVERLAPPEDWINDOW`-подобные стили с `WS_THICKFRAME`, `WS_MINIMIZEBOX`, `WS_MAXIMIZEBOX`, `WS_SYSMENU`.
- `WM_NCCALCSIZE` — убрать стандартный caption, но сохранить frame для resize и тени.
- `WM_NCHITTEST` — вернуть `HTLEFT/HTRIGHT/HTTOP/...` для краёв, `HTCAPTION` для пустой зоны заголовка, `HTCLIENT` для интерактивных элементов, `HTMAXBUTTON` для кнопки maximize (нужно для Windows 11 Snap Layouts).
- `WM_GETMINMAXINFO` — минимальный размер и максимизация в рабочую область монитора с учётом taskbar.
- `WM_CLOSE` — вызвать `Form::onClose`; при `false` не закрывать окно.
- Системное меню — правый клик по заголовку и Alt+Space; minimize/maximize/restore/close.
- DWM: `DWMWA_USE_IMMERSIVE_DARK_MODE`, `DWMWA_WINDOW_CORNER_PREFERENCE`, при необходимости — `DWMWA_BORDER_COLOR`; все атрибуты проверять на поддержку ОС.
- DPI: Per-Monitor V2 через manifest, обработка `WM_DPICHANGED`, работа с suggested rect.
- Maximized state: контент не должен уезжать под границы экрана; добавлять корректный inset.
- Активное/неактивное состояние окна — отдельный визуальный state.
- Иконка приложения и AppUserModelID — для taskbar.

Hybrid hit-testing (client-drawn кнопки + non-client поведение Windows) — главный технический риск. До реализации виджетов нужен отдельный spike, проверяющий:

- drag и double-click на пустом заголовке;
- hover/press на дополнительных кнопках;
- Snap Layouts на maximize;
- resize со всех краёв;
- системное меню и Alt+F4;
- поведение при 100/150/200% DPI и двух мониторах.

### 5.3 Диалоги

`Dialog` использует тот же chrome и content host, но:

- блокирует owner-окно при modal (`ShowDialog`);
- возвращает фокус владельцу;
- имеет собственные `onClose`/`DialogResult`;
- не создаёт отдельную копию кнопок окна.

Механика modal loop должна быть изолирована от `Form`, чтобы не превращать форму в набор режимов.

## 6. Рендер

### 6.1 Pipeline

- Создание D3D11 device + DXGI swap chain для HWND.
- `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`, 2–3 буфера.
- D2D device context создаётся из DXGI device; back buffer становится render target.
- DirectWrite используется для текста и измерений; кеши text layouts и font faces.
- VSync по умолчанию; Present по invalidation, без busy-loop.
- WARP fallback, если аппаратный D3D11 недоступен (RDP/VM/слабый GPU).
- Обработка `DXGI_ERROR_DEVICE_REMOVED`/`DXGI_ERROR_DEVICE_RESET`/`D2DERR_RECREATE_TARGET`: пересоздать device и ресурсы, не теряя состояние UI.
- При resize — `ResizeBuffers` и пересоздание bitmap target; не рисовать в старый back buffer.

> Phase 1 baseline: реализованы D3D11/DXGI flip/D2D/DWrite, WARP, resize, DPI callbacks, `D2DERR_RECREATE_TARGET` и полное пересоздание device/swap chain/ресурсов при `DEVICE_REMOVED`/`DEVICE_RESET`/`DEVICE_HUNG`. Present выполняется по invalidation, без busy-loop.

### 6.2 Кадры

- UI поток обрабатывает сообщения и при необходимости рисует кадр.
- Invalidation: изменение состояния/размера/анимации помечает области грязными.
- Анимации запрашивают следующий кадр через frame scheduler; в покое рендер не крутится.
- В MVP допустим полный repaint окна при изменении; позже — dirty regions/layers.
- Не допускать лишних аллокаций и создания кистей/форматов в горячем пути.

### 6.3 Качество графики

- Геометрия и 1 px линии выравниваются по пиксельной сетке.
- Цветовые токены, а не hard-coded brushes.
- Скругления, границы и фокус-кольца рисуются через общий стиль.
- Mica/acrylic и DirectComposition — post-MVP, отдельным решением.
- Для окон без прозрачности использовать opaque swap chain; это быстрее и предсказуемее.

## 7. Layout и виджеты

### 7.1 Базовый контракт

```cpp
class Widget {
public:
    virtual Size measure(const Constraints& constraints) = 0;
    virtual void arrange(const Rect& finalRect) = 0;
    virtual void paint(PaintContext& context) = 0;
    // focus, event routing, accessibility hooks
};
```

- Layout в DIP; рендер масштабирует под DPI.
- Два прохода: measure и arrange.
- Первые контейнеры: `Panel`, `GroupPanel`, `StackPanel`, `GridPanel`.
- `ScrollViewer` добавляет viewport/extent и позже — virtualization.
- Виджеты не рисуют вне clip-области.
- Контролы не хранят HWND, brushes или D2D-объекты; для рисования используют `PaintContext`.

### 7.2 События и фокус

- Hit-testing по дереву с учётом z-order, clip и disabled state.
- Pointer capture во время drag.
- Focus manager: Tab/Shift+Tab, стрелки в группах, Enter/Space, Esc для popup/dialog.
- `disabled` и `readonly` — состояния виджета, а не отсутствие обработчика.
- Мышь и клавиатура должны приводить к одинаковому результату.
- Tooltip и popup получают события через общий popup-менеджер.

### 7.3 Сервисы

**Command/Action.** Одна команда хранит `enabled/checked/text/icon/shortcut` и умеет выполниться из Button, Menu Item или shortcut. `BarManager` — менеджер команд и баров, а не самостоятельная копия логики.

**Scroll/Virtualization.** `ScrollViewer` предоставляет viewport, scrollbar и контракт для виртуализированных списков. Реализация item recycling — обязательное условие для `TreeList`/`GridView`.

**Popup.** Единый `PopupHost` управляет позиционированием, z-order, dismiss по Esc/клику вне, фокусом и ограничением размеров. На нём строятся `PopupMenu`, `ComboBox`, `DropDownButton`, `ToolTip` и редакторы `PropertyGrid`.

**Validation.** Ошибки привязаны к полю/контролу, отображаются через `ErrorProvider`/inline message, не ломая layout. Ошибки не должны быть исключениями в paint path.

**Data.** До `TreeList`/`GridView`/`PropertyGrid` фиксируются:

- `DataSource`/`ListAdapter` с количеством, доступом к элементу, уведомлениями;
- selection model;
- column model;
- virtualization contract;
- `PropertyDescriptor` для `PropertyGrid` (в C++ нет рефлексии).

**Accessibility.** В базовом `Widget` предусмотрены accessible name/role/value и bounds. UIA-провайдер добавляется поэтапно; клавиатурная доступность — с первого релиза.

## 8. Темы

- Семантические токены: фон, поверхность, границы, текст, акцент, danger, focus, disabled.
- Режимы: Light, Dark, System.
- Системная тема читается из Windows и обновляется при `WM_SETTINGCHANGE`; приложение может переопределить режим.
- Смена темы не пересоздаёт окно и не требует перезапуска.
- Все контролы используют токены; прямые цвета в коде контрола запрещены.
- Токены и визуальные размеры — в [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md).

## 9. Потоки, владение и ошибки

- Все UI-операции и работа с HWND/D2D — только в UI-потоке.
- Фоновые задачи возвращают результат в UI-поток через dispatcher.
- Render thread не вводится до появления реальной необходимости; текущая модель — UI thread render.
- Владение виджетами — деревом формы; родитель удаляет детей.
- COM-объекты — через RAII/`ComPtr`.
- Ошибки инициализации: явный error/result; ошибки отрисовки не должны приводить к падению.
- Callback’и должны быть безопасны к удалению виджета; правила lifetime документируются в API.

## 10. Структура проекта

Фактическая структура Phase 3:

```text
CppUI-framework/
├─ include/cppui/geometry.hpp
├─ include/cppui/theme/tokens.hpp
├─ include/cppui/theme/mode.hpp
├─ include/cppui/ui/painter.hpp
├─ include/cppui/ui/density.hpp
├─ include/cppui/ui/widget.hpp
├─ include/cppui/ui/widgets.hpp
├─ include/cppui/form.hpp
├─ include/cppui/ui/form.hpp
├─ include/cppui/ui/host.hpp
├─ src/form.cpp
├─ src/theme/theme.cpp
├─ src/theme/mode.cpp
├─ src/chrome/chrome_state.hpp
├─ src/platform/win32/win32_window.hpp
├─ src/platform/win32/win32_window.cpp
├─ src/render/d2d/d2d_renderer.hpp
├─ src/render/d2d/d2d_renderer.cpp
├─ src/ui/widget.cpp
├─ src/ui/widgets.cpp
├─ src/ui/form.cpp
├─ src/ui/host.cpp
├─ samples/bootstrap/main.cpp
├─ tests/theme_tests.cpp
├─ tests/ui_tests.cpp
├─ tests/form_tests.cpp
├─ tests/visual_test.ps1
├─ tests/dpi_matrix_test.ps1
├─ tests/baselines/
├─ docs/PHASE1.md
├─ docs/PHASE2.md
├─ docs/PHASE3.md
├─ docs/PHASE4.md
├─ docs/PHASE5.md
├─ .github/workflows/windows.yml
├─ tools/build.ps1
├─ CMakeLists.txt
└─ CMakePresets.json
```

Целевая структура после появления ядра:

```text
CppUI-framework/
├─ README.md
├─ PLAN.md
├─ CONTROL_BACKLOG.md
├─ CMakeLists.txt
├─ CMakePresets.json
├─ cmake/
├─ include/cppui/
│  ├─ cppui.hpp
│  ├─ core/
│  ├─ widgets/
│  └─ app/
├─ src/
│  ├─ core/
│  ├─ services/
│  ├─ platform/win32/
│  ├─ render/d2d/
│  └─ widgets/
├─ samples/
│  └─ gallery/
├─ tests/
│  ├─ unit/
│  ├─ integration/
│  └─ visual/
├─ design/
└─ docs/
```

`design_chats/` — внешние дизайн-референсы, не часть сборки.

### Build targets

- `cppui` — статическая библиотека.
- `cppui_gallery` — демо-приложение.
- `cppui_tests` — unit/integration tests.

CMake-опции:

```text
CPPUI_BUILD_SAMPLES=ON
CPPUI_BUILD_TESTS=ON
CPPUI_ENABLE_ASAN=OFF
```

Warnings: `/W4 /permissive- /Zc:__cplusplus`; в CI — `/WX` для собственного кода.

## 11. Тестирование и качество

- Unit: layout, geometry, theme, state machines, command, validation.
- Integration: создание hidden window, сообщения Windows, DPI/theme changes, закрытие.
- Visual: offscreen-рендер в PNG и сравнение с эталоном; допуск на сглаживание.
- Manual QA: chrome, Snap Layouts, multi-monitor, RDP/WARP, auto-hide taskbar.
- Performance: startup, idle CPU, input-to-photon, resize FPS, память.
- CI: windows-latest, MSVC, CMake, tests, format check, artifact demo.

Подробно — в [QUALITY.md](QUALITY.md).

## 12. Известные ограничения

- Windows 10 не имеет части Windows 11 visual features (rounded corners, Snap Layouts); нужен graceful fallback.
- Кастомный chrome требует постоянного тестирования при обновлениях Windows.
- Direct2D/DirectWrite рисуют по-разному на разных DPI; нужны visual snapshots с допуском.
- UIA для полностью кастомных контролов требует отдельной работы.
- Нет стабильного ABI до 1.0; распространять предпочтительно исходниками/статической библиотекой.
- IME, BiDi и сложные текстовые сценарии — отдельная зона тестирования.
- `PropertyGrid` без рефлексии ограничен явными дескрипторами.

## 13. Открытые технические решения

1. Точная схема hybrid hit-testing для клиентских кнопок title bar и maximize/Snap Layouts.
2. Нужен ли waitable swap chain с frame latency в M0 или достаточно Present(1, 0).
3. Формат data adapter и нужен ли observable-based binding.
4. Степень поддержки high contrast и когда включать полный UIA.
5. Дистрибуция после 1.0: source-only или DLL с C ABI.
6. Нужен ли отдельный render thread в будущем.

Решения ведутся в [DECISIONS.md](DECISIONS.md).
