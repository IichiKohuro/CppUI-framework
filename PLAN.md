# PLAN — создание CppUI

> Статус: Draft · 14.11.2026 · Рабочее название проекта — **CppUI**.
> Визуальное направление: **Signal Teal**; density S/M/L и data-workspace demo зафиксированы в [DESIGN_SYSTEM.md](docs/DESIGN_SYSTEM.md).
> Связанные документы: [CONTROL_BACKLOG.md](CONTROL_BACKLOG.md), [docs/ROADMAP.md](docs/ROADMAP.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/QUALITY.md](docs/QUALITY.md), [docs/DECISIONS.md](docs/DECISIONS.md).

## 1. Видение

CppUI — компактный нативный UI-фреймворк для Windows-приложений на C++20. Он должен закрывать типовой десктопный интерфейс: форму с кастомным заголовком, статус-бар, меню и панели действий, поля ввода, списки, таблицы и свойства объекта.

Ключевая идея — **не конкурировать с Qt по объёму**, а сделать узкое, предсказуемое и качественное ядро, на котором быстро собираются собственные приложения. Сначала — форма и базовые контролы, затем data-heavy контролы отдельными волнами.

## 2. Цели

1. **Скорость.** Нативная отрисовка, отсутствие лишних прослоек, минимум аллокаций в кадре, стабильные 60 FPS при resize.
2. **Удобство.** Понятное владение объектами, layout без ручных координат, темы без переписывания контролов.
3. **Современный вид.** Аккуратная типографика, светлая/тёмная темы, осмысленные состояния наведения и фокуса, кастомный title bar.
4. **Расширяемость.** Отдельные слои `core / platform / render / widgets`, публичный API не протекает Win32/COM-типами.
5. **Качество.** DPI, клавиатура, доступность, визуальные тесты и измеряемая производительность закладываются с основания.
6. **Управляемый объём.** Контролы вводятся волнами; каждый новый контрол имеет зависимости и критерии готовности.

## 3. Не цели первого этапа

- Cross-platform (macOS/Linux) и второй рендер-бэкенд.
- Аналог полного Qt/DevExpress по количеству контролов.
- Docking, Ribbon, визуальный designer, charts, report designer, code editor.
- Полноценный data binding и `PropertyGrid` до готовности базовых редакторов.
- Полная поддержка Windows 10 и ARM64 (Windows 11 x64 — приоритет).
- Стабильный ABI/DLL-контракт до версии 1.0.

## 4. Границы поставки

### 4.1 Technical Preview (M0)

Минимальный вертикальный срез, на котором проверяются архитектура и визуальное направление:

- окно `Form` с кастомным title bar;
- resize за все края, minimize / maximize / restore / close;
- дополнительные действия в title bar и статус-бар;
- кнопка с состояниями и вариантами `Primary` / `Secondary` / `Ghost` / `Disabled`;
- темы Light / Dark / System без перезапуска;
- DPI и multi-monitor на разумном уровне;
- демо-приложение и визуальный QA.

### 4.2 v0.1 — Form Starter

После M0 добавляются публичные `Label`, `Panel`/`GroupPanel`, `TextEdit`, `CheckBox`, `RadioButton`, `ProgressBar`, `Loader`, `PopupMenu`, `Dialog`/`MessageBox`, `ToolTip` и необходимый фундамент: команды, scroll, popup, validation, image/icon.

Точный состав и критерии выхода — в [docs/ROADMAP.md](docs/ROADMAP.md).

### 4.3 Полный заявленный список контролов

Полный каталог и волны — в [CONTROL_BACKLOG.md](CONTROL_BACKLOG.md). Ориентир: 6–9 месяцев для одного senior C++/Win32-разработчика при полном рабочем дне. Это не обещание даты, а ёмкость для планирования.

## 5. Технический подход

| Решение | Выбор |
|---|---|
| Платформа | Windows 11 x64 first; Windows 10 — guarded best effort |
| Язык | C++20 |
| Компилятор | MSVC 2022 (Build Tools), позже — clang-cl |
| Сборка | CMake 3.28+ / Ninja, `CMakePresets.json` |
| Артефакт | Статическая библиотека; DLL/ABI — после 1.0 |
| Окно | Win32 + DWM, custom non-client area, native hit-testing |
| Рендер | D3D11 + DXGI flip-model + Direct2D + DirectWrite |
| Fallback | WARP при недоступности аппаратного D3D11 |
| UI-модель | Retained-mode дерево, measure/arrange/paint, DIP |
| Текст | DirectWrite, системный Segoe UI / Segoe UI Variable при наличии |
| Зависимости | Windows SDK; test-only Catch2, без runtime-зависимостей |
| Строки API | UTF-8 на границе, UTF-16 внутри Win32/DirectWrite |
| Доступность | Клавиатура и focus с первого дня; UIA — поэтапно |

Полная архитектура, включая chrome, рендер и API, — в [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## 6. Этапы до Technical Preview

Оценка — один senior C++/Win32-разработчик, full-time. В календарь нужно заложить буфер.

| Этап | Время | Главный результат | Выход этапа |
|---|---:|---|---|
| **0. Bootstrap ✅** | 1–2 дня | Toolchain, git, CMake-пресеты, пустое окно, дизайн-токены | Сборка с нуля одной командой; окно открывается |
| **1. Platform + render ✅** | 3–4 дня | Win32-цикл, DXGI/D2D/DWrite, resize, DPI, device loss, WARP | Пустое окно resize без мерцания; idle CPU около нуля |
| **2. Custom chrome ✅** | 5–7 дней | `WM_NCCALCSIZE`, `WM_NCHITTEST`, drag, системные кнопки, Snap Layouts, DWM dark/corners | Chrome проходит ручную матрицу на Win11, 100–200% DPI |
| **3. Widget core + Button + theme ✅** | 4–5 дней | Дерево виджетов, layout, focus, Button, токены, Light/Dark/System; density-метрики и компактные Button | Демо с кнопками, переключателем темы в шапке и density S/M/L |
| **4. Preview demo ✅** | 3–4 дня | Экран формы, `FormPanel`/`StatusBar`, action в titlebar, базовая документация | Демо соответствует заявленному объёму M0 |
| **5. Hardening 🟡** | 4–5 дней | Unit/visual tests, CI, профилирование, resize/DPI matrix, исправление P0/P1 | DPI matrix/benchmark/visual/CI готовы; M0 closure pending |

**Итого:** 20–27 рабочих дней, примерно **5–6 календарных недель** с буфером.

Phase 0 закрыта 14.11.2026: git, CMake, токены Signal Teal, Win32-окно и D3D11/DXGI/D2D/DWrite baseline. Phase 1: `Win32Window`, resize/DPI callbacks, WARP, device-loss recovery и render smoke test. Phase 2: кастомный chrome, системные кнопки, hit testing, maximize/restore и DWM. Phase 3: retained-mode `Widget`/layout, `StackPanel`/`CardPanel`/`Label`/`Button`, `UiHost`, Light/Dark/System, переключатель темы в шапке и начальный `DensityMode` (compact/cozy/comfortable) для Button. Phase 4: Form-preview, `FormPanel`, `StatusBar`, sidebar, titlebar action сброса, density для Button/StatusBar; исправлена синхронизация render target. Phase 5: simulated DPI matrix 96–192, benchmark (Release: frame 8.33 ms / CPU 1.30 ms, layout 0.119 ms), WIC visual regression Light/Dark/Compact, top-level `Form` API, CI workflow, cold start 177 ms, idle CPU ≈0, memory 64.4 MB (P2). Детали — в [PHASE0](docs/PHASE0.md), [PHASE1](docs/PHASE1.md), [PHASE2](docs/PHASE2.md), [PHASE3](docs/PHASE3.md), [PHASE4](docs/PHASE4.md), [PHASE5](docs/PHASE5.md). Следующая — M0 closure: visual snapshots, реальная DPI/multi-monitor матрица, memory/flicker.

### Критерии выхода M0

- Все функции формы из раздела 4.1 работают в демо.
- Переключение Light / Dark / System не требует перезапуска.
- Нет мерцания и явных артефактов при resize; idle CPU близок к нулю.
- 100/125/150/200% DPI и перенос между мониторами не ломают геометрию.
- Tab/Shift+Tab и Enter/Space работают; фокус визуально заметен.
- Закрытие окна можно отменить через callback.
- CI собирает проект, unit-тесты и базовые визуальные снапшоты проходят.
- Есть README с быстрым стартом и известными ограничениями.

## 7. После M0

Последовательность волн:

1. **v0.1 Form Starter** — фундамент и минимальный полезный набор контролов.
2. **v0.2 Input & Selection** — поля ввода, выпадающие списки, вкладки, сплиттеры, дата/время.
3. **v0.3 Data Controls** — `TreeList`, `GridView`, `PropertyGrid`, `VerticalGrid`, `BarManager`, data binding и виртуализация.
4. **v1.0** — стабилизация API, доступность, документация, упаковка, дополнительные контролы.

Детали, зависимости и критерии — в [docs/ROADMAP.md](docs/ROADMAP.md).

## 8. Архитектурные обязательства

1. `core` не включает `windows.h`; платформа и рендер взаимодействуют через интерфейсы.
2. Публичные заголовки не содержат HWND, COM-указателей и внутренних структур.
3. Все контролы используют общий `PopupHost`, `ScrollViewer`, `Command` и `Theme`, а не собственные копии.
4. `TreeList`, `GridView` и `PropertyGrid` проектируются вокруг общего data-контракта, а не вокруг конкретного источника данных.
5. `PropertyGrid` использует явные `PropertyDescriptor`; в C++ нет рефлексии.
6. `BarManager` — менеджер команд и баров, а не листовой контрол.
7. Доступность и клавиатурная навигация закладываются в базовый `Widget`, а не добавляются после релиза.
8. Любой контрол проходит критерии Definition of Done из [CONTROL_BACKLOG.md](CONTROL_BACKLOG.md).

## 9. Риски

| Риск | Вероятность | Влияние | Митигация |
|---|---|---|---|
| Кастомный chrome и Snap Layouts | Высокая | Высокое | Отдельный spike в Phase 2; тестовая матрица; native hit-testing |
| Device loss / RDP / слабая GPU | Средняя | Среднее | Device manager, WARP fallback, recreate resources |
| Scope creep из-за большого списка контролов | Высокая | Высокое | Волны, Definition of Done, запрет тащить data-контролы в v0.1 |
| TextEdit/MemoEdit: IME, selection, undo | Высокая | Среднее | Общий текстовый core; тесты ввода и клавиатуры |
| PropertyGrid без рефлексии | Высокая | Среднее | Явные `PropertyDescriptor` и адаптеры |
| Визуальная несогласованность | Средняя | Среднее | Дизайн-токены, mockup, visual regression |
| Отсутствие toolchain | Низкая | Низкое | Phase 0, установка VS Build Tools + SDK + CMake/Ninja |
| Оптимизм в оценках | Высокая | Среднее | Пересмотр после M0 по фактической скорости |

## 10. Оценки и допущения

- Один опытный разработчик, full-time; без второго дизайнера и без cross-platform.
- Оценка M0 — 5–6 недель.
- v0.1 — ориентировочно 8–12 недель суммарно от старта.
- Полный заявленный список — **6–9 месяцев** в базовом сценарии; 4–6 месяцев возможны только в оптимистичном сценарии при урезанной полировке или дополнительном ресурсе.
- Оценки уточняются после M0 по реальным метрикам, а не остаются фиксированными.
- Любая новая волна начинается только после стабилизации предыдущей.

## 11. Открытые решения

1. Финальное имя и брендинг (сейчас — CppUI).
2. Лицензия и публичность репозитория.
3. Разделение `Button` на `IconButton` / `DropDownButton` / `SplitButton`.
4. Нужен ли одновременно `PropertyGrid` и `VerticalGrid`.
5. Нужен ли отдельный `TreeView` в дополнение к `TreeList`.
6. Полнота `BarManager`: menu bar, toolbar, status bar, shortcuts.
7. Тайминг UIA и уровень поддержки high contrast.
8. Дистрибуция: source-only, статическая библиотека или DLL после 1.0.
9. Когда делать `DensityMode` глобальным для всех контролов; в Phase 3 уже есть метрики и компактный Button, остальные контролы подключаются в Phase 4.
10. Оставаться ли на Segoe UI или добавить IBM Plex/Golos как font-профиль.

Решения и их статус ведутся в [docs/DECISIONS.md](docs/DECISIONS.md).

## 12. Ближайшие действия

1. Установить Visual Studio Build Tools, Windows 11 SDK, CMake и Ninja.
2. Инициализировать git-репозиторий и CMake-каркас.
3. Перенести токены **Signal Teal** и density-метрики из [docs/DESIGN_SYSTEM.md](docs/DESIGN_SYSTEM.md) в код.
4. Собрать пустое окно с Direct2D.
5. Провести spike кастомного chrome до реализации виджетов.

---

_Документ описывает план, а не уже реализованный функционал. Все оценки подлежат пересмотру после первых замеров._
