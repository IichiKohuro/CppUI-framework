# ROADMAP — релизы и контрольные точки

> Статус: Draft · 14.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md), [QUALITY.md](QUALITY.md), [ARCHITECTURE.md](ARCHITECTURE.md).

## 1. Принципы планирования

- Сначала — фундамент и вертикальный срез формы, затем контролы волнами.
- Каждый релиз должен быть пригоден для реального приложения, а не только для демонстрации.
- Data-контролы не начинаются, пока не готовы scroll, virtualization, command, popup и data-контракты.
- Оценки относительные и уточняются после M0. Это не календарный план с фиксированными датами.
- Если сроки поджимают, режем объём, а не качество фундамента.

**Допущения по ресурсам:** один senior C++/Win32-разработчик, full-time; дизайн и документация ведутся параллельно; cross-platform и docking не входят.

## 2. Обзор релизов

| Релиз | Цель | Ориентировочно от старта | Основной результат |
|---|---|---:|---|
| **M0 Technical Preview** | Проверить архитектуру и визуал | 5–6 недель | Форма, Button, темы, custom chrome |
| **v0.1 Form Starter** | Минимальный полезный toolkit | 8–12 недель | Базовые поля, панели, диалоги, статус-бар |
| **v0.2 Input & Selection** | Полноценный ввод и выбор | 14–20 недель | ComboBox, ListBox, TabControl, SplitContainer, DateEdit |
| **v0.3 Data Controls** | Работа с данными | 22–32 недели | TreeList, GridView, PropertyGrid, BarManager |
| **v1.0** | Стабильный релиз | 30–40 недель (6–9 месяцев) | API freeze, документация, доступность, extras |
| **post-1.0** | Крупные расширения | после v1.0 | Docking, Ribbon, CodeEditor, cross-platform |

Оценки кумулятивные и включают дизайн, тесты, документацию и стабилизацию. Для одного разработчика **6–9 месяцев до v1.0** — базовый сценарий. Более короткие сроки возможны только при урезанном объёме, дополнительном ресурсе или отказе от части качества.

## 3. M0 — Technical Preview

**Вопрос, на который отвечает релиз:** можно ли сделать быструю, красивую и надёжную форму на Win32 + Direct2D, и удобен ли выбранный API?

### Состав

- `Form` с кастомным title bar.
- Изменение размера за все края; minimize / maximize / restore / close.
- Дополнительные действия в title bar.
- `StatusBar` как часть формы.
- `Button`: Primary / Secondary / Ghost / Disabled, hover/press/focus.
- Light / Dark / System темы.
- DPI и multi-monitor.
- Демо-приложение.
- Визуальный концепт и дизайн-токены.

### Критерии выхода

- [ ] Демо собирается с чистой машины по инструкции.
- [ ] Chrome проходит матрицу: Win11, 100–200% DPI, два монитора, Snap Layouts, Alt+Space, Alt+F4.
- [ ] Нет мерцания при resize; idle CPU близок к нулю.
- [ ] Тема переключается без перезапуска.
- [ ] Tab/Shift+Tab, Enter/Space работают; focus заметен.
- [ ] Есть базовые unit- и visual-тесты, CI зелёный.
- [ ] Performance-замеры зафиксированы в [QUALITY.md](QUALITY.md).

### Чего в M0 нет

- Data-контролы, меню/бар-менеджер, UIA.
- Полноценный текстовый ввод и IME.
- Настраиваемый layout-конструктор.
- Cross-platform.

## 4. v0.1 — Form Starter

**Цель:** набор, с которого можно собрать реальную небольшую форму.

### Состав

- `Label`, `Panel`, `GroupPanel`, `StackPanel`, `GridPanel`.
- `ScrollViewer`, `ScrollBar`.
- `TextEdit`.
- `CheckBox`, `RadioButton`.
- `DensityMode` (Compact / Cozy / Comfortable) как свойство theme/layout.
- `ProgressBar`, `Loader`, `ProgressRing`.
- `PopupMenu`, `ToolTip`.
- `Dialog`, `MessageBox`, `BusyOverlay`.
- `Image`/`PictureBox`, иконки в `Button`.
- Базовая `Validation`.

### Критерии выхода

- [ ] Демо-приложение с формой, полем ввода, чекбоксами, радио-группой, статус-баром, диалогом и popup-меню.
- [ ] Текстовый ввод: selection, clipboard, undo/redo, IME.
- [ ] Валидация отображается и не ломает layout.
- [ ] Keyboard navigation покрывает все контролы волны.
- [ ] Scroll работает для `TextEdit`/панелей; API готов к virtualization.
- [ ] Есть документация на каждый публичный контрол.

### Главный риск

`TextEdit` может съесть больше времени, чем ожидается. Если так, v0.1 не расширяем:
`MemoEdit`, `SpinEdit` и остальные поля остаются в v0.2.

## 5. v0.2 — Input & Selection

**Цель:** закрыть типовые формы выбора и ввода.

### Состав

- `MemoEdit`.
- `ComboBox`, `CheckedComboBox`.
- `ListBox`, `CheckedListBox`.
- `ListView`.
- `SpinEdit`.
- `DropDownButton`, `SplitButton`.
- `ToggleSwitch`, `Slider`.
- `DateEdit`, `Calendar`.
- Search / Password / Masked режимы `TextEdit`.
- `TabControl`, `SplitContainer`.
- `SegmentedControl` / `RadioCard`.
- Системные file/folder dialogs.

### Критерии выхода

- [ ] На примере собирается форма настроек с вкладками, сплиттером, combo, date picker и списком.
- [ ] Keyboard/focus работают в popup, списках и вкладках.
- [ ] Popup корректно закрывается по Esc, клику вне и потере фокуса.
- [ ] Начата UIA-поддержка ключевых контролов.
- [ ] Visual snapshots покрывают Light/Dark и 100/150/200%.

## 6. v0.3 — Data Controls

**Цель:** поддержать десктопные приложения с таблицами, деревьями и свойствами.

### Состав

- `TreeList`.
- `GridView` с inline editing и row states.
- `PropertyGrid` через `PropertyDescriptor`.
- `VerticalGrid` — если решение оставить принято.
- Data-workspace demo: дерево + таблица + инспектор + bottom dock.
- Полноценный `BarManager`: меню, toolbar, shortcuts.
- DataSource/Adapter.
- Виртуализация строк, selection, editing, сортировка, фильтрация.
- Базовый UIA для data-контролов.

### Критерии выхода

- [ ] `GridView` стабильно работает на 100k строк без загрузки всех элементов в UI.
- [ ] `TreeList` поддерживает expand/collapse, selection и keyboard.
- [ ] `PropertyGrid` редактирует объект по явным дескрипторам.
- [ ] `BarManager` управляет командами и не дублирует состояние кнопок.
- [ ] Редактирование ячеек использует общий validation/editor слой.
- [ ] Keyboard- и screen-reader-сценарии проходят проверку.

### Главный риск

Это самая дорогая волна. `GridView` и `PropertyGrid` могут занять больше времени, чем все предыдущие волны вместе. Не начинать без реального сценария использования.

## 7. v1.0 — Stability & Extras

**Цель:** версия, которую можно рекомендовать внешним пользователям.

### Состав

- API freeze и semver.
- Полная документация, примеры, migration notes.
- Доступность, высокая контрастность, reduced motion.
- Performance budgets и профилирование.
- `ColorPicker`, `Accordion`/`Expander`, `NavigationView`/`Breadcrumb`.
- `Toast`/`InfoBar`, `Skeleton`/`Badge`.
- Упаковка и стабильная сборка.

### Критерии выхода

- [ ] Публичный API не меняется без major-версии.
- [ ] Есть стабильный набор примеров, включая data-приложение.
- [ ] Пройдена accessibility-матрица для ключевых сценариев.
- [ ] Зафиксированы и проверены performance-бюджеты.
- [ ] Нет известных P0/P1.
- [ ] Понятна лицензия и политика поддержки.

## 8. post-1.0

Крупные расширения, которые нельзя тащить в ядро без отдельного проекта:

- Docking.
- Ribbon.
- CodeEditor.
- Charts и отчёты.
- Второй платформенный/рендер-бэкенд.
- Продвинутый data tooling: `FilterBuilder`, `DataPager`, `ColumnChooser`.

## 9. Зависимости между релизами

```text
M0 (Form + Button)
  └─ v0.1 (label/input/panels/dialog)
       └─ v0.2 (selection/navigation/date)
            └─ v0.3 (data controls)
                 └─ v1.0 (stability)
```

Внутри волн:

- Scroll и Popup — раньше `ComboBox`/`ListView`/`MemoEdit`.
- Command — раньше `BarManager`.
- TextEdit — раньше `SpinEdit`, `ComboBox` editor, `PropertyGrid`.
- DataSource — раньше `TreeList`/`GridView`/`PropertyGrid`.
- Validation — раньше `PropertyGrid` и редакторов ячеек.

## 10. Контрольные точки

| Точка | Что проверяем | Действие по результату |
|---|---|---|
| После Phase 2 (chrome) | Snap Layouts, resize, DPI, DWM | Если слишком нестабильно — упростить часть chrome до fallback, не блокируя виджеты |
| После M0 | Фактическая скорость, качество API, визуал | Уточнить оценки v0.1–v0.3; при необходимости урезать v0.1 |
| После v0.1 | Удобство text/popup/validation | Не начинать v0.2, пока не закрыты P0/P1 и не стабилен focus |
| После v0.2 | Реальная стоимость ListView/TabControl | Пересмотреть объём v0.3; data-контролы не резать по качеству |
| Перед v0.3 | Есть ли реальный data-сценарий | Если нет — отложить `TreeList`/`PropertyGrid`, сделать v1.0 на базовых контролах |
| Перед v1.0 | API и доступность | Заморозить API; не добавлять новые крупные контролы |

## 11. Что можно делать параллельно

- Дизайн-токены и визуальные снапшоты.
- Unit-тесты layout/theme/command.
- Документация API.
- Примеры приложений.
- Performance-инструментация.

Нельзя безопасно распараллелить рисование и поведение одного контрола без стабильного базового `Widget` API.

## 12. Сценарии сокращения срока

Если нужен быстрый результат:

1. **Только M0** — 5–6 недель: форма и кнопка, темы.
2. **M0 + TextEdit/CheckBox/Panel** — примерно 8–10 недель: минимальный редактор форм.
3. **v0.1** — 8–12 недель: практичный toolkit без data-контролов.
4. Всё остальное — только при наличии реального запроса.

Сокращать нужно количество контролов, а не качество chrome, DPI, focus и тестов.
