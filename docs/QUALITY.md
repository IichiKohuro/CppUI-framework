# QUALITY — тесты, производительность и критерии релиза

> Статус: Draft · 14.11.2026.
> Связанные документы: [PLAN.md](../PLAN.md), [ROADMAP.md](ROADMAP.md), [ARCHITECTURE.md](ARCHITECTURE.md), [CONTROL_BACKLOG.md](../CONTROL_BACKLOG.md).

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

- создание hidden window и message loop;
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

- [ ] Все заявленные контролы/сценарии реализованы.
- [ ] Unit- и integration-тесты зелёные.
- [ ] Visual snapshots обновлены и просмотрены.
- [ ] Ручная матрица chrome/DPI пройдена.
- [ ] Проверены Compact / Cozy / Comfortable в data-layout.
- [ ] Нет известных P0/P1.
- [ ] Performance-бюджеты соблюдены.
- [ ] Документация и примеры обновлены.
- [ ] Accessibility-минимум пройден.
- [ ] Сборка с чистой машины воспроизводится.

## 5. Performance-бюджеты

Это стартовые цели, которые нужно подтвердить на реальном железе после Phase 1. Значения не являются обещанием для любой конфигурации.

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
| Память демо | < 40 MB | < 60 MB |
| Переключение темы | 1 кадр | ≤ 100 ms |

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

Планируется GitHub Actions (после инициализации репозитория):

- Windows runner, MSVC x64.
- Debug и Release.
- Configure/Build через CMake presets.
- Unit- и integration-тесты через CTest.
- Visual snapshots с artifacts.
- `clang-format --dry-run --Werror`.
- Warnings as errors для собственного кода.
- Сборка sample.
- ASan job для Debug (MSVC).
- Кеширование CMake/Ninja, если уместно.

CI не может полноценно проверить Snap Layouts, multi-monitor DPI и реальный WARP; эти пункты остаются в ручной матрице.

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

- [ ] Все запланированные контролы прошли Definition of Done.
- [ ] Проверены Light/Dark/System.
- [ ] 100/125/150/200% DPI и mixed-DPI.
- [ ] Chrome-матрица пройдена.
- [ ] RDP/WARP smoke пройден.
- [ ] Нет новых P0/P1.
- [ ] Performance-замеры в допустимых пределах.
- [ ] Visual snapshots приняты.
- [ ] README, API docs и примеры обновлены.
- [ ] Версия и changelog зафиксированы.
- [ ] Сборка воспроизводится с чистой машины.

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
