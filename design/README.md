# CppUI — визуальные концепты

Это дизайн-макеты для M0/v0.1+, а не финальная реализация.
Визуальное направление: **Signal Teal** по мотивам Build Manager Tray App.
Связанные документы: [PLAN.md](../PLAN.md) · [DESIGN_SYSTEM.md](../docs/DESIGN_SYSTEM.md) · [DESIGN_REFERENCES.md](../docs/DESIGN_REFERENCES.md) · [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md).

## Галерея контролов

Интерактивный макет всех запланированных контролов на пяти экранах:

- [**controls-gallery.html**](controls-gallery.html) — Light / Dark, навигация, density S/M/L и inline editing.
- [Обзор Light](controls-overview-light.png) — пять экранов + inline editing.
- [Обзор Dark](controls-overview-dark.png) — то же в тёмной теме.

### Полноразмерные экраны

| Экран | Light | Dark |
|---|---|---|
| 01 · Ввод и действия | [PNG](controls-input-light.png) | [PNG](controls-input-dark.png) |
| 02 · Выбор и списки | [PNG](controls-selection-light.png) | [PNG](controls-selection-dark.png) |
| 03 · Данные | [PNG](controls-data-light.png) | [PNG](controls-data-dark.png) |
| 04 · Рабочее место | [PNG](controls-workspace-light.png) | [PNG](controls-workspace-dark.png) |
| 05 · Макет и оверлеи | [PNG](controls-layout-light.png) | [PNG](controls-layout-dark.png) |

### Density и inline editing

Плотность переключается в тулбаре экрана «Рабочее место»: **S / M / L**. Она меняет высоту строк, контролов, отступы и размер шрифта внутри workspace.

| Состояние | Файл |
|---|---|
| Compact | [controls-workspace-compact.png](controls-workspace-compact.png) |
| Cozy / default | [controls-workspace-light.png](controls-workspace-light.png) |
| Comfortable | [controls-workspace-comfortable.png](controls-workspace-comfortable.png) |
| Inline editing, Light | [controls-workspace-edit.png](controls-workspace-edit.png) |
| Inline editing, Dark | [controls-workspace-edit-dark.png](controls-workspace-edit-dark.png) |

В интерактивном HTML по клику на ячейку статуса открывается inline-меню; в примере уже показан статус `Warning` в режиме редактирования.

### Что покрыто

- **01 · Ввод и действия:** Label, LinkLabel, Button (primary/secondary/ghost/danger, icon, dropdown, split, focus/disabled), CheckBox, RadioButton, ToggleSwitch, Slider, TextEdit, SearchEdit, PasswordEdit, SpinEdit, MaskedEdit, MemoEdit, ProgressBar, Loader, Validation, InfoBar, Image.
- **02 · Выбор и списки:** ComboBox, CheckedComboBox, DateEdit + Calendar, ListBox, CheckedListBox, ListView.
- **03 · Данные:** TreeList, GridView, PropertyGrid, VerticalGrid.
- **04 · Рабочее место:** TreeList + GridView + PropertyGrid в одном workspace; row states, inline editing, toolbar, bottom dock, density.
- **05 · Макет и оверлеи:** TabControl, GroupPanel/Panel, SplitContainer, PopupMenu, ToolTip, Dialog/MessageBox, BusyOverlay.
- **Оболочка:** Form, title bar, MenuBar, ToolBar/BarManager, StatusBar.

Не входят в эту галерею опциональные контролы поздних волн: `ColorPicker`, `Accordion`/`Expander`, `NavigationView`/`Breadcrumb`, `Toast`, `Skeleton`, `Docking`, `CodeEditor`.

## Форма: Light / Dark

- [form-preview.html](form-preview.html) — интерактивный макет формы: Light / Dark / System.
- [form-light.png](form-light.png) — светлая тема.
- [form-dark.png](form-dark.png) — тёмная тема.

Тёмную тему в HTML можно открыть сразу: `form-preview.html#dark`.

Фон вокруг окна на скриншотах — презентационная подложка, не часть фреймворка.

## Параметры формы

| Параметр | Значение |
|---|---|
| Окно | 1120 × 720 DIP, радиус 14 px |
| Title bar | 40 px |
| Status bar | 28 px |
| Системные кнопки | 46 × 40 px |
| Базовый шрифт | Segoe UI, 12–13 px |
| Акцент light | `#00705F` |
| Акцент dark | `#3BB39E` |
| Primary light | `#00705F` |
| Primary dark | `#006E5C` |
| Фон light | `#F1F6F3` / `#FFFFFF` |
| Фон dark | `#0C1211` / `#1A2120` |

## Параметры галереи

| Параметр | Значение |
|---|---|
| Окно | 1500 × 960 DIP, 5 экранов |
| Stage | 1580 × 1040 DIP, screenshot scale 1.5× |
| Sidebar | 230 DIP |
| Карточка | radius 12 DIP |
| Density compact | row 25 px, control 25 px, font 9.5 px |
| Density cozy | row 30 px, control 28 px, font 10.5 px |
| Density comfortable | row 36 px, control 32 px, font 11.5 px |
| Снимки | 2370 × 1560 PNG |

## Границы концепта

- Это визуальный концепт, не реализованный C++ framework.
- Density в галерее применена к workspace, а не ко всем страницам; в будущем это общий `DensityMode` фреймворка.
- Inline editing и меню статуса работают в HTML; в C++ это будет поведение `GridView`/`PropertyGrid`.
- Селектор темы в контенте — демонстрационный. В v0.1 тема переключается в title bar, контролы `RadioCard`/`SegmentedControl` появятся в v0.2.
- Контролы поздних волн отмечены бейджами `M0`, `v0.1`, `v0.2`, `v0.3`.
