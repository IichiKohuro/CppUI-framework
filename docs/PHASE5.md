# PHASE 5 — hardening pass, benchmark, CI

> Статус: hardening pass завершён 14.11.2026; M0 closure ещё не объявлен.
> Связанные документы: [PHASE4.md](PHASE4.md), [QUALITY.md](QUALITY.md), [ROADMAP.md](ROADMAP.md), [PLAN.md](../PLAN.md).

## Что добавлено

- DPI/resize matrix в `cppui.render_smoke`: 96 / 120 / 144 / 192 DPI,
  размеры 900×580 … 1120×720, Light/Dark и Compact/Cozy/Comfortable.
- `cppui.dpi`: реальный перенос окна между двумя мониторами
  **120 DPI (DISPLAY1)** и **96 DPI (DISPLAY2)**; проверяется смена DPI и
  непустой client area. На машинах с одним DPI тест делает SKIP.
- Проверка target size: после `Resize` D2D target в DIP должен совпадать
  с ожидаемым размером (защита от stale-target бага Phase 4).
- Visual regression:
  - `D2DRenderer::RenderAndCapture` читает backbuffer через D3D11 staging и
    кодирует PNG через WIC (`windowscodecs`);
  - baselines: `tests/baselines/phase5-{light-cozy,dark-cozy,light-compact}.png`;
  - `cppui.visual` сравнивает текущие PNG с допуском 8/255 и 2% пикселей;
  - при точном совпадении SHA-256 — быстрый проход.
- `cppui_bootstrap --benchmark` и CTest `cppui.benchmark`:
  - 120 кадров с UI;
  - layout 500 итераций;
  - resize по пяти размерам;
  - 60 переключений темы;
  - 30 переключений density;
  - CPU-время процесса через `GetProcessTimes`.
- GitHub Actions workflow `.github/workflows/windows.yml`:
  - Windows 2022 runner;
  - Debug и Release;
  - CMake presets, build, CTest;
  - artifacts с `cppui-benchmark.txt`.
- `.gitignore` для локального benchmark-файла.

## Результаты Release benchmark

Окружение: Windows 11, x64, 22 logical cores, 125% DPI, Release, hidden window.

| Метрика | Значение |
|---|---:|
| Frame wall time, avg | **8.33 ms** |
| Frame wall time, p95 | **8.43 ms** |
| Frame rate | **120 FPS** (VSync) |
| Frame CPU time, avg | **~1.8 ms** |
| Layout, avg | **~0.15 ms** |
| Resize cycle, avg frame | **~7.4 ms** |
| Startup to first frame (in-app) | **101 ms** |
| Theme switch + frame | **7.98 ms** |
| Density switch + layout + frame | **8.33 ms** |
| Target size | **1000×640 DIP** |

Debug benchmark: frame ~8.33 ms, layout ~0.124 ms. CTest: **7/7** в Debug и
Release; visual regression проходит за ~1.5 s при точном PNG-совпадении.
`DXGI_SWAP_EFFECT_FLIP_DISCARD` и `D3D11_CREATE_DEVICE_SINGLETHREADED` снизили
frame CPU с ~1.95 до ~1.8 ms. Debug-каталог был удалён и собран с нуля; 7/7.

## Cold start / idle / memory

- Cold start до появления HWND: **177 ms median** (161 / 177 / 199 ms, 3 запуска).
  In-app startup до первого кадра — **101 ms**; внешние 177 ms включают запуск процесса.
  Target 150 ms по внешнему замеру не выполнен, hard limit 250 ms выполнен.
- Idle CPU: **0.000 ms за 8 s** (≈0% от суммарного CPU).
- Working set Release: **≈64.4 MB**.
  Hard budget 60 MB превышен на ~4.4 MB — открытый P2.
- Clean build directory Debug удалён и собран заново: **7/7** тестов зелёные.
  Это не заменяет проверку на отдельной чистой машине.

## Статус M0

Закрыто автоматизацией:

- [x] DPI-матрица 96–192 DPI на уровне renderer/layout;
- [x] Light/Dark + density в матрице;
- [x] theme switch без перезапуска;
- [x] Tab/Shift+Tab, Space/Enter и focus;
- [x] unit/integration-тесты;
- [x] публичный top-level `Form` API (`cppui.form` integration test);
- [x] реальный mixed-DPI переход 120 → 96 на двух мониторах;
- [x] visual snapshots Light/Dark/Compact с допуском;
- [x] benchmark и performance-замеры;
- [x] CI workflow добавлен.

Ещё не закрыто:

- [ ] clean-machine build на отдельной машине;
- [ ] 150/200% DPI (на машине доступны только 120/96);
- [ ] Snap Layouts / Alt+Space / Alt+F4 / taskbar auto-hide;
- [ ] flicker/input-to-photon замеры;
- [ ] memory budget 60 MB;
- [ ] подтверждение CI на удалённом runner.

## Следующий шаг

M0 closure: реальная multi-monitor/DPI-матрица, flicker/input-to-photon,
снижение памяти и cold start, финальный прогон CI и проверка оставшихся
критериев Technical Preview.
