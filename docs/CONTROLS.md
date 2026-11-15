# CONTROLS — публичный API v0.1

> Статус: v0.1 Form Starter · 15.11.2026.
> Обзор волны — [V01.md](V01.md); общий план — [ROADMAP.md](ROADMAP.md).

Страница описывает контролы, которые уже есть в `include/cppui/ui/`. Контролы v0.2
(`MemoEdit`, `ComboBox`, `TabControl`, `SplitContainer`, …) здесь намеренно отсутствуют.

## 1. Общие правила

### 1.1 Дерево и владение

Все контролы — потомки `cppui::Widget`:

```cpp
auto panel = std::make_unique<cppui::StackPanel>();
auto button = std::make_unique<cppui::Button>(L"OK", cppui::ButtonStyle::primary);
button->set_on_click([] { /* ... */ });
panel->add_child(std::move(button));  // panel владеет кнопкой
```

`add_child` забирает `unique_ptr`; родитель разрушает детей. Сырые указатели можно
хранить для обновления состояния, но нельзя освобождать вручную. `remove_child`
возвращает владение, если контрол нужно перенести.

Контракт layout:

```cpp
Size measure(Painter&, const Size& available);
void arrange(const Rect& bounds);
void paint(Painter&) const;
```

Размеры и координаты — в DIP. `Painter` — абстрактный контракт; публичный код не
работает с HWND, D2D или COM.

### 1.2 Видимость, tooltip, валидация

```cpp
widget.set_visible(false);
widget.set_tooltip(L"Подсказка через 600 мс");
widget.set_validation_error(L"Обязательное поле");
widget.clear_validation_error();
```

- Скрытый виджет не measure/arrange/paint и не попадает в Tab-порядок.
- Tooltip показывается общим `PopupHost`; для контролов без мыши (например `Label`) он
  включается автоматически, если tooltip задан.
- Ошибка валидации — состояние `Widget`; конкретный контрол решает, как её рисовать.
  `TextEdit` подсвечивает рамку. Текст ошибки обычно рисуется отдельным `Label`.

### 1.2.1 Accessibility metadata

`Widget` уже хранит name/role/value для будущего UIA-слоя:

```cpp
widget.set_accessible_name(L"Сохранить профиль");
widget.set_accessible_role(cppui::AccessibleRole::button);
widget.set_accessible_value(L"enabled");
```

Контролы v0.1 заполняют metadata автоматически (Label/Button text, TextEdit value,
CheckBox/RadioButton state, ProgressBar percent, GroupPanel title, Image placeholder).
Сам UIA AutomationPeer пока не реализован; это осознанный остаток R-13.

### 1.3 Фокус и клавиатура

- `Tab` / `Shift+Tab` — общий порядок.
- `Enter` / `Space` активируют `Button`, `CheckBox`, `RadioButton`.
- Стрелки переключают `RadioButton` внутри `RadioGroup`.
- `PopupMenu`: стрелки, Home/End, Enter/Space, Esc.
- `BusyOverlay` и открытый popup включают focus trap: фон не получает клавиатуру.
- `TextEdit`: стрелки, Home/End, Backspace/Delete, Ctrl+A/C/X/V/Z/Y.

### 1.4 DensityMode

```cpp
form.SetDensityMode(cppui::DensityMode::compact);  // compact / cozy / comfortable
```

| Mode | Control height | Padding | Radius | Min width |
|---|---:|---:|---:|---:|
| Compact | 28 | 10 | 6 | 64 |
| Cozy | 34 | 14 | 7 | 76 |
| Comfortable | 40 | 18 | 9 | 88 |

Все контролы v0.1 читают `Painter::density()` и пересчитываются при смене режима.

## 2. Текст

### `Label`

`#include <cppui/ui/widgets.hpp>`

```cpp
Label(std::wstring text, FontRole role = FontRole::body,
      LabelStyle style = LabelStyle::primary);
void set_text(std::wstring);
void set_wrap(bool);
```

`LabelStyle`: `primary`, `secondary`, `accent`, `muted`, `danger`.
Перенос по словам включается `set_wrap(true)`; в остальных случаях текст рисуется
одной строкой без ellipsis.

### `TextEdit`

`#include <cppui/ui/input.hpp>`

```cpp
TextEdit(std::wstring text = {}, std::wstring placeholder = {});
void set_text(std::wstring);
const std::wstring& text() const;
void set_placeholder(std::wstring);
void set_read_only(bool);
void set_max_length(std::size_t);
void set_on_text_changed(std::function<void(const std::wstring&)>);
void set_on_enter(std::function<void(const std::wstring&)>);
void select_all();
void set_caret(std::size_t index, bool extend_selection = false);
```

- Однострочный редактор с caret/anchor и Shift-выделением.
- Clipboard: Ctrl+C/X/V; системный UTF-16 clipboard через platform-слой.
- Undo/redo: Ctrl+Z/Y, лимит 100 снимков.
- Ввод IME: результат `WM_IME_COMPOSITION` и `WM_IME_CHAR` приходит в `on_char`.
- Caret прокручивается по горизонтали; валидация рисует danger-рамку.
- `set_on_enter` вызывается Enter, но новая строка не вставляется.

## 3. Кнопки и команды

### `Button`

```cpp
Button(std::wstring text, ButtonStyle style = ButtonStyle::secondary);
void set_text(std::wstring);
void set_icon(std::shared_ptr<Image>);
void set_command(std::shared_ptr<Command>);
void set_on_click(std::function<void()>);
void set_enabled(bool);
```

`ButtonStyle`: `primary`, `secondary`, `ghost`.
Иконка — `std::shared_ptr<Image>`, рисуется слева от текста. Если задан `Command`,
кнопка берёт его текст/enabled и вызывает `Command::Execute()`; кнопка обновляется при
следующем measure/paint, поэтому после смены состояния команды вызовите
`Form::Invalidate()`.

### `Command`

`#include <cppui/ui/command.hpp>`

```cpp
Command(std::wstring text = {}, std::function<void()> execute = {});
void set_text(std::wstring);
void set_shortcut(std::wstring);
void set_enabled(bool);
void set_checked(bool);
bool Execute() const;
```

`Command` — общий источник текста, shortcut и enabled/checked для `Button` и
`PopupMenu::add_command`. Наблюдателей нет: после изменения состояния перерисуйте хост.

## 4. Выбор

### `CheckBox`

```cpp
CheckBox(std::wstring text = {}, bool checked = false);
void set_checked(bool);
void set_state(CheckState);       // unchecked / checked / indeterminate
void set_tristate(bool);
void set_on_changed(std::function<void(CheckState)>);
```

Click и Space/Enter переключают состояние. При `tristate` цикл:
`unchecked → checked → indeterminate → unchecked`.

### `RadioButton` и `RadioGroup`

```cpp
RadioButton(std::wstring text = {}, bool checked = false);
void set_checked(bool);
void set_on_changed(std::function<void(bool)>);

RadioGroup(Orientation orientation = Orientation::vertical, float spacing = 8.0f);
void select(RadioButton*);
RadioButton* selected() const;
```

`RadioGroup` держит ровно один выбранный прямой дочерний `RadioButton`. Стрелки
переключают выбор; программный `set_checked(true)` синхронизирует группу.

## 5. Контейнеры и прокрутка

### `StackPanel`

```cpp
StackPanel(Orientation = vertical, float spacing = 0);
void set_padding(Insets);
void set_spacing(float);
void set_cross_align(CrossAlign);  // start / center / stretch
```

### `Panel`

```cpp
Panel(PanelStyle = surface, float corner_radius = 10);
void set_padding(Insets);
void set_style(PanelStyle);
void set_border(bool);
void set_clip_children(bool);
```

`PanelStyle`: `none`, `surface`, `soft`, `card`. Дети укладываются друг на друга внутри
padding; это базовая поверхность и контейнер для overlay.

### `GroupPanel`

```cpp
GroupPanel(std::wstring title, PanelStyle = surface, float corner_radius = 10);
void set_title(std::wstring);
```

Панель с заголовком, разделителем и контентной областью. Наследует `Panel`.

### `GridPanel`

```cpp
GridPanel();
void set_columns(std::vector<float> weights);
void set_rows(std::vector<float> weights);
void set_gap(float horizontal, float vertical);
void set_padding(Insets);
void set_stretch_rows(bool);
void add_cell(std::unique_ptr<Widget>, int column, int row,
              int column_span = 1, int row_span = 1);
```

- Положительный weight — star-колонка.
- Weight `0` — auto-sized колонка по содержимому (удобно для правой кнопки в header).
- Строки измеряются по контенту; `set_stretch_rows(true)` распределяет лишнюю высоту.

### `ScrollViewer`

```cpp
ScrollViewer(std::unique_ptr<Widget> content);
void scroll_by(float delta);
void scroll_to(float offset);
float scroll_offset() const;
float maximum_scroll() const;
```

Вертикальная прокрутка, wheel, clip и `ScrollBar`. Содержимое меряется с большим
доступным размером, поэтому подходит для длинных панелей. Горизонтальный скролл в v0.1
не включён; `TextEdit` прокручивает caret сам.

### `ScrollBar`

```cpp
ScrollBar(Orientation = vertical);
void set_range(float content_size, float viewport_size);
void set_value(float);
float value() const;
float maximum() const;
void set_on_scroll(std::function<void(float)>);
```

Обычно создаётся внутри `ScrollViewer`; публичный API нужен для собственных
scroll-контейнеров и тестов.

## 6. Индикаторы и overlay

### `ProgressBar`

```cpp
ProgressBar(float value = 0.0f);
void set_value(float);       // clamp 0..1
void set_show_text(bool);
```

### `ProgressRing`

```cpp
ProgressRing(float progress = -1.0f);  // <0 = indeterminate
void set_progress(float);
void set_diameter(float);
```

Анимируется только при `progress < 0`.

### `Loader`

```cpp
Loader(std::wstring text = {});
void set_text(std::wstring);
void set_ring_visible(bool);
```

Inline spinner + текст. `animating()` истинно, пока видим; `Form` запускает 16 мс timer
только при наличии анимации и останавливает его в покое.

### `BusyOverlay`

```cpp
BusyOverlay(std::wstring message = L"Working...");
void set_message(std::wstring);
void set_progress(float);       // <0 = spinner
void set_cancel_text(std::wstring);
void set_on_cancel(std::function<void()>);
```

Блокирует мышь, Tab и ввод. Добавляйте последним ребёнком `Panel` с `PanelStyle::none`
или поверх контента; `Panel` укладывает детей друг на друга.

## 7. Popup и tooltip

### `PopupHost`

```cpp
void open(std::unique_ptr<Widget> popup, Point position);
void close();
bool is_open() const;
Widget* popup() const;

void show_tooltip(std::wstring text, Point position);
void hide_tooltip();
```

В `Form` уже создан: `form.popups()->open(std::move(menu), point)`. Позиция — в DIP
относительно клиентской области. Popup зажимается в окно, закрывается по клику вне,
а Tab не уходит в фон. Tooltip пассивен: hit-testing идёт в контент под ним.

### `PopupMenu`

```cpp
PopupMenu(float minimum_width = 220);
void add_item(std::wstring text,
              std::function<void()> action = {},
              std::wstring shortcut = {},
              bool enabled = true,
              bool checked = false);
void add_command(std::shared_ptr<Command>);
void add_separator();
void set_checked(int index, bool checked);
void set_on_dismiss(std::function<void()>);
```

Пункты меню не являются отдельными виджетами; hover/selected хранится в меню. Клик по
пункту выполняет action и закрывает popup. Esc закрывает. Вложенные подменю — v0.2.

### `ToolTip`

```cpp
ToolTip(std::wstring text = {});
```

Обычно не создаётся вручную: задайте `Widget::set_tooltip`, а `Form` покажет подсказку
через 600 мс. Позиция — у курсора, мышью не перехватывается.

## 8. Окна и сообщения

### `Dialog`

`#include <cppui/ui/dialog.hpp>`

```cpp
Dialog(std::wstring title, int width_dip = 460, int height_dip = 220);
void SetContent(std::unique_ptr<Widget>);
void AddButton(std::wstring text, DialogResult result,
               ButtonStyle style = ButtonStyle::secondary);
void SetDefaultResult(DialogResult);
DialogResult ShowModal(Form& owner);
DialogResult ShowModal(void* owner_handle = nullptr);
void Close(DialogResult result = DialogResult::cancel);
```

`DialogResult`: `none`, `ok`, `cancel`, `yes`, `no`.
Модальный цикл отключает owner, возвращает фокус после закрытия, X/Esc дают `cancel`,
Enter активирует default-кнопку. Тема и density берутся из `Form` при `ShowModal(Form&)`.
В title bar диалога остаются только системные caption-кнопки: theme switch и action
скрыты намеренно.

### `MessageBox`

```cpp
enum class MessageBoxButtons { ok, ok_cancel, yes_no, yes_no_cancel };
struct MessageBoxLabels { std::wstring ok, cancel, yes, no; };
DialogResult MessageBox::Show(Form& owner,
                              std::wstring title,
                              std::wstring message,
                              MessageBoxButtons = MessageBoxButtons::ok,
                              const MessageBoxLabels& = {});
```

Готовый диалог поверх `Dialog`; подписи кнопок можно локализовать. Текст сообщения
переносится по словам.

## 9. Изображения

### `Image`

```cpp
static std::shared_ptr<Image> LoadFromFile(const std::wstring& path,
                                           std::wstring* error = nullptr);
static std::shared_ptr<Image> FromPixels(int width, int height,
                                         std::vector<std::uint8_t> bgra_premultiplied);
int width() const;
int height() const;
bool valid() const;
```

WIC загружает PNG/JPEG в premultiplied BGRA. Изображение immutable и может
разделяться между контролами; renderer кеширует D2D bitmap по `Image::id()`, поэтому
анимация не пересоздаёт текстуру на каждом кадре.

### `PictureBox`

```cpp
PictureBox(std::shared_ptr<Image> = {});
void set_image(std::shared_ptr<Image>);
void set_fit(ImageFit);          // contain / cover / stretch
void set_preferred_size(Size);
void set_corner_radius(float);
void set_placeholder(std::wstring);
```

## 10. Форма и статус-бар

```cpp
Form(std::wstring title, int width_dip, int height_dip,
     int min_width_dip = 900, int min_height_dip = 580);
bool Create();
void SetContent(std::unique_ptr<Widget>);
StatusBar* status_bar();
PopupHost* popups();
void SetStatusText(std::wstring);
void SetThemeMode(ThemeMode);
void SetDensityMode(DensityMode);
void Show();
int Run();
bool CapturePng(std::wstring_view path);
```

`StatusBar` — часть `Form`; `set_left_text`/`set_right_text` доступны через
`form.status_bar()`. `CapturePng` синхронно рисует текущий кадр в PNG (используется
визуальными тестами). `NativeHandle()` — escape hatch для Win32-интеграции.

Перед созданием формы хост должен включить Per-Monitor V2:
`SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`.
Form Starter делает это сам; библиотека не меняет глобальный DPI-контекст молча.

## 11. Ограничения и lifetime

- Callback’и хранятся в виджетах; не захватывайте объекты, которые могут умереть раньше.
- `Form` владеет деревом; после `SetContent` старые указатели недействительны.
- `Dialog::SetContent` и `AddButton` предназначены для вызова до `ShowModal`.
- `PopupHost::open` забирает popup; после закрытия объект уничтожается отложенно,
  чтобы callback пункта меню успел завершиться.
- IME: финальный текст и позиция candidate window; inline composition preview
  остаётся системным.
- UIA, high contrast и SVG — вне v0.1.
