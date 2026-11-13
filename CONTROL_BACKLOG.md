# CONTROL_BACKLOG — каталог контролов и волны реализации

> Статус: Draft · 14.11.2026.
> Связанные документы: [PLAN.md](PLAN.md), [docs/ROADMAP.md](docs/ROADMAP.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/DESIGN_SYSTEM.md](docs/DESIGN_SYSTEM.md).

## 1. Назначение

Этот документ фиксирует полный планируемый набор UI-элементов, их тип, зависимости и релизную волну. Он не означает, что всё перечисленное войдёт в первый релиз.

В CppUI есть четыре разные категории сущностей, которые нельзя проектировать одинаково:

1. **Листовые контролы** — `Button`, `TextEdit`, `CheckBox`, `ProgressBar`.
2. **Контейнеры и навигация** — `Panel`, `GroupPanel`, `TabControl`, `SplitContainer`.
3. **Popups и overlays** — `PopupMenu`, `ToolTip`, `Dialog`, `BusyOverlay`.
4. **Сервисы и модели** — `Command`, `DataSource`, `Validation`, `PropertyDescriptor`.

Если смешать их в одном списке «контролов», позже появятся дублирующиеся реализации скролла, popup и команд.

## 2. Легенда волн

| Волна | Смысл |
|---|---|
| **Foundation** | Внутренняя инфраструктура, без которой нельзя строить контролы |
| **M0** | Technical Preview: форма, кнопка, темы |
| **v0.1** | Form Starter: минимальный полезный набор для реальной формы |
| **v0.2** | Input & Selection: поля ввода, списки, табы, сплиттеры |
| **v0.3** | Data Controls: таблицы, деревья, свойства, bar-менеджер |
| **v1.0** | Стабилизация, доступность, дополнительные контролы |
| **post-1.0** | Крупные расширения, не входящие в первый стабильный релиз |
| **TBD** | Решение о необходимости или сроках ещё не принято |

## 3. Полный каталог

### 3.1 Окна, контейнеры и навигация

| Элемент | Тип | Волна | Примечание |
|---|---|---|---|
| `Form` | top-level window | M0 | Кастомный title bar, дополнительные действия, статус-бар, resize, min/max/close |
| `Dialog` | modal/non-modal window | v0.1 | Возврат фокуса, owner window, modal loop |
| `MessageBox` | готовый диалог | v0.1 | Строится поверх `Dialog`; локализуемый набор кнопок |
| `Panel` | базовый контейнер | v0.1 | Клиппинг, вложенные виджеты, фон/граница |
| `GroupPanel` | контейнер с заголовком | v0.1 | Наследует `Panel`, заголовок и опциональное сворачивание позже |
| `StackPanel` | layout-контейнер | v0.1 | Вертикальная/горизонтальная укладка, gap, stretch |
| `GridPanel` | layout-контейнер | v0.1 | Строки/колонки, min/max; не путать с `GridView` |
| `ScrollViewer` | контейнер прокрутки | v0.1 | Общий для `MemoEdit`, списков, форм |
| `ScrollBar` | контрол прокрутки | v0.1 | Вертикальный/горизонтальный, темизация, drag |
| `SplitContainer` | двухпанельный контейнер | v0.2 | Перетаскиваемый splitter, min size панелей, cursor feedback |
| `TabControl` | навигация | v0.2 | Страницы, клавиатура, focus scope, overflow tabs позже |
| `StatusBar` | статус формы | M0 | Пока часть `Form`; публичный API для левой/правой зоны |
| `MenuBar` | главное меню | v0.3 | Часть `BarManager`; зависит от `Command` и `PopupMenu` |
| `ToolBar` | панель команд | v0.3 | Часть `BarManager`; overflow items — позже |
| `BarManager` | менеджер команд и баров | v0.3 | Не листовой контрол: управляет `Command`, барами, shortcuts |
| `PopupMenu` | контекстное/выпадающее меню | v0.1 | Общий `PopupHost`, `Command`, keyboard navigation |
| `ToolTip` | подсказка | v0.1 | Общий popup-механизм, задержка, доступное имя |
| `Toast` | всплывающее уведомление | v1.0 | Позиционирование, очередь, таймер |
| `InfoBar` | встроенное уведомление | v1.0 | Может быть реализован как стиль `Panel` |
| `NavigationView` / `Sidebar` | навигация приложения | v1.0 | Опционально, после базовых контейнеров |
| `Breadcrumb` | навигационная цепочка | v1.0 | Опционально |
| `Docking` | докинг панелей | post-1.0 | Крупная подсистема, отдельный проект |
| `Ribbon` | ленточный интерфейс | post-1.0 | Не входит в `BarManager` v0.3 |

### 3.2 Текст, ввод и редактирование

| Элемент | Тип | Волна | Примечание |
|---|---|---|---|
| `Label` | статический текст | v0.1 | Перенос, ellipsis, mnemonics, доступное имя |
| `LinkLabel` | текст-ссылка | v1.0 | Опционально; курсор, hover, keyboard activation |
| `TextEdit` | однострочное поле | v0.1 | Selection, clipboard, undo/redo, IME, placeholder |
| `MemoEdit` | многострочное поле | v0.2 | Общий текстовый core, scroll, переносы |
| `PasswordEdit` | режим `TextEdit` | v0.2 | Маскирование, clipboard policy, reveal toggle |
| `SearchEdit` | режим `TextEdit` | v0.2 | Иконка поиска, clear button, debounce |
| `MaskedEdit` | режим `TextEdit` | v0.2 | Маска ввода; не обязателен для первого применения |
| `SpinEdit` | числовое поле | v0.2 | TextEdit + validation + step buttons |
| `DateEdit` | редактор даты | v0.2 | Calendar popup, ручной ввод, локализация |
| `Calendar` | выбор даты | v0.2 | Может быть частью `DateEdit` и отдельным контролом |
| `ColorPicker` | выбор цвета | v1.0 | Опционально; палитра, alpha, не в v0.1 |
| `RichTextEdit` | форматированный текст | post-1.0 | Не планируется в базовом ядре |
| `CodeEditor` | редактор кода | post-1.0 | Отдельная подсистема, не тащить в UI-core |

### 3.3 Кнопки и переключатели

| Элемент | Тип | Волна | Примечание |
|---|---|---|---|
| `Button` | кнопка | M0 | Text + optional icon; Primary/Secondary/Ghost/Disabled |
| `IconButton` | вариант кнопки | v0.1 | Может быть стилем `Button`, а не отдельным классом |
| `DropDownButton` | кнопка с popup | v0.2 | Требует `PopupHost` и `PopupMenu` |
| `SplitButton` | кнопка + отдельная стрелка | v0.2 | Основное действие и список действий |
| `ToggleSwitch` | boolean-переключатель | v0.2 | Альтернатива checkbox; доступность и анимация |
| `SegmentedControl` | группа взаимоисключающих кнопок | v0.2 | Удобно для выбора темы/режима |
| `RadioCard` | карточка-радиокнопка | v0.2 | Нужна для макета выбора темы; возможно, стиль `RadioButton` + `Panel` |

### 3.4 Выбор и списки

| Элемент | Тип | Волна | Примечание |
|---|---|---|---|
| `CheckBox` | boolean/three-state | v0.1 | Keyboard, indeterminate state, доступность |
| `RadioButton` | выбор из группы | v0.1 | Группировка, стрелки, mnemonics |
| `ComboBox` | выпадающий выбор | v0.2 | Editable/non-editable, popup, search |
| `CheckedComboBox` | multi-select dropdown | v0.2 | Чекбоксы в popup, сводка выбора |
| `ListBox` | простой список | v0.2 | Selection, keyboard, scroll, virtualization |
| `CheckedListBox` | список с чекбоксами | v0.2 | Общая модель checkable items |
| `ListView` | список с иконками/колонками | v0.2 | Отличать от простого `ListBox` и табличного `GridView` |
| `TreeList` | дерево с колонками | v0.3 | Виртуализация, expand/collapse, selection, editing, row states |
| `GridView` | таблица данных | v0.3 | Колонки, сортировка, фильтрация, inline editing, virtual rows, row states |
| `PropertyGrid` | редактор свойств | v0.3 | `PropertyDescriptor`, категории, редакторы значений; последним |
| `VerticalGrid` | вертикальная запись | v0.3 / TBD | Отдельный контрол; решить, нужен ли вместе с `PropertyGrid` |
| `DataPager` | пагинация | post-1.0 | Опционально, если `GridView` не ограничится virtualization |
| `FilterBuilder` | конструктор фильтров | post-1.0 | Опционально |
| `ColumnChooser` | настройка колонок | post-1.0 | Опционально |

### 3.5 Индикаторы, обратная связь и сервисы

| Элемент | Тип | Волна | Примечание |
|---|---|---|---|
| `ProgressBar` | determinate progress | v0.1 | Горизонтальный, значения 0..1, labels по желанию |
| `Loader` | индикатор загрузки | v0.1 | Inline-индикатор; блокирующий overlay — отдельный `BusyOverlay` |
| `ProgressRing` | компактный spinner | v0.1 | Анимированный, reduced-motion aware |
| `BusyOverlay` | блокирующий overlay | v0.1 | Блокирует ввод, focus trap, отмена/прогресс |
| `Validation` / `ErrorProvider` | сервис ошибок | v0.1 | Inline error, иконка, tooltip, доступное описание |
| `Image` / `PictureBox` | изображение | v0.1 | PNG/JPEG/SVG-ресурс, масштабирование, DPI |
| `Icon` / `Glyph` | ресурс иконки | Foundation / v0.1 | SVG path или bitmap; не обязательно отдельный контрол |
| `Command` / `Action` | команда | Foundation | enabled/checked/text/icon/shortcut; один источник для Button/BarManager |
| `DataSource` / `Adapter` | модель данных | Foundation | Общий контракт для ComboBox/ListBox/TreeList/GridView |
| `PropertyDescriptor` | метаданные свойства | Foundation | Для `PropertyGrid`; без C++-рефлексии |
| `ToolTipService` | сервис подсказок | v0.1 | Единая задержка и позиционирование для `ToolTip` |
| `FileDialog` / `FolderDialog` | системные диалоги | v0.2 | Платформенный сервис, не рисовать самостоятельно в MVP |

## 4. Что было упущено в исходном списке

### 4.1 Критично для фундамента

| Добавление | Почему нельзя откладывать |
|---|---|
| **`ScrollViewer` + `ScrollBar` + virtualized item model** | Без общего механизма `MemoEdit`, `ListBox`, `TreeList`, `GridView` и `CheckedComboBox` начнут делать собственный скролл |
| **Общий `PopupHost` + `ToolTip`** | `ComboBox`, `CheckedComboBox`, `DropDownButton`, `PopupMenu` и редакторы `PropertyGrid` используют один и тот же popup-механизм; `PopupMenu` не является им |
| **Layout-контейнеры** | `Panel`/`GroupPanel` — только базовые поверхности; `StackPanel` и `GridPanel` нужны для нормальных форм |
| **`SplitContainer`, `TabControl`** | Стандартные формы и IDE-подобные экраны без них не собираются |
| **`Image`/`PictureBox`, `Icon`** | Логотипы, иконки кнопок и деревьев, пустые состояния |
| **`Dialog`, `MessageBox`, `BusyOverlay`** | `Form` — не универсальное модальное окно; подтверждения и блокирующие операции нужны почти сразу |
| **`Command`/`Action`** | Единый источник `enabled/checked/shortcut` для кнопок, меню и `BarManager` |
| **`StatusBar`** | Был в требованиях, но отсутствовал в списке; сейчас зафиксирован как часть `Form` |
| **`Validation` / `ErrorProvider`** | Ошибки ввода и форм; дешевле заложить в базовый контрол, чем добавлять после |

### 4.2 Повседневные, но не обязательные для M0

- `DateEdit` и `Calendar`.
- `ToggleSwitch`.
- `Slider`/`TrackBar`.
- `ListView`.
- `CheckedListBox`.
- `SearchEdit`, `PasswordEdit`, `MaskedEdit` — как режимы `TextEdit`.
- `RadioCard`/`SegmentedControl` — нужны для экрана выбора темы из макета.
- `LinkLabel`, `ColorPicker`, `Accordion`/`Expander`, `Toast`/`InfoBar`, `Skeleton`, `Badge`.

### 4.3 Сервисы, а не контролы

- DataSource/Adapter, сортировка, фильтрация, редактирование, колонки.
- Keyboard/focus management.
- UI Automation и screen-reader support.
- Локализация и pluralization.
- Clipboard, undo/redo, IME.
- Drag & drop.
- Файловые диалоги.
- High contrast / reduced motion.

## 5. Уточнения по названиям

### `Button (Icon, DropDown)`

Рекомендуемое разделение:

- `Button` — базовое действие с текстом и опциональной иконкой.
- `IconButton` — стиль или отдельный класс для icon-only.
- `DropDownButton` — кнопка открывает `PopupMenu`.
- `SplitButton` — основное действие + отдельная область со списком.

Иконка — свойство, а не отдельный тип кнопки.

### `Form`

`Form` — top-level окно. Модальные окна должны быть отдельным `Dialog`, иначе `Form` начнёт обрастать режимами `ShowDialog`/owner/return-focus. `MessageBox` — готовый диалог поверх `Dialog`.

### `Panel` и `GroupPanel`

`Panel` — базовая поверхность. `GroupPanel` — панель с заголовком. Для компоновки нужны отдельные `StackPanel`, `GridPanel`, `SplitContainer`, `TabControl`. Не путать `GridPanel` с `GridView`.

### `TreeList` и `TreeView`

`TreeList` — дерево с колонками и строками данных. Если нужен простой иерархический список без колонок, это отдельный `TreeView`. Заводить оба класса стоит только если разница в API оправдана; иначе сделать `TreeView` и `TreeList` как два режима/надстройки.

### `GridView` и `ListView`

- `ListView` — список элементов (иконки, details, selection), обычно не таблица с произвольными колонками.
- `GridView` — таблица с колонками, сортировкой, фильтрацией, редактированием и виртуализацией.

Не стоит делать `GridView` «универсальным списком на всё».

### `PropertyGrid` и `VerticalGrid`

Это разные контролы:

- `PropertyGrid` показывает свойства выбранного объекта: имя/значение, категории, редакторы.
- `VerticalGrid` — вертикальное представление записи, ближе к data-контролу.

Если нужен только один — лучше выбрать один и явно убрать второй из backlog, чтобы не тащить лишнюю архитектуру.

### `CheckedComboBox` и `CheckedListBox`

Оба нужны, если пользователю важно множественное выделение и в выпадающем списке, и в обычном. У них должна быть общая модель checkable items, а не две независимые реализации.

### `BarManager`

Это не листовой контрол, а менеджер:

- `Command`/`Action`;
- `Bar`/`ToolBar`/`MenuBar`;
- shortcuts;
- enabled/checked states;
- контекстные меню.

`BarManager` не должен рисовать «всё меню» одним классом; ему нужны отдельные `Bar`, `BarItem`, `Menu`, `PopupMenu`.

### `Loader`

Планируемое разделение:

- inline-индикатор — `Loader`/`ProgressRing`;
- блокирующий overlay — `BusyOverlay`.

Не использовать одно имя для обоих сценариев. Конкретный набор публичных имён
(`Loader` + `ProgressRing` или только один) уточняется при реализации v0.1.

### `StatusBar`

Сейчас зафиксирован как часть `Form` с публичным API для левой/правой зоны. Если позже понадобится несколько статус-баров или встраивание в панели — вынести в отдельный контрол.

### `Label`

Нужны:

- перенос строк и ellipsis;
- mnemonics (`&` + Alt);
- доступное имя;
- опционально `LinkLabel`, а не перегрузка `Label` гиперссылкой по умолчанию.

## 6. Зависимости

```text
Command           -> Button, PopupMenu, BarManager, shortcuts
PopupHost         -> ComboBox, CheckedComboBox, DropDownButton, PopupMenu, PropertyGrid
Scroll/Virtual    -> MemoEdit, ListBox, ListView, TreeList, GridView, CheckedComboBox
DataSource        -> ListBox, ListView, ComboBox, TreeList, GridView, PropertyGrid
TextEdit core     -> MemoEdit, SpinEdit, ComboBox editor, PropertyGrid editors
Validation        -> TextEdit, SpinEdit, ComboBox, Form
```

Дополнительные правила:

1. **`PropertyGrid` — последним.** Он зависит от редакторов, `PropertyDescriptor`, категорий, валидации и popup.
2. **`BarManager` — после `Command` и `PopupMenu`.** Иначе меню и кнопки будут дублировать состояние.
3. **`TreeList`/`GridView` — после Scroll/Virtualization и DataSource.** Это не «нарисовать таблицу», а модель данных, selection, inline editing, row states, keyboard и UIA.
4. **`ComboBox`/`CheckedComboBox` — после PopupHost и item model.**
5. **`TextEdit` — раньше `MemoEdit`, `SpinEdit` и `PropertyGrid`.**
6. **`Form` — раньше `Dialog`.** `Dialog` переиспользует chrome, focus return и modal loop.

## 7. План по волнам

### Foundation — до публичных контролов

- layout engine, measure/arrange, DIP и DPI;
- **`DensityMode`**: Compact / Cozy / Comfortable, row/control height и type scale;
- Direct2D/DirectWrite и device management;
- базовый `Widget`, paint, event routing, hit-testing;
- focus manager, keyboard navigation, pointer capture;
- scroll и virtualization hooks;
- `PopupHost`, tooltip infrastructure;
- `Command`/`Action`;
- theme tokens и переключение Light/Dark/System;
- image/icon resources;
- data source/descriptor contracts;
- validation base;
- accessibility hooks.

### M0 — Technical Preview

- `Form` с title bar, resize, min/max/restore, close, extra actions;
- `StatusBar` как часть `Form`;
- `Button` (primary/secondary/ghost/disabled, hover/press/focus);
- Light/Dark/System;
- демо и базовая документация.

### v0.1 — Form Starter

- `Label`;
- `Panel`, `GroupPanel`, `StackPanel`, `GridPanel`;
- `ScrollViewer`, `ScrollBar`;
- `TextEdit`;
- `CheckBox`, `RadioButton`;
- `ProgressBar`, `Loader`, `ProgressRing`;
- `PopupMenu`, `ToolTip`;
- `Dialog`, `MessageBox`, `BusyOverlay`;
- `Image`/`PictureBox`;
- базовая `Validation`;
- поддержка иконок у `Button`;
- соответствующие тесты и примеры.

### v0.2 — Input & Selection

- `MemoEdit`;
- `ComboBox`, `CheckedComboBox`;
- `ListBox`, `CheckedListBox`;
- `ListView`;
- `SpinEdit`;
- `DropDownButton`, `SplitButton`;
- `ToggleSwitch`, `Slider`;
- `DateEdit`, `Calendar`;
- Search/Password/Masked режимы `TextEdit`;
- `TabControl`, `SplitContainer`;
- `SegmentedControl`/`RadioCard`;
- системные file/folder dialogs;
- расширение keyboard/focus/UIA.

### v0.3 — Data Controls

- `TreeList`;
- `GridView`;
- `PropertyGrid` с `PropertyDescriptor`;
- `VerticalGrid` — если решено оставить;
- полноценный `BarManager` (`MenuBar`, `ToolBar`, shortcuts);
- data binding/adapter;
- virtualization и item recycling;
- editing/selection/sorting/filtering;
- базовый UIA для data-контролов;
- общий data demo.

### v1.0 — Stability & Extras

- API freeze и semver-политика;
- полная документация и примеры;
- accessibility-полировка и UIA;
- performance budgets и профилирование;
- `ColorPicker`, `Accordion`/`Expander`, `NavigationView`/`Breadcrumb`;
- `Toast`/`InfoBar`, `Skeleton`/`Badge`;
- packaging и стабильная сборка.

### post-1.0

- Docking;
- Ribbon;
- `CodeEditor`;
- charts, report designer;
- cross-platform backend;
- advanced data tooling (`FilterBuilder`, `DataPager`, `ColumnChooser`).

## 8. Definition of Done для контрола

Контрол считается готовым, если:

- [ ] публичный API и владение объектами задокументированы;
- [ ] используются только семантические токены темы, без хардкода цветов/размеров;
- [ ] проверены Light / Dark / System;
- [ ] проверены Compact / Cozy / Comfortable, если контрол участвует в data-layout;
- [ ] проверены 100 / 125 / 150 / 200% DPI;
- [ ] работают mouse, keyboard и focus; focus визуально заметен;
- [ ] есть состояния `disabled` и, где нужно, `readonly`;
- [ ] есть доступное имя/роль; UIA-провайдер добавлен в рамках волны;
- [ ] ошибки/валидация отображаются и не ломают layout;
- [ ] есть unit-тесты и хотя бы один визуальный snapshot;
- [ ] нет известных P0/P1;
- [ ] есть пример использования и строка в документации;
- [ ] нет лишних аллокаций и перерисовок в горячем пути.

## 9. Открытые вопросы

1. Нужен ли отдельный `TreeView` в дополнение к `TreeList`?
2. Оставляем ли `VerticalGrid`, если уже есть `PropertyGrid`?
3. Каков точный scope `BarManager`: только toolbar/menu или ещё status bar и shortcuts?
4. `DropDownButton`/`SplitButton` — отдельные классы или режимы `Button`?
5. `RadioCard` — новый контрол или стиль `RadioButton` + `Panel`?
6. Нужен ли `ColorPicker` в первой стабильной версии?
7. Какая модель data binding нужна: минимальные adapters или полноценные observable-свойства?
8. Когда включать UIA: базовая доступность в v0.1 или полноценно в v0.3?
9. Нужна ли поддержка RTL/BiDi в v1.0?
10. Насколько важен Windows 10 feature parity?

Ответы фиксируются в [docs/DECISIONS.md](docs/DECISIONS.md), а сроки и состав релизов — в [docs/ROADMAP.md](docs/ROADMAP.md).
