# RISKS — реестр рисков и gate-проверки

> Статус: Living document · 15.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md) §9, [ROADMAP.md](ROADMAP.md), [QUALITY.md](QUALITY.md), [DECISIONS.md](DECISIONS.md), [M0.md](M0.md).

Реестр разворачивает восемь рисков из [PLAN.md §9](../PLAN.md) и добавляет
риски, которые видны по репозиторию, но не попали в таблицу плана. Каждая
карточка содержит механизм, ранние признаки, действия и критерий закрытия.
Три проверки автоматизированы как CTest-gates (раздел 4).

## 1. Как читать реестр

| Поле | Значение |
|---|---|
| **ID** | `R-NN`; ссылки на ID используются в issues и ревью |
| **Вероятность / влияние** | H / M / L |
| **Статус** | `Open` — не закрыт; `Partial` — часть мер внедрена; `Mitigated` — есть работающая защита; `Blocked` — нельзя закрыть в текущем окружении; `Accepted` — принят осознанно |
| **Триггер** | наблюдаемое событие, переводящее риск в `Open` и назначающее дату |
| **Gate** | автоматическая проверка или ручная матрица, которая ловит регресс |
| **Ревью** | когда пересматривается статус |

Владелец всех рисков на текущем этапе — владелец ядра (единственный
senior C++/Win32-разработчик). Ревью — после каждой волны и внепланово при
срабатывании триггера. Риск без даты следующего ревью считается забытым.

## 2. Сводная таблица

| ID | Риск | П/В | Статус | Gate |
|---|---|---|---|---|
| R-01 | Кастомный chrome и Snap Layouts | H/H | Partial | `cppui.chrome` + work area |
| R-02 | Device loss / RDP / слабая GPU | M/M | Partial | `cppui.render_smoke`, WARP fallback |
| R-03 | Scope creep из-за списка контролов | H/H | Mitigated | заморозка scope v0.1 + 14/14 CTest |
| R-04 | TextEdit/MemoEdit: IME, selection, undo | H/M | Partial | `cppui.v01` + ручная IME-матрица |
| R-05 | PropertyGrid без рефлексии | H/M | Open | ревью контракта до v0.3 |
| R-06 | Визуальная несогласованность | M/M | Partial | `cppui.visual*` |
| R-07 | Отсутствие/хрупкость toolchain | L/L | Mitigated | `cppui.*` + CI |
| R-08 | Оптимизм в оценках | H/M | Open | пере-баз после v0.1 |
| R-09 | Удалённый CI ни разу не выполнялся | H/M | Blocked | G-3 |
| R-10 | Performance-бюджеты не были gate | M/M | Mitigated | G-1 |
| R-11 | Visual-тесты хрупкие по конструкции | M/M | Partial | `cppui.visual_v01_warp` + diff |
| R-12 | DPI awareness не выставлялся библиотекой | M/H | Mitigated | G-2 |
| R-13 | UIA/accessibility отложен архитектурно | M/H | Partial | metadata в `Widget` |
| R-14 | Лицензия и публичность репозитория | M/H | Mitigated | LICENSE в корне, решение O-002 |
| R-15 | Концентрация знаний на одном человеке | L/H | Accepted | документация + ADR |

### 2.1 Ревизия 15.11.2026

Проработаны все 15 рисков: у каждого есть текущий статус, конкретное действие и
остаток, который нельзя закрыть в текущем окружении.

| ID | Статус | Проверено / сделано | Остаток |
|---|---|---|---|
| R-01 | Partial | `WM_GETMINMAXINFO` максимизирует в work area монитора; `cppui.chrome` проверяет client на work area. Real 120/96, Snap/Alt+Space. | Win10, touch/pen, auto-hide taskbar, реальные 150/200%. |
| R-02 | Partial | WARP выбирается после 3 подряд render failures; `render_smoke` проходит и на WARP. | Нет реального TDR/RDP reconnect soak, нет backoff после WARP. |
| R-03 | Mitigated | Scope v0.1 закрыт, data-контролы не добавлены, 14/14 CTest. | Расширение v0.2 требует нового scope-review. |
| R-04 | Partial | Selection, clipboard, undo/redo, max length, read-only, IME result string; тесты в `cppui.v01`. | CJK IME вручную, inline composition, double-click, Memo. |
| R-05 | Open | Контракт `PropertyDescriptor` зафиксирован как обязательный до v0.3. | Кода нет; решение O-005 откладывает VerticalGrid. |
| R-06 | Partial | Baselines Light/Dark/Compact/Popup; focus-ring и border clipping исправлены. | Нет автоматической матрицы всех состояний hover/focus/disabled. |
| R-07 | Mitigated | VS 2026/MSVC 19.51, Windows SDK 10.0.26100, CMake 4.3, локальный Debug/Release. | Удалённый runner = R-09. |
| R-08 | Open | v0.1 закрыт; фактические замеры производительности есть. | Нет учёта часов на контрол, ROADMAP не пере-базирован. |
| R-09 | Blocked | Workflow-файлы и CTest готовы. | Нет git remote; ни одного remote run. |
| R-10 | Mitigated | G-1 проверяет Release-бюджеты; текущий прогон зелёный. | Пороги грубые, дрейф 20% не ловят. |
| R-11 | Partial | WARP-прогон Form Starter проходит; при mismatch создаётся diff PNG. | Нет CI-runner и второго шрифтового/GPU окружения. |
| R-12 | Mitigated | `IsPerMonitorDpiAware`, G-2, PMv2 в Form Starter, диагностика в `Form::Create`. | Библиотека осознанно не меняет процесс глобально. |
| R-13 | Partial | В `Widget` добавлены accessible name/role/value; контролы заполняют metadata. | UIA AutomationPeer и Narrator-тест отсутствуют. |
| R-14 | Mitigated | Выбрана MIT, добавлен [LICENSE](../LICENSE), обновлены README и O-002. | Лицензию может пересмотреть владелец до v1.0. |
| R-15 | Accepted | README, ARCHITECTURE, ADR, тесты и demo покрывают основной контекст. | Онбординг нового разработчика не измерялся. |

**Итог:** 5 Mitigated, 6 Partial, 2 Open, 1 Blocked, 1 Accepted. Все риски имеют
владельца, gate/ручную проверку и записанный остаток; необработанных карточек нет.

## 3. Карточки рисков

### R-01. Кастомный chrome и Snap Layouts — H/H, Partial

**Механизм.** Свой non-client означает ручную реализацию drag, resize,
min/max/close, double-click, Alt+Space, Snap Layouts, DWM-рамки. Ошибки
проявляются только в конкретных сочетаниях ОС, DPI, мониторов и таскбара.

**Где.** [win32_window.cpp](../src/platform/win32/win32_window.cpp):
`WM_NCCALCSIZE` компенсирует рамку в maximized, `HitTest` отдаёт
`HTMAXBUTTON`, поэтому Snap работает. `WM_GETMINMAXINFO` теперь максимизирует
в `rcWork` текущего монитора; `cppui.chrome` проверяет, что maximized client
совпадает с work area. Урок с `SetCapture` уже оплачен (коммит `6c891c4`).
Слабые места: theme/action-кнопки принудительно отдаются как `HTCLIENT`;
touch/pen (`WM_POINTER`) не обрабатывается; Win10 и auto-hide taskbar
проверены не на всех конфигурациях.

**Триггер.** Кнопка «залипает» после drag; Snap не открывается на втором
мониторе; maximized окно вылезает за край на 1–2 px; после смены DPI
hit-area смещена; Alt+Space открывает меню не там.

**Что делать.** Матрица Win10/Win11 × 100–200 % × один/два монитора × RDP ×
touch. Зафиксировать «мышь/клавиатура в MVP» или добавить `WM_POINTER`.
Каждый ручной кейс из [M0.md](M0.md) — пункт чек-листа с датой.

**Критерий закрытия.** Chrome-матрица автоматизирована на реальных 150/200 %
(сейчас simulated); Win10 явно best effort; нет P1 по chrome.

### R-02. Device loss / RDP / слабая GPU — M/M, Partial

**Механизм.** D3D11-устройство исчезает при TDR, обновлении драйвера, RDP,
сне, смене GPU. Нужно пересоздать swap chain, target и ресурсы и не уйти в
цикл пересоздания.

**Где.** [d2d_renderer.cpp](../src/render/d2d/d2d_renderer.cpp):
`RecreateDevice` пересоздаёт всё, `CreateDeviceResources` падает на WARP,
если hardware не создался; [form.cpp](../src/form.cpp) вызывает recreate при
ошибке рендера.

**Дыры и прогресс.** WARP выбирается, если hardware не создался, и после
**3 подряд render failures** (`consecutive_render_failures_`). Порог 10 с
остановкой рендера и backoff пока не сделаны. Реальный TDR/RDP reconnect
не воспроизводился — только принудительный `F5` и `CPPUI_FORCE_WARP=1`.

**Триггер.** Чёрное окно после RDP; CPU в idle не ноль; мигание при
перетаскивании между GPU; падение при сворачивании.

**Что делать.** Добавить порог 10 → остановка рендера с диагностикой и backoff.
Recreate по `WM_DISPLAYCHANGE`, `WM_DPICHANGED`, `WM_POWERBROADCAST`.
Soak: 100 recreate с замером private memory (база M0 — 43.8 MB). Ручной
сценарий RDP disconnect/reconnect.

**Критерий закрытия.** 100 recreate без роста памяти и падений; RDP-сессия
переживает reconnect; в логе виден выбранный путь.

### R-03. Scope creep из-за большого списка контролов — H/H, Mitigated

**Механизм.** В [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md) ~69 позиций. v0.1 —
около 10 контролов плюс пять подсистем (Command, Popup, Scroll, Validation,
Image/Icon). По Definition of Done контрол — это 3 темы × 3 density × 4 DPI ×
набор состояний.

**Триггер.** Контрол «готов» без клавиатуры/UIA; PopupMenu и Dialog
появляются раньше `PopupHost`; TODO растут быстрее тестов; «ещё один простой
контрол».

**Что делать.** Заморозить список v0.1 письменно. Сначала подсистемы, потом
контролы. Если инфраструктура не готова — резать `PopupMenu`/`Dialog` в
v0.2, а не качество. Прогресс считать по закрытым пунктам DoD, не по строкам.

**Критерий закрытия.** v0.1 закрыт ровно по списку; ни одного data-контрола.
Статус 15.11.2026: v0.1 закрыт без data-контролов, scope list исполнен; остаточный
риск — расширение v0.2.

### R-04. TextEdit/MemoEdit: IME, selection, undo — H/M, Partial

**Механизм.** Модель документа, каретка по графемам, выделение мышью за
пределами окна, double/triple click, shift+стрелки, clipboard, undo-группы,
read-only, валидация, скролл, IME-композиция и позиционирование окна
кандидатов, доступность. Исторически самый дорогой контрол.

**Где.** v0.1: `input.cpp`/`input.hpp`; spike и тесты — `cppui.v01`.

**Триггер.** TextEdit поверх `IDWriteTextLayout` без своей модели; IME-кандидат
в углу экрана; undo по символу; вставка 10 МБ текста роняет кадр; каретка
между суррогатной парой.

**Что делать.** Spike 2–3 дня: headless `TextDocument` (UTF-16 внутри, UTF-8 на
границе), тесты на суррогатные пары/combining/selection/undo; IME через
`WM_IME_*` + `ImmSetCandidateWindow`; clipboard `CF_UNICODETEXT`; Memo —
виртуализация строк отдельной волной.

**Критерий закрытия.** Русский и китайский ввод, clipboard, undo/redo,
стрелки/shift/Home/End, выделение за пределами окна, read-only — в тестах и
ручной IME-матрице.

**Статус 15.11.2026.** Selection, clipboard, undo/redo, Home/End, read-only,
IME result string и candidate window у caret реализованы (`cppui.v01`).
Остаются: inline composition preview, проверка на CJK-IME, double/triple click,
selection за пределами окна, Memo. Поэтому `Partial`, не `Mitigated`.

### R-05. PropertyGrid без рефлексии — H/M, Open

**Механизм.** Публичный контракт `PropertyDescriptor` + адаптеры + фабрика
редакторов; ошибка в нём ломает API пользователя. Плюс mixed values,
категории, сортировка, вложенность, валидация, undo, локализация имён.

**Триггер.** API дескриптора меняется под каждый тип; каждый редактор —
отдельный класс; mixed value не поддержан с начала; `VerticalGrid` и
`PropertyGrid` пишутся параллельно.

**Что делать.** Зафиксировать контракт до реализации и проверить на двух
реальных доменных типах. Решить O-005 до старта волны. Заморозить API после
v0.3.

**Критерий закрытия.** int/string/enum + nested + mixed + validation + undo на
реальной форме; API не менялся с заморозки.

### R-06. Визуальная несогласованность — M/M, Partial

**Механизм.** Хардкод цветов/размеров, неполные токены, непроработанные
комбинации состояний, DPI-округление, шрифтовой фолбэк.

**Где.** [tokens.hpp](../include/cppui/theme/tokens.hpp), [theme.cpp](../src/theme/theme.cpp),
`cppui.visual` (M0) и `cppui.visual_v01` (Form Starter Light/Dark/Compact/Popup)
с допуском 8/255 и 2 % пикселей. Допуск в 2 % от мегапикселя всё ещё скрывает
небольшой сдвиг контрола.

**Триггер.** RGB/hex вне `theme.cpp`; контрол иначе выглядит в dark; отступы
«плывут» на 150 %; hover+focus+disabled вместе не заданы.

**Что делать.** Линт на цветовые литералы вне theme-слоя; baseline-матрица
(тема × density × состояния, минимум 12); дизайн-ревью каждого контрола;
проверять геометрию/hit-area, а не только пиксели.

**Критерий закрытия.** Нет хардкода, baseline покрывает заявленные состояния,
регресс ловится автоматически.

### R-07. Отсутствие/хрупкость toolchain — L/L, Mitigated

**Механизм.** Локально VS 2026, на CI — `windows-2022` с VS 2022; CMake 4.3
против версии раннера; на локализованной MSVC Ninja ломает `/showIncludes`
([D-027](DECISIONS.md)); Catch2 упомянут в плане, но тесты — собственные exe.

**Триггер.** CI падает на configure; инкрементальная сборка перестала
работать; «у меня собирается»; свежая машина требует ручных шагов.

**Что делать.** Прогнать workflow удалённо (R-09). Пиннить версии CMake/VS/SDK
в README. Проверять оба семейства пресетов. Держать `tools/build.ps1`
единственной точкой входа.

**Критерий закрытия.** Зелёный удалённый run; сборка с чистой машины по README
без ручных правок.

### R-08. Оптимизм в оценках — H/M, Open

**Механизм.** M0 по истории git занял ~14 часов и ~950 строк в `src+include`
при плане 20–27 дней. Это узкий знакомый срез, а не база для прогноза 6–9
месяцев: текст, виртуализация, UIA, IME и PropertyGrid ещё не написаны.

**Триггер.** «Осталось только добавить контролы»; оценка контрола в один день;
нет лога фактических часов; ROADMAP не пересматривался после M0.

**Что делать.** Reference-class forecasting: после v0.1 разделить фактические
часы на число закрытых пунктов DoD и экстраполировать остаток. Логировать часы
на контрол. Буфер 30–50 %. Пересмотреть [ROADMAP.md](ROADMAP.md) после v0.1.
Резать объём, не качество фундамента.

**Критерий закрытия.** Пересмотренный ROADMAP на фактических данных.

### R-09. Удалённый CI ни разу не выполнялся — H/M, Blocked

**Механизм.** Workflow есть, но в репозитории нет git remote, и ни один run не
запускался. Пути артефактов, пресеты, PowerShell-тесты на раннере с другим DPI
и шрифтами не проверены.

**Триггер.** Первый push/PR; падение configure; visual-тест на WARP-раннере.

**Что делать.** Добавить remote и прогнать оба workflow (`windows.yml`,
`ci.yml`) на PR. Зафиксировать ссылку на зелёный run в README или M0.
Сохранять visual-output при падении (уже настроено в `windows.yml`).

**Критерий закрытия.** G-3: зелёный удалённый run Debug и Release.

### R-10. Performance-бюджеты не были gate — M/M, Mitigated

**Механизм.** `--benchmark` писал метрики и всегда возвращал 0, поэтому
деградация проходила CTest незамеченной.

**Что делать / сделано.** G-1: `cppui.benchmark` запускает benchmark через
`tests/benchmark_gate.ps1` и проверяет Release-пороги; Debug пропускает
пороги, но требует успешного прогона.

**Остаток.** Пороги абсолютные и намеренно грубые (~5× от baseline). Они ловят
регресс на порядок, а не дрейф 20 %. Для дрейфа нужен тренд по артефактам.

**Критерий закрытия.** G-1 зелёный в Release; при падении видно, какая метрика
и на сколько вышла за предел.

### R-11. Visual-тесты хрупкие по конструкции — M/M, Partial

**Механизм.** PowerShell + `System.Drawing` + WIC + сравнение PNG, снятых на
конкретной GPU/DPI/шрифтах. На WARP или другом раннере антиалиасинг текста
отличается.

**Что делать / сделано.** `cppui.visual_v01_warp` запускает Form Starter под
`CPPUI_FORCE_WARP=1`; WARP проходит текущие baseline с расхождением <0.1%.
Оба visual-скрипта при превышении допуска сохраняют `diff-*.png` в
`visual-output`, который CI загружает при падении. Baseline снимаются при
фиксированном 96-DPI capture-профиле.

**Остаток.** Удалённого CI и второго шрифтового окружения нет; структурные
hit-area проверяются отдельно в `cppui.chrome`, а не внутри visual-скрипта.

**Критерий закрытия.** Visual-тест стабилен на двух разных конфигурациях
(локально и CI) без обновления baseline.

### R-12. DPI awareness не выставлялся библиотекой — M/H, Mitigated

**Механизм.** `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)` вызывался
только в `samples/bootstrap/main.cpp`. Пользователь публичного `Form` получал
DPI-виртуализацию и сломанную геометрию молча.

**Что делать / сделано.** G-2: публичный `cppui::IsPerMonitorDpiAware()` в
[form.hpp](../include/cppui/form.hpp); `Form::Create` пишет диагностику в
debugger output при отсутствии PMv2; тест `cppui.dpi_awareness` проверяет оба
направления. Требование задокументировано в заголовке.

**Остаток.** Библиотека не выставляет awareness за приложение — это осознанно:
глобальный вызов должен делать хост до создания окон. Проверка читает контекст
текущего потока.

**Критерий закрытия.** G-2 зелёный; в README/API явно сказано, что хост
обязан включить PMv2.

### R-13. UIA/accessibility отложен архитектурно — M/H, Partial

**Механизм.** Keyboard/focus есть, AutomationPeer-слоя нет. Для кастомно
нарисованных контролов UIA — отдельная подсистема, которую дорого добавлять
после релиза.

**Что сделано.** В `Widget` добавлены стабильные accessible name/role/value;
Label, Button, TextEdit, CheckBox, RadioButton, ProgressBar, GroupPanel,
PictureBox, PopupMenu и ScrollViewer заполняют metadata. Это контракт для
будущего AutomationPeer, но не UIA-провайдер.

**Остаток.** Нет `IRawElementProviderSimple`/AutomationPeer, Narrator/Inspector
теста и полного набора actions. Решить тайминг UIA в O-008.

**Критерий закрытия.** Базовый UIA-провайдер для Button/TextEdit; клавиатурная
навигация без мыши проходит сценарий формы.

### R-14. Лицензия и публичность репозитория — M/H, Mitigated

**Механизм.** Без выбранной лицензии публичный push юридически запрещён: у
пользователей нет прав на использование кода.

**Что сделано.** 15.11.2026 владелец выбрал MIT. Добавлен [LICENSE](../LICENSE),
раздел README и решение O-002 в [DECISIONS.md](DECISIONS.md) обновлены. Проверено
ранее: в репозиторий не добавлены чужие шрифты (используется системный Segoe UI).

**Остаток.** Лицензию может пересмотреть владелец до v1.0; при добавлении
сторонних ассетов нужен их лицензионный аудит.

**Критерий закрытия.** LICENSE в корне; публичный push после решения. — выполнено.

### R-15. Концентрация знаний на одном человеке — L/H, Accepted

**Механизм.** Один senior-разработчик; решения живут в документах, но не в
коде. При остановке проект теряет скорость восстановления контекста.

**Что делать.** ADR-дисциплина (уже есть), «карта кода» в ARCHITECTURE, тесты
как спецификация, короткие записи «почему так» в PR/коммитах.

**Критерий закрытия.** Новый разработчик собирает и запускает проект по
README за один день без устных объяснений.

## 4. Gate-проверки

### G-1. Performance budgets (реализовано)

`cppui.benchmark` → [tests/benchmark_gate.ps1](../tests/benchmark_gate.ps1).

| Метрика | Предел | Baseline Release 15.11.2026 |
|---|---:|---:|
| `frame_cpu_avg_ms` | 10.0 | 1.3–2.9 |
| `frame_p95_ms` | 45.0 | 8.4–8.6 |
| `layout_avg_ms` | 1.5 | 0.12 |
| `resize_avg_ms` | 25.0 | 7.7–7.9 |
| `startup_to_first_frame_ms` | 600.0 | 116–182 |
| `theme_switch_avg_ms` | 20.0 | 8.0 |
| `density_switch_avg_ms` | 20.0 | 8.3 |

Release проверяет пороги, Debug — только успешность прогона. Запуск:

```powershell
cmake --build --preset windows-vs2026-release
ctest --preset windows-vs2026-release -R cppui.benchmark --output-on-failure
```

### G-2. DPI awareness (реализовано)

`cppui.dpi_awareness` → [tests/dpi_awareness_tests.cpp](../tests/dpi_awareness_tests.cpp).

Проверяет, что `cppui::IsPerMonitorDpiAware()` различает unaware-процесс и
PMv2, и что `SetProcessDpiAwarenessContext` переводит процесс в PMv2.
`Form::Create` предупреждает в debugger output, если хост забыл awareness.

### G-3. Удалённый CI (заблокирован)

Нужен git remote и первый push. До этого workflow считается непроверенным:
[windows.yml](../.github/workflows/windows.yml) — VS 2022 Debug/Release,
[ci.yml](../.github/workflows/ci.yml) — Ninja Debug. Оба запускают CTest,
поэтому G-1 и G-2 подключаются автоматически.

## 5. Журнал ревизий

| Дата | Изменение |
|---|---|
| 15.11.2026 | Реестр создан; R-10 и R-12 закрыты gates G-1/G-2; R-09 переведён в Blocked (нет remote); подтверждены 11/11 CTest в Debug и Release |
| 15.11.2026 | Полная ревизия 15 рисков. R-01: maximize в work area + тест. R-02: 3 render failures → WARP. R-11: WARP visual gate + diff PNG. R-13: accessible name/role/value в Widget. R-03/R-07/R-10/R-12 — Mitigated; R-05/R-08/R-14 — Open; R-09 — Blocked; R-15 — Accepted. **14/14 CTest в Debug и Release**. |

## 6. Ближайшие действия

1. **R-09:** добавить remote и получить первый зелёный run — это разблокирует
   доверие ко всем остальным gate.
2. **R-14:** владельцу выбрать лицензию; без этого публичный push запрещён.
3. **R-13:** spike UIA AutomationPeer для Button/TextEdit и Narrator-проверка.
4. **R-02:** TDR/RDP soak и порог 10 → остановка рендера с диагностикой.
5. **R-08:** пересчитать ROADMAP по фактическим часам после v0.1.
6. **R-05:** контракт PropertyDescriptor до старта v0.3.
