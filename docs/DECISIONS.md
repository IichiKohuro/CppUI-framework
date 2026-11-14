# DECISIONS — журнал решений

> Статус: Draft · 14.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md), [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md).

## 1. Статусы

| Статус | Значение |
|---|---|
| **Accepted** | Зафиксировано, используется в планах и реализации |
| **Proposed** | Рекомендация, ожидает подтверждения |
| **Open** | Решение ещё не принято |
| **Deferred** | Осознанно отложено на позднюю волну |
| **Superseded** | Заменено другим решением |

## 2. Принятые решения

| ID | Решение | Статус | Обоснование / последствия |
|---|---|---|---|
| D-001 | Рабочее название — **CppUI** | Accepted | Удобно для репозитория и документов; финальный бренд — отдельно |
| D-002 | **Windows-first**, Windows 11 x64 приоритет, Windows 10 best effort | Accepted | Позволяет использовать Win32/DWM/D2D без компромиссов кросс-платформенного слоя |
| D-003 | **C++20 + MSVC 2022 + CMake + Ninja**, статическая библиотека | Accepted | Быстрая сборка, современный C++, без тяжёлого ABI/DLL до 1.0 |
| D-004 | **Win32 + DWM custom chrome** | Accepted | Полный контроль над title bar, Snap Layouts и DPI |
| D-005 | **D3D11 + DXGI flip-model + Direct2D/DirectWrite**, WARP fallback | Accepted | Низкая задержка, качественный текст, отсутствие внешнего рендера |
| D-006 | **Retained-mode** дерево виджетов, собственный layout `measure → arrange → paint` | Accepted | Предсказуемое состояние, проще темы, инспекция и доступность |
| D-007 | **Ноль сторонних runtime-зависимостей**, только Windows SDK; Catch2 — test-only | Accepted | Простое распространение, меньше рисков сборки |
| D-008 | Публичный API **без Win32/COM**, Pimpl, UTF-8 на границе | Accepted | Изоляция платформы, удобство для пользователей, место для будущего backend |
| D-009 | **Семантические design tokens**, режимы Light/Dark/System | Accepted | Единый визуальный язык, отсутствие хардкода цветов в контролах |
| D-010 | **M0 — Form + Button + темы**, остальные контролы — отдельными волнами | Accepted | Проверка архитектуры до наращивания объёма |
| D-011 | M0 и v0.1 **не включают data-контролы** | Accepted | `TreeList`/`GridView`/`PropertyGrid` — самая дорогая и рискованная часть |
| D-012 | **Keyboard/focus** с первого релиза; UIA поэтапно | Accepted | Доступность дешевле закладывать в базовый `Widget` |
| D-013 | **Один UI-поток**, рендер в UI-потоке на первом этапе | Accepted | Проще синхронизация; render thread — только при доказанной необходимости |
| D-014 | **Cross-platform не входит в первый этап** | Accepted | Windows-специфика (DWM/Snap) важнее ранней абстракции |
| D-015 | Оценки: M0 5–6 недель; **полный список 6–9 месяцев** в базовом сценарии | Accepted | Расширенный scope требует реалистичного ориентира; оценки уточняются после M0 |
| D-016 | **Scroll, Popup, Command, DataSource, Validation** — общие сервисы, а не логика конкретного контрола | Accepted | Убирает дублирование и будущие переделки |
| D-017 | **BarManager** — менеджер команд и баров, не листовой контрол | Accepted | Правильное разделение ответственности |
| D-018 | **StatusBar** на первом этапе — часть `Form`, не отдельный публичный контрол | Accepted | Меньше сущностей; API для зон уже есть |
| D-019 | Визуальные концепты формы и полной галереи контролов сохранены в `design/` | Accepted | Единый источник для реализации и QA; выбранное направление — D-023 |
| D-020 | **Command/Action** — центральный объект для кнопок, меню и shortcut | Accepted | Единые enabled/checked/text/icon states; не дублировать логику |
| D-021 | Для **`PropertyGrid`** использовать явные `PropertyDescriptor`, без рефлексии | Accepted | В C++ нет рефлексии; проще и предсказуемее |
| D-022 | **`Form`** владеет `TitleBar`, `ContentHost`, `StatusBar`; `Dialog` — отдельный класс | Accepted | Не превращать форму в набор режимов |
| D-023 | Основное визуальное направление — **Signal Teal** из Build Manager | Accepted | Выбрано после аудита `design_chats`; не копия интерфейса, а собственные токены CppUI |
| D-024 | **Density S / M / L** — свойство theme-слоя | Accepted | В галерее применено к workspace; в C++ станет общим `DensityMode` |
| D-025 | **Data-workspace demo** с TreeList + GridView + PropertyGrid + bottom dock | Accepted | Показывает реальный data-heavy сценарий, а не только карточки контролов |
| D-026 | **Inline editing и row states** входят в дизайн-требования data-контролов | Accepted | В HTML работают inline-меню, Running/Failed/new/changed; в C++ — поведение GridView/PropertyGrid |
| D-027 | CI использует **CMake + Ninja**; локально на локализованной MSVC — **Visual Studio generator** | Accepted | CMake 4.3 + Ninja на русской MSVC пишут `/showIncludes`-префикс в неверной кодировке и теряют header-deps; VS generator даёт корректный incremental build. Оба пути используют один CMake-проект |
| D-028 | Phase 1: **полное пересоздание D3D/DXGI/D2D-ресурсов** при device loss; отрисовка только по invalidation | Accepted | `DEVICE_REMOVED`/`RESET`/`HUNG` и ошибки target восстанавливаются без падения; idle CPU измерен около нуля. `Win32Window` остаётся внутренним слоем до custom chrome |
| D-029 | Phase 2: custom chrome через **WS_OVERLAPPEDWINDOW + WM_NCCALCSIZE(0) + WM_NCHITTEST + D2D title bar** | Accepted | Сохраняются Snap/Alt+Tab/Alt+F4 и системные resize-коды; DWM даёт rounded corners/shadow; кнопки min/max/close рисуются D2D |

## 3. Предложенные решения

| ID | Решение | Статус | Комментарий |
|---|---|---|---|
| P-001 | `Button` с опциональной иконкой; `DropDownButton` и `SplitButton` — отдельные поведения; icon-only — стиль `Button` | Proposed | Не размножать классы без необходимости; DropDown/Split имеют отдельную логику |
| P-005 | `RadioCard` реализовать как стиль `RadioButton` + `Panel`, а не новый класс | Proposed | Меньше API; проверить на реальном экране |
| P-006 | `SearchEdit`/`PasswordEdit`/`MaskedEdit` — режимы `TextEdit` | Proposed | Не раздувать число классов |
| P-007 | `ProgressRing` и `BusyOverlay` разделить: inline spinner vs блокирующий overlay | Proposed | Разные UX-сценарии |
| P-008 | System accent color не использовать автоматически в v0.1 | Proposed | Сохраняет бренд-акцент; можно добавить опцией позже |

## 4. Открытые решения

| ID | Вопрос | Что блокирует | Рекомендация |
|---|---|---|---|
| O-001 | Финальное название и брендинг | README, repo, namespace | Оставить CppUI до M0, переименовать до публичного релиза |
| O-002 | Лицензия и публичность репозитория | Публикация, вклад, зависимости | Решить до первого публичного push |
| O-004 | Нужен ли отдельный `TreeView` в дополнение к `TreeList` | API data-контролов | Сначала определить сценарии; возможно, TreeView позже |
| O-005 | Нужен ли `VerticalGrid`, если есть `PropertyGrid` | Объём v0.3 | Выбрать один, если нет реального сценария для двух |
| O-006 | Полный scope `BarManager` | v0.3, меню/тулбары | Определить: bars, menu, shortcuts, status bar? |
| O-007 | Модель data binding | v0.3 API | Начать с adapters и notifications, без тяжёлого observable |
| O-008 | Тайминг UIA | v0.1–v1.0 | Keyboard в M0, UIA-база в v0.3, полная в v1.0 |
| O-009 | High Contrast | Дизайн и QA | Запланировать в v1.0, проверить систему раньше |
| O-010 | Дистрибуция: source-only или DLL + C ABI | v1.0 | Статическая библиотека до 1.0; DLL после стабилизации |
| O-011 | RTL/BiDi | Дизайн, текст, layout | Не в v0.1; решить к v1.0 |
| O-012 | Windows 10 parity | QA и visual features | Best effort; не блокировать Win11-релиз |
| O-013 | Mica/acrylic | Визуал и рендер | Post-MVP, отдельный эксперимент |
| O-014 | Waitable swap chain / frame latency | Render pipeline | Проверить в Phase 1, если Present(1,0) даёт плохой input latency |
| O-015 | Нужен ли render thread | Архитектура | Только при доказанной проблеме с UI-thread render |
| O-016 | Когда распространять density на все контролы | v0.1/v0.2 | Сначала API/токены, затем контролы волнами |
| O-017 | Оставаться ли на Segoe UI или бандлить IBM Plex/Golos | Визуал, лицензии, вес сборки | Segoe UI default; font-профили после MVP |
| O-018 | Нужны ли дополнительные theme presets (Studio Indigo/Graphite) | Визуал, QA | Только после стабилизации Signal Teal |

## 5. Отложено осознанно

| ID | Что | Почему |
|---|---|---|
| D-101 | Docking | Крупная подсистема, не нужна для базовых форм |
| D-102 | Ribbon | Не покрывается `BarManager` автоматически |
| D-103 | CodeEditor | Отдельный продукт/подсистема |
| D-104 | Charts / report designer | Не входят в UI-core |
| D-105 | Cross-platform backend | Сначала валидируем Windows-версию |
| D-106 | Полная UIA-поддержка всех контролов в M0 | Слишком дорого до стабилизации API |
| D-107 | Тёмная тема для data-контролов в v0.1 | Контролов ещё нет |

## 6. Как менять решения

1. Добавить новую строку или изменить статус существующей.
2. Указать дату и короткое обоснование.
3. Обновить связанные документы:
   - scope/сроки — [PLAN.md](../PLAN.md), [ROADMAP.md](ROADMAP.md);
   - API — [ARCHITECTURE.md](ARCHITECTURE.md);
   - контролы — [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md);
   - визуал — [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md);
   - качество — [QUALITY.md](QUALITY.md).
4. Для архитектурных изменений — отдельный ADR-раздел или новый файл, если решение большое.

## 7. Что подтвердить в первую очередь

1. Финальное имя и лицензия.
2. Нужны ли одновременно `PropertyGrid` и `VerticalGrid`.
3. Нужен ли отдельный `TreeView`.
4. Scope `BarManager`.
5. Шрифтовой профиль: Segoe UI или IBM Plex/Golos.
6. Приоритет: быстрый релиз малого набора или полный toolkit на 6–9 месяцев.
