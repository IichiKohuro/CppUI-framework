# QUALITY — тесты, производительность и критерии релиза

> Статус: v0.1 Form Starter delivered · 15.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md), [ROADMAP.md](ROADMAP.md), [ARCHITECTURE.md](ARCHITECTURE.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md), [V01.md](V01.md).

## 1. Принципы

- Quality gate важнее скорости добавления контролов.
- Сначала автоматизируем то, что можно; chrome, DPI и Snap Layouts проверяем вручную по матрице.
- Визуальные эталоны проверяют светлую/тёмную темы и DPI, но сравнение должно иметь допуск.
- Performance — не «ощущение», а зафиксированные бюджеты и замеры.
- Доступность и клавиатура входят в Definition of Done, а не откладываются на «после релиза».

## 2. Пирамида тестов

### Unit

Быстрые тесты без HWND и реального окна:

- geometry, Rect, Insets, DIP/DPI conversions;
- constraints, measure/arrange, Stack/Grid layout;
- theme resolution и токены;
- переходы состояний Button/CheckBox/RadioButton;
- Command enabled/checked;
- validation state;
- keyboard focus order;
- scroll model и virtualization math;
- сериализация/десериализация layout-конфигурации, если появится.

### Integration

Тесты с окном, но без активного пользователя:

- `cppui.render_smoke`: hidden window, DPI matrix 96/120/144/192, Light/Dark, density S/M/L, target-size invariants, device recreation;
- `cppui.ui`: Widget layout, hit testing, click, focus, Space activation, disabled button, compact/cozy Button metrics, FormPanel/StatusBar layout and compact status height;
- `cppui.form`: public `Form` API Create/SetContent/theme/density callbacks/Show/Run/Close;
- `cppui.benchmark`: frame/layout/resize/theme/density timings, startup, target DIP size, CPU time;
- `cppui.dpi`: реальный перенос между 120 DPI и 96 DPI мониторами, смена DPI и client area;
- `cppui.chrome`: hit testing caption/min/max/close/resize, maximize/restore, minimize/restore, resize stress, DwmFlush;
- `cppui.visual`: WIC PNG capture Light/Dark/Compact, baselines в `tests/baselines`, допуск 8/255 и 2% пикселей;
- `cppui.dpi_awareness`: публичный `IsPerMonitorDpiAware()` и переход процесса в PMv2; `Form::Create` предупреждает при отсутствии awareness;
- `cppui.benchmark`: Release-метрики проверяются по порогам через `tests/benchmark_gate.ps1`; Debug требует только успешного прогона (G-1 в [RISKS.md](RISKS.md));
- создание hidden window и message loop;
- `WM_NCHITTEST`: caption, min/max/close, resize-border, client;
- custom maximize/restore, minimize, close и close-hover;
- `WM_SIZE`, `WM_DPICHANGED`, `WM_SETTINGCHANGE`;
- закрытие через `WM_CLOSE` и отмена через callback;
- minimize/maximize/restore state machine;
- popup open/dismiss/focus return;
- clipboard/undo для TextEdit;
- переключение темы без пересоздания окна.

### Visual regression

- Offscreen-рендер формы/контрола в PNG через WIC/D2D.
- Эталоны для Light/Dark и 100/150/200%.
- Сравнение с допуском на сглаживание шрифтов; фиксировать различия в отчёте.
- Snapshots хранить в репозитории; обновлять только осознанно.
- Проверять не только цвета, но и размеры/hit areas.

### Manual QA

- Кастомный chrome и Snap Layouts.
- Два монитора с разным DPI.
- Taskbar auto-hide и maximized bounds.
- RDP/WARP.
- IME, сложный ввод, clipboard.
- Inline editing: commit/cancel, клик вне, Esc, позиционирование popup.
- Row states: loading/error/new/changed и связанный с ними selection.
- Screen reader/UIA — после появления провайдеров.
- Real-world sample forms и data-сценарии.

## 3. Матрица окружений

| Параметр | Обязательные значения |
|---|---|
| ОС | Windows 11 24H2/25H2 (основная); Windows 10 — best effort |
| Архитектура | x64 |
| DPI | 100%, 125%, 150%, 200% |
| Плотность | Compact / Cozy / Comfortable для data-layout |
| Мониторы | Один; два с одинаковым DPI; два с разным DPI |
| Тема | Light, Dark, System |
| GPU | Аппаратный D3D11; WARP/RDP |
| Ввод | Мышь, клавиатура, IME |
| Локаль | Русский и английский; длинные строки |
| Окно | Normal, maximized, minimized, restore, close, Alt+F4 |
| Taskbar | Обычный и auto-hide |
| High contrast | Проверка после появления поддержки |

RTL/BiDi — не входит в v0.1; добавить в матрицу, когда будет принято решение о поддержке.

## 4. Definition of Done

Для контрола — критерии в [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md). Для релиза:

- [x] Все заявленные контролы/сценарии v0.1 реализованы.
- [x] Unit- и integration-тесты зелёные: **12/12 Debug**.
- [x] Visual snapshots v0.1 обновлены и просмотрены: Light/Dark/Compact/Popup.
- [x] Ручная матрица chrome/DPI пройдена для M0.
- [x] Compact / Cozy / Comfortable проверены на Form Starter.
- [x] Нет известных P0/P1 в v0.1.
- [x] Performance-бюджеты M0 соблюдены.
- [x] Документация и примеры обновлены: [CONTROLS.md](CONTROLS.md), [V01.md](V01.md).
- [x] Accessibility-минимум v0.1: Tab/Shift+Tab, focus, Enter/Space, Esc, focus trap.
- [ ] Сборка с чистой машины воспроизводится — clean build-каталоги проверены; clean machine waiver M0.

## 5. Performance-бюджеты

Это стартовые цели, которые нужно подтвердить на реальном железе после Phase 1. Значения не являются обещанием для любой конфигурации.

Phase 1 baseline: idle CPU измерен как 0.000 s за 8 s (≈0%); resize stress из пяти размеров не привёл к потере отзывчивости. Phase 2: hit testing и maximize/restore проверены на 125% DPI. Phase 3: UI-тест покрывает layout/hit-test/click/focus/Space/disabled и compact/cozy-метрики; Light/Dark/System, theme-switch в шапке, S/M/L и mouse-coordinate mapping проверены вручную. Phase 4: проверены Form-preview, StatusBar, reset-action, обновление статусов и синхронизация render target после density; render target без sync обрезал нижнюю часть UI. Phase 5: добавлены DPI matrix 96–200, real mixed-DPI 120/96, benchmark, chrome/visual tests и CI workflow; M0 принят с environment-waivers.

### Phase 5 benchmark (Release, hidden window, 125% DPI, 22 logical cores)

| Метрика | Значение | Статус |
|---|---:|---|
| Frame wall avg / p95 | **8.33 / 8.43 ms** (120 FPS VSync) | цель 60 FPS закрыта |
| Frame CPU avg | **~1.8 ms** | target <4 ms |
| Layout avg | **~0.15 ms** | target <0.3 ms |
| Resize cycle avg frame | **~7.4 ms** | hard <8 ms |
| Startup to first frame | **101 ms** | target 150 ms |
| Real mixed DPI | **120/96 DPI** | two monitors pass |
| Chrome resize DwmFlush | **17.7 ms worst** | ~56 FPS |
| Theme switch + frame | **7.98 ms** | hard ≤100 ms |
| Density switch + frame | **8.33 ms** | — |
| Cold start, external | **177 ms** | launcher/process start; hard 250 pass |
| In-app startup | **101 ms** | target 150 pass |
| Idle CPU, 8 s | **0.000 ms** | target pass |
| Private memory | **43.8 MB** | hard 60 MB pass; target 40 miss 3.8 MB |
| Working set | **64.4 MB** | shared GPU/font pages, informational |

С 15.11.2026 эти бюджеты являются gate, а не только отчётом: CTest-тест
`cppui.benchmark` запускает `tests/benchmark_gate.ps1` и падает в Release при
выходе за пределы. Пороги намеренно грубые (~5× от baseline), чтобы не ловить
шум CI; они рассчитаны на регресс на порядок. Подробности — в
[RISKS.md](RISKS.md), G-1.

Debug benchmark: frame ~8.33 ms, layout ~0.124 ms. M0 CTest **8/8** в Debug и Release;
после v0.1 — **12/12** в Debug. Clean build directory Debug собран с нуля. Visual
regression проходит exact-SHA за ~1.5 s. FLIP_DISCARD + single-threaded D3D снизили
frame CPU до ~1.8 ms. Реальный mixed-DPI 120/96 проверен на двух мониторах; 150/200%
simulated. Snap/Alt+Space/Alt+F4 проверены. Private memory 43.8 MB проходит hard 60 MB;
input-to-photon и flicker остаются не измеренными и вынесены в waiver [M0.md](M0.md).

### v0.1 результаты

| Проверка | Результат |
|---|---|
| `cppui.v01` | TextEdit/CheckBox/Radio/Grid/Scroll/Command/Image/Popup/ToolTip — pass |
| `cppui.dialog` | modal result, default Enter, owner re-enable, MessageBox cancel — pass |
| `cppui.visual_v01` | 4 WIC snapshots, Light/Dark/Compact/Popup — pass |
| Form Starter live | ввод, caret, ToolTip, popup, MessageBox, Yes/No/Cancel — проверено |
| Density | Compact/Cozy/Comfortable на всех контролах волны |
| Release CTest | **12/12**, benchmark gate pass |
| Release metrics v0.1 | frame 8.33 ms / 120 FPS, CPU 1.69 ms, layout 0.182 ms, resize 7.69 ms, startup 96.8 ms |
| Ограничения | non-modal Dialog, inline IME composition, popup submenu, ellipsis — не в v0.1 |

| Метрика | Target | Hard threshold |
|---|---:|---:|
| Cold start демо до первого кадра | ≤ 150 ms | ≤ 250 ms |
| Время до появления окна | ≤ 100 ms | ≤ 180 ms |
| Idle CPU | ≈ 0–0.5% | < 1% |
| Input-to-photon | ≤ 2 кадра при 60 Hz | ≤ 3 кадра |
| Resize | 60 FPS | ≥ 55 FPS на 2560×1440 |
| Пропущенные кадры при resize | < 1% | < 5% |
| Layout простой формы | < 0.3 ms | < 1 ms |
| CPU-время кадра простой формы | < 4 ms | < 8 ms |
| Аллокации в steady-state кадре | 0–минимум | без роста |
| Память демо, private | < 40 MB | < 60 MB (measured 43.8) |
| Переключение темы | 1 кадр | ≤ 100 ms |
| Frame wall avg, Release | 60 FPS | 120 FPS measured |
| Frame CPU avg, Release | < 4 ms | ~1.8 ms measured |

Дополнительно:

- Present с VSync; в покое рендер не крутится.
- WARP fallback не должен приводить к зависанию UI.
- При resize не должно быть мерцания и чёрных кадров.
- После многократных minimize/maximize/close не должно быть утечек HWND/COM.

### Benchmark scenarios

1. Idle form 1120 × 720.
2. Resize от 720 × 480 до 1920 × 1080.
3. Переключение темы 100 раз.
4. Открытие/закрытие 100 диалогов.
5. Long text/IME input.
6. `GridView` на 100k строк (v0.3).
7. `TreeList` с глубокой иерархией (v0.3).
8. `PropertyGrid` на объекте с 100 свойствами (v0.3).
9. Popup open/dismiss.
10. WARP/RDP smoke.

## 6. Доступность

Минимум для v0.1:

- клавиатурная навигация во всех интерактивных контролах;
- видимый focus и логичный Tab order;
- accessible name для icon-only действий;
- Enter/Space активируют кнопки и checkbox;
- Esc закрывает popup/dialog;
- контраст текста 4.5:1;
- disabled/error состояния не только цветом;
- tooltip не является единственным носителем важной информации.

Позже:

- UIA providers для ключевых контролов (v0.2–v0.3);
- High Contrast;
- screen reader тестирование;
- reduced motion;
- полная accessibility-матрица в v1.0.

## 7. CI

Добавлен `.github/workflows/windows.yml`:

- Windows 2022 runner, MSVC x64.
- Debug и Release.
- Configure/Build через CMake presets.
- CTest, включая DPI matrix, mixed-DPI, chrome, v0.1 controls/dialog, benchmark gate и visual regression (включая Form Starter).
- Artifacts с `cppui-benchmark.txt`; visual output загружается при падении.
- Warnings as errors для собственного кода.
- Сборка sample.

Ещё не подтверждено на удалённом runner. `clang-format --dry-run --Werror`, ASan и кеширование CMake/Ninja остаются следующими шагами. Visual snapshots входят в CTest, но на CI ещё не проверялись. CI не может полноценно проверить Snap Layouts, multi-monitor DPI и реальный WARP; эти пункты остаются в ручной матрице.

## 8. Статический анализ и стиль

- `/W4 /permissive- /Zc:__cplusplus`; `/WX` в CI.
- `clang-format` и `.editorconfig` с первого коммита.
- Позже — clang-tidy для критичных модулей.
- Без C-style casts, кроме Win32/COM границ, где это оправдано.
- RAII: COM через `ComPtr`, GDI/D2D ресурсы через умные обёртки.
- Новые публичные API требуют документации и примера.
- Никаких магических чисел цвета/размера в контролах.

## 9. Управление дефектами

| Уровень | Определение | Политика |
|---|---|---|
| **P0** | Крэш, зависание, потеря данных, сломанная сборка | Блокирует merge и релиз |
| **P1** | Сломан основной сценарий, нет workaround | Блокирует релиз |
| **P2** | Ошибка с workaround или редкий edge case | Можно отложить с тикетом |
| **P3** | Косметика, неудобство | В backlog |

Для chrome-багов обязательно указывать Windows build, DPI, число мониторов, GPU/WARP и шаги.

## 10. Release checklist

- [x] Все запланированные контролы v0.1 прошли Definition of Done.
- [x] Проверены Light/Dark/System.
- [x] 100/125/150/200% DPI и mixed-DPI (150/200 simulated, waiver).
- [x] Chrome-матрица пройдена.
- [ ] RDP/WARP smoke пройден — WARP render smoke есть, full RDP manual pending.
- [x] Нет новых P0/P1.
- [x] Performance-замеры M0 в допустимых пределах.
- [x] Visual snapshots приняты.
- [x] README, API docs и примеры обновлены.
- [ ] Версия и changelog зафиксированы — v0.1 notes есть, git tag не создавался.
- [ ] Сборка воспроизводится с чистой машины — local clean build only.

## 11. Риски качества

| Риск | Митигация |
|---|---|
| Нестабильные visual snapshots из-за шрифтов/D2D | Допуск, фиксация окружения, проверка ключевых метрик вместо 100% пикселей |
| Chrome ломается на обновлении Windows | Регулярная ручная матрица, graceful fallback |
| DPI-баги всплывают поздно | Тестировать DPI с Phase 2, не после релиза |
| TextEdit/IME сложнее ожидаемого | Ранние spike-тесты; не тянуть MemoEdit в v0.1 |
| Производительность деградирует с контролами | Бюджеты и benchmark с первого вертикального среза |
| UIA откладывается | Проектировать accessibility hooks в Widget с самого начала |

## 12. Ссылки на инструменты (позже)

- Catch2 — тесты.
- CMake/CTest — запуск.
- D2D + WIC — offscreen snapshots.
- PresentMon/ETW или собственные счётчики — frame timing.
- UI Automation Inspect — проверка доступности.
- WinAppDriver/аналоги — только если появится необходимость в end-to-end UI-тестах.
