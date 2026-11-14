# CppUI

**CppUI** — рабочее название лёгкого UI-фреймворка для Windows на C++20. Цель — быстрый и предсказуемый десктопный toolkit с кастомным окном, аппаратной отрисовкой, аккуратными темами и API, которое не заставляет писать инфраструктуру заново в каждом приложении.

> Статус: **Phase 5 hardening pass завершён**. Есть Form-preview, DPI matrix 96–192, benchmark и CI workflow; M0 closure ещё открыт (multi-monitor, visual, memory).
> Дата: 14.11.2026.

## Ключевые решения

- **Windows-first**: Windows 11 x64 в приоритете, Windows 10 — best effort. Cross-platform не входит в первый этап.
- **C++20**, MSVC 2022/2026, CMake, статическая библиотека. Ninja — основной путь для CI; Visual Studio generator используется локально на локализованной MSVC.
- **Win32 + DWM** для окна и кастомного title bar; **D3D11 + DXGI flip-model + Direct2D/DirectWrite** для отрисовки.
- **Retained-mode** дерево виджетов с layout-проходами `measure → arrange → paint`, без HTML/CSS-движка.
- **Ноль сторонних runtime-зависимостей**: только Windows SDK. Тесты — собственные исполняемые файлы и CTest; внешний test framework не подключён.
- **Визуальное направление Signal Teal** (выбрано после аудита `design_chats`), светлая/тёмная/системная тема через семантические дизайн-токены.
- **Density S / M / L** и паттерны data-workspace с inline editing и row states зафиксированы в дизайне.
- **Управляемая область**: сначала фундамент и форма, затем контролы волнами. Полный список контролов не входит в первый релиз.

## Документация

| Документ | Назначение |
|---|---|
| [PLAN.md](PLAN.md) | Основной план создания, цели, границы, этапы и риски |
| [docs/PHASE0.md](docs/PHASE0.md) | Bootstrap: CMake, токены, первое окно |
| [docs/PHASE1.md](docs/PHASE1.md) | Platform + render: device loss, WARP, idle CPU, smoke test |
| [docs/PHASE2.md](docs/PHASE2.md) | Custom chrome: title bar, hit testing, maximize/restore, DWM |
| [docs/PHASE3.md](docs/PHASE3.md) | Retained-mode core, Button, UiHost, Light/Dark/System |
| [docs/PHASE4.md](docs/PHASE4.md) | Form preview, StatusBar, titlebar actions |
| [docs/PHASE5.md](docs/PHASE5.md) | DPI matrix, benchmark, CI, hardening results |
| [CONTROL_BACKLOG.md](CONTROL_BACKLOG.md) | Полный каталог контролов, недостающие элементы, зависимости и волны |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Релизы, ориентировочные сроки, критерии выхода |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Архитектура, стек, окно, рендер, API и структура проекта |
| [docs/DESIGN_SYSTEM.md](docs/DESIGN_SYSTEM.md) | Визуальный язык, токены, спецификация формы и кнопок |
| [docs/DESIGN_REFERENCES.md](docs/DESIGN_REFERENCES.md) | Аудит `design_chats`: какие дизайны, палитры и паттерны стоит взять |
| [docs/QUALITY.md](docs/QUALITY.md) | Тестовая стратегия, DPI-матрица, performance-бюджеты и release checklist |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Журнал принятых и открытых решений |
| [design/README.md](design/README.md) | Описание макетов, галереи контролов и скриншотов |

## Phase 4: Form preview

![Phase 4 form preview](docs/images/phase4-preview.png)

Экран собран как форма: sidebar с разделами, основная карточка настроек, `FormPanel`, закреплённый снизу `StatusBar` и дополнительное действие сброса в titlebar. Работают Light/Dark/System, density S/M/L, hover/pressed/focus, клики, Tab/Shift+Tab, Space/Enter и обновление статуса при каждом действии.

Тёмная тема: [phase4-dark.png](docs/images/phase4-dark.png). Компактный режим: [phase4-compact.png](docs/images/phase4-compact.png). Подробности — в [docs/PHASE4.md](docs/PHASE4.md).

## Hardening (Phase 5)

- CTest **6/6** в Debug и Release: theme, UI, Form, DPI matrix, benchmark, visual regression.
- Simulated DPI matrix **96 / 120 / 144 / 192** с Light/Dark и S/M/L.
- Visual snapshots Light/Dark/Compact в `tests/baselines`, сравнение через WIC/D2D.
- Публичный `Form` API с Create/SetContent/theme/density/status/Show/Run.
- Release benchmark: **120 FPS** (VSync), **1.30 ms CPU/кадр**, layout **0.119 ms**.
- Cold start **177 ms**, idle CPU **≈0%**, working set **64.4 MB** (P2: выше 60 MB).
- GitHub Actions workflow для Debug/Release и benchmark artifacts.

M0 closure ещё не объявлен: реальная multi-monitor матрица, flicker/input-to-photon, memory budget, clean-machine и подтверждение CI на runner. `Form` API уже доступен в [include/cppui/form.hpp](include/cppui/form.hpp). Подробности — в [docs/PHASE5.md](docs/PHASE5.md).

## Визуальный концепт

- [**Галерея контролов**](design/controls-gallery.html) — пять экранов, Light / Dark, density S/M/L и inline editing.
- [Обзор галереи, Light](design/controls-overview-light.png) · [Dark](design/controls-overview-dark.png)
- [Рабочее место, Light](design/controls-workspace-light.png) · [Dark](design/controls-workspace-dark.png) · [inline editing](design/controls-workspace-edit.png)
- [Интерактивный макет формы](design/form-preview.html) — Light / Dark / System.
- [Форма, светлая тема](design/form-light.png) · [тёмная тема](design/form-dark.png)

![Обзор галереи контролов в Signal Teal](design/controls-overview-light.png)

![Рабочее место с data-контролами](design/controls-workspace-light.png)

![Форма в светлой теме](design/form-light.png)

## Текущее состояние

- [x] Зафиксированы цели, технический стек и границы первого этапа.
- [x] Собран визуальный концепт формы в двух темах.
- [x] Собрана галерея всех запланированных контролов в двух темах.
- [x] Выбрано направление **Signal Teal**, добавлены density S/M/L и data-workspace с inline editing.
- [x] Проведён аудит `design_chats` и зафиксированы дизайн-референсы.
- [x] Составлен каталог планируемых контролов и зависимостей.
- [x] Разложены релизы и критерии качества.
- [x] Найден и проверен toolchain (VS 2026 / MSVC 19.51, Windows SDK 10.0.26100, CMake 4.3, Ninja 1.13).
- [x] Инициализирован git-репозиторий и CMake-каркас.
- [x] Появился первый исполняемый пример: окно с D3D11/DXGI/Direct2D.
- [x] Добавлены smoke-тест, Debug/Release пресеты и CI workflow.
- [x] Выделен `Win32Window`, добавлены DPI/resize callbacks и WARP.
- [x] Добавлены device-loss recovery и render smoke test; idle CPU ≈ 0%.
- [x] Добавлен кастомный chrome: title bar, системные кнопки, hit testing, maximize/restore.
- [x] Добавлены retained-mode `Widget`, `StackPanel`, `CardPanel`, `Label`, `Button`.
- [x] Добавлены Light/Dark/System, переключатель темы в шапке и интерактивное demo.
- [x] Добавлен `DensityMode` Compact/Cozy/Comfortable для компактных Button.
- [x] Добавлены Form-preview, `FormPanel`, `StatusBar` и действие сброса в titlebar.
- [x] Добавлены simulated DPI matrix 96–192, benchmark, visual regression и GitHub Actions workflow.
- [x] Добавлен публичный top-level `Form` API и integration test.
- [ ] Закрыт M0: реальная multi-monitor матрица, flicker/input-to-photon, memory budget, clean-machine.

## Сборка

Требования:

- Windows 11 x64 (Windows 10 — best effort).
- Visual Studio 2022/2026 с workload «Desktop development with C++» и Windows SDK.
- CMake 3.28+; Ninja — для CI/английской локали MSVC.
- Git.

Быстрый старт:

```powershell
.\tools\build.ps1 -Test
.\out\build\windows-vs2026-debug\Debug\cppui-bootstrap.exe
```

Скрипт находит Visual Studio, CMake и Ninja через `vswhere` и собирает нужный preset. Для VS 2022 используйте `-Preset windows-vs2022-debug`. Для Ninja-сборки на английской локали — `-Preset windows-msvc-debug`; подробнее в [docs/PHASE1.md](docs/PHASE1.md).

Проверки renderer:

```powershell
.\out\build\windows-vs2026-debug\Debug\cppui-bootstrap.exe --self-test
$env:CPPUI_FORCE_WARP = '1'
.\out\build\windows-vs2026-debug\Debug\cppui-bootstrap.exe --self-test
```

Benchmark (Release):

```powershell
Push-Location .\out\build\windows-vs2026-release
.\Release\cppui-bootstrap.exe --benchmark
Get-Content cppui-benchmark.txt
Pop-Location
```

`--self-test` проходит четыре комбинации DPI/density/theme; `--benchmark`
пишет frame/layout/resize/theme/density-метрики в `cppui-benchmark.txt`.
`--capture-visual` пишет Light/Dark/Compact PNG в `visual-output/`; CTest
`cppui.visual` сравнивает их с `tests/baselines/`. В живом окне `F5`
принудительно пересоздаёт render device, а `Esc` закрывает его. Кастомный chrome проверяется вручную: min/max/close, resize и drag title bar.

## Ближайший шаг

M0 closure: реальная 100–200% / mixed-DPI / multi-monitor матрица, flicker/input-to-photon, снижение memory и cold start, подтверждение CI на runner.

## Лицензия

Пока не выбрана. Это одно из открытых решений в [docs/DECISIONS.md](docs/DECISIONS.md). До определения лицензии не стоит делать репозиторий публичным.
