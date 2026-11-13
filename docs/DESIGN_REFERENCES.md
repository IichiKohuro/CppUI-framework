# DESIGN_REFERENCES — что взять из `design_chats`

> Статус: decisions applied · 14.11.2026.
> **Принято:** Signal Teal как основной визуал, density-режимы, data-workspace demo, inline editing и row states.
> Связанные документы: [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md), [PLAN.md](../PLAN.md).

## 1. Что было проанализировано

В `design_chats/` лежат 11 экспортов дизайн-чатов. Это не готовые исходники приложения, а JSON с сообщениями и фрагментами Design Components / HTML / JSX.

- **9 чатов содержат материалы**, 2 — пустые (`Nocturne`, `Classical`).
- Два Blender-чата (`Blender-стиль приложение`, `C# AvaloniaUI Blender интерфейс`) во многом дублируют друг друга.
- В JSON **нет самих изображений** — только ссылки на `uploads/*.png`. Скриншоты, которые видел дизайн-агент, недоступны.
- Часть финальных файлов и внешних дизайн-систем (`_ds/classical`, `Nocturne`) в экспорте отсутствует.
- Выводы сделаны по исходному коду, токенам, текстам запросов и описаниям дизайнов, а не по финальным рендерам.

Ничего брендированного (Blender, существующий Protection Settings) не копируем 1:1. Берём **дизайн-ДНК**: плотность, токены, композицию и поведение.

## 2. Короткий вывод

Лучшие источники для CppUI:

1. **Build Manager Tray App** (ноябрь 2026) — самый свежий и цельный компактный desktop-дизайн: teal-палитра, Golos Text + JetBrains Mono, 30-пиксельные icon-кнопки, tooltip, toast.
2. **Редактор Метабазы XML** — лучший пример data-плотного рабочего места: 3 панели, IBM Plex Sans/Mono, indigo-акцент, compact/comfortable density, поиск с подсветкой, dirty/validation/undo.
3. **Lite UI** — лучший источник для density-системы и таблично-инженерного светлого интерфейса: 27/34/40 px строки, синий `#2F6FB0`, валидация, differences, undo/save.
4. **Protection Settings + TreeList с CheckedListBox** — лучший источник **поведения** для контролов: inline-редакторы в ячейке, toolbar с badge, фильтрация TreeList через CheckedListBox, каскадное скрытие групп.
5. **Blender-чаты** — только как источник мрачной плотной эстетики и row-states. Брендинг Blender, иконки и точную композицию не переносим.

**Решение:** выбран **Signal Teal** — самая свежая и цельная палитра из Build Manager. Density-режимы, data-workspace demo, inline editing и row states приняты и уже отражены в [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) и в [галерее](../design/controls-gallery.html). Studio Indigo и Graphite остаются возможными будущими пресетами, но не входят в текущий визуал.

## 3. Аудит чатов

| Проект | Дата | Ценность | Что взять |
|---|---|---|---|
| **Build Manager Tray App** | 04.11.2026 | Высокая | Teal-палитра, Golos Text + JetBrains Mono, компактный popover, tooltip/toast, фильтры, status-футер |
| **Редактор Метабазы XML** | 25.06.2026 | Высокая | 3-панельный workspace, IBM Plex, indigo, density, поиск с подсветкой, dirty + validation + undo |
| **Lite UI** | 14–25.06.2026 | Высокая | Density 27/34/40, светлая инженерная палитра, toolbar, таблица, validation, differences |
| **UI с тулбаром защит** | 08.09.2026 | Высокая (поведение) | Badge-toolbar, inline ComboBox/SpinEdit в ячейке, style-token пресеты, TreeList + фильтры |
| **TreeList с CheckedListBox** | 08.09.2026 | Высокая (локально) | Фильтрация дерева чек-листом, скрытие пустых групп, resizable-панель |
| **Blender темная тема** | 27.06.2026 | Средняя | Плотный graphite-интерфейс, синий акцент, toast/confirm, row-actions |
| **Blender-стиль приложение** | 01.07.2026 | Средняя | Многотабличный data-экран, журналы/статусы, loading per row; визуал Blender не копировать |
| **C# AvaloniaUI Blender** | 29.06.2026 | Низкая/средняя | Дублирует Blender-направление; полезен только как пример логов/таблицы |
| **Dusk** | 04.08.2026 | Низкая для CppUI | Тёплый amber-акцент на глубоком charcoal; mobile-only, только как идея будущей night-темы |
| **Nocturne / Classical** | 08.2026 / 09.2026 | Нет | Пустые экспорты; Classical использовался как внешняя дизайн-система, но её файлов нет |

## 4. Что конкретно можно перенести

### 4.1 Density-режимы

Самый практичный и дешёвый для внедрения паттерн. Он встречается в **Lite UI** и **XML Editor** и хорошо ложится на CppUI.

| Режим | Строка | Контрол | Базовый шрифт | Источник |
|---|---:|---:|---:|---|
| Comfortable | 36 px | 32 px | 11.5 px | адаптация Lite UI `comfortable` |
| Cozy (default) | 30 px | 28 px | 10.5 px | адаптация Lite UI `cozy` |
| Compact | 25 px | 25 px | 9.5 px | адаптация Lite UI `compact`, XML editor |

Принято: `DensityMode { Comfortable, Cozy, Compact }` хранит row height, control height, paddings и font size. В HTML это реализовано для workspace; глобальное применение ко всем контролам — следующий шаг реализации.

### 4.2 Типографика

| Профиль | Пары | Где брать | Комментарий |
|---|---|---|---|
| **Native** | Segoe UI / Segoe UI Variable + Cascadia Mono | текущий CppUI | Ничего не бандлим, нативный вид, системная доступность |
| **Technical** | IBM Plex Sans + IBM Plex Mono | XML Editor | Лучший вариант для data/IDE-подобных приложений, полная кириллица |
| **Utility** | Golos Text + JetBrains Mono | Build Manager | Современный компактный характер, хорошо работает с teal |
| **Mono-accent** | Noto Sans + Noto Sans Mono | Blender-чаты | Допустим для «графитового» варианта, но не обязателен |

Шрифты IBM Plex, Golos Text и JetBrains Mono распространяются по открытым лицензиям (перед бандлингом проверить актуальные OFL-условия). Для CppUI это означает загрузку font assets в репозиторий и регистрацию через DirectWrite, а не системную зависимость.

Рекомендация: **Segoe UI оставить default**, а IBM Plex/Golos добавить как опциональные font-профили после MVP. Иначе стартовая сборка и лицензионная гигиена усложнятся раньше времени.

### 4.3 Палитры

| Направление | Ключевые цвета | Источник |
|---|---|---|
| **Native Blue** (предыдущий концепт) | accent `#3B6CF6` / `#6D9BFF`, surface `#FFFFFF` / `#171D26`, bg `#F4F6F9` / `#0C1016` | прежний gallery, не выбран |
| **Signal Teal** ✅ | light: accent `#00705F`, surface `#F4F8F6`, fg `#16201E`, line `#D8E4DF`; dark: accent `#3BB39E`, bg `#0C1211`, surface `#1A2120`, fg `#E4ECEA`, line `#2C3735` | Build Manager, выбран |
| **Studio Indigo** | accent `#4F46E5`, cool-gray surfaces, IBM Plex, light/dark пары на oklch | XML Editor, возможный будущий пресет |
| **Graphite** | bg `#1B1B1B`/`#2B2B2B`, surface `#303030`, border `#141414`, text `#E4E4E4`, accent `#5E88C9`/`#4772B3` | Blender-чаты, только идея |

Отдельно: **Protection Settings** содержит набор OS-подобных токенов — Fluent (`#0F6CBD`), Windows 11 (`#005FB8`), macOS (`#007AFF`), Material (`#6750A4`), Classic (тёплый крем/жёлтый). Это хороший пример того, как один и тот же UI переключается токенами, но **не рекомендация** делать пять OS-клонов в CppUI.

Что уже взято:

- **Signal Teal** стал основным визуалом CppUI;
- фон/поверхности/текст адаптированы под teal, а не скопированы у Build Manager;
- **Segoe UI** пока остаётся системным шрифтом; Golos/IBM Plex — открытые варианты на будущее;
- **density S/M/L** и **workspace с inline editing/row states** добавлены в gallery;
- **Graphite** не тащим: он остаётся идеей для отдельной тёмной темы без Blender-брендинга.

### 4.4 Композиция и метрики

Из XML Editor:

- title bar 34 px, menu bar 28 px, toolbar 42 px;
- side panel 320 px;
- 3 области: дерево → инспектор → raw source;
- bottom dock с вкладками «Проверка / История»;
- compact rows 24 px, comfortable 29 px;
- resizable splitter'ы.

Из Lite UI:

- title bar 46 px, toolbar 42 px, sidebar 320 px;
- row 34 px в cozy;
- поиск, фильтр Differences, status connection pulse;
- плотная таблица с inline validation.

Из Build Manager:

- компактный popover 400 × 560;
- 30 px icon-кнопки с tooltip;
- сегментированные фильтры;
- status footer с connection dot;
- toast и confirm.

Для CppUI это подтверждает текущий курс: **кастомный title bar + toolbar/menu + resizable side pane + content + statusbar**. Новое — density и более выраженный data-workspace как отдельный demo/компонент, а не как часть простой формы.

### 4.5 Поведения, которые стоит занести в backlog

1. **Inline-редактор ячейки** — ComboBox/SpinEdit открывается только по клику и занимает всю ячейку без отступов; после выбора текст возвращается в режим отображения.
2. **TreeList + CheckedListBox** — отмеченные элементы показываются/скрываются в дереве; родительские группы скрываются, если в них не осталось видимых детей.
3. **Badge-toolbar** — кнопки-фильтры с количеством и состоянием on/off, группировка по категориям.
4. **Row lifecycle** — loading/progress per row, новые элементы, ошибка (красный фон строки), изменённые значения (жирный текст), freshness/status icon.
5. **Поиск с подсветкой** найденного фрагмента в дереве.
6. **Dirty/validation/undo/save** — индикатор несохранённых изменений, «Differences», проблемы валидации, отмена/повтор.
7. **Toast и confirm-диалог** — ненавязчивое подтверждение и защита деструктивных действий.
8. **Resizable panes** — изменение ширины/высоты панелей с сохранением пользовательского размера.
9. **Tooltip для icon-only кнопок** — обязательный паттерн из Build Manager; у нас уже заложен в требованиях.

Куда это ложится:

- v0.1 — toast/confirm, density, tooltips, dirty/validation states;
- v0.2 — inline-редакторы, search highlight, resizable panes;
- v0.3 — tree+checklist filtering, per-row loading/error/freshness, differences.

## 5. Что не брать

- **Брендинг и ассеты Blender**, иконки/композицию 1:1.
- **Точный визуал Protection Settings** — это проприетарный интерфейс с чужого скриншота; полезны только паттерны.
- **Пять OS-клонов** (Material/Fluent/macOS) в MVP — размывает identity и увеличивает QA.
- **Мобильную сетку Dusk** — CppUI desktop-first.
- **Web/React-код** дизайнов как архитектуру C++ — только токены и UX-идеи.
- **Фиксированные ширины 1376/1400 px** — в CppUI нужен DIP/layout без фиксированной ширины окна.
- **Все три шрифтовые пары сразу** — выбрать одну дополнительную максимум.

## 6. Что сделано и что дальше

Сделано в дизайне:

1. **Signal Teal** применён к форме и галерее контролов.
2. Density S/M/L добавлена в data workspace и зафиксирована в DESIGN_SYSTEM.
3. Собран data-workspace demo: TreeList + GridView + PropertyGrid + toolbar + bottom dock.
4. Добавлены row states и inline editing с меню статуса.
5. Обновлены галерея, README и docs.

Дальше:

1. Перенести density и interaction patterns в C++ `DensityMode` и `GridView`/`PropertyGrid` API.
2. Решить, остаётся ли Segoe UI или добавляем IBM Plex/Golos как опциональный font-профиль.
3. Включить TreeList + CheckedListBox filtering в data-demo следующей волны.
4. Проверить inline editing на клавиатуре, IME и accessibility.
5. Не добавлять пять OS-клонов и Blender-брендинг.

## 7. Открытые вопросы

1. Бандлить ли IBM Plex/Golos или оставаться на Segoe UI?
2. Когда делать density глобальным для всех контролов, а не только workspace?
3. Включать ли TreeList + CheckedListBox фильтрацию в v0.2 или v0.3?
4. Нужен ли дополнительный Graphite/Studio theme preset после v0.1?
5. Нужна ли настройка accent color поверх Signal Teal?

## 8. Источники

- [Build Manager Tray App](../design_chats/f53e75a7-f746-4331-9dc1-3e6d6aa5154e.json) — самый свежий визуальный референс.
- [Редактор Метабазы XML](../design_chats/c32244a4-aad2-4837-a1f9-47b420f86b4e.json) — техническая плотная структура.
- [Lite UI](../design_chats/0d8214b1-95fb-444d-9403-b0b79f1ff7a0.json) — density и инженерная таблица.
- [UI с тулбаром защит](../design_chats/9ddb82e1-0548-4a53-970d-bbef8b44328f.json) — inline-редакторы, badge-toolbar, темы.
- [TreeList с CheckedListBox](../design_chats/eab98614-5539-42d7-aef4-feeeb8c0ddac.json) — фильтрация дерева.
- [Blender темная тема](../design_chats/c4f69817-6fb7-42cf-be28-59e498ecaae0.json) и [Blender-стиль](../design_chats/2843b374-2a09-40d9-9bcb-b3e1f79c43c2.json) — только dark/density-вдохновение.
- [Dusk](../design_chats/ceabe4c6-4970-4f5d-83f7-965b87400248.json) — mobile, вне основного scope.
