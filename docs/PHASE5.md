# PHASE 5 — hardening pass, benchmark, CI

> Статус: hardening pass завершён 14.11.2026; **M0 принят с waivers** — [M0.md](M0.md).
> Связанные документы: [PHASE4.md](PHASE4.md), [QUALITY.md](QUALITY.md), [ROADMAP.md](ROADMAP.md), [PLAN.md](../PLAN.md).

## Что добавлено

- DPI/resize matrix в `cppui.render_smoke`: 96 / 120 / 144 / 192 DPI,
  размеры 900×580 … 1120×720, Light/Dark и Compact/Cozy/Comfortable.
- `cppui.dpi`: реальный перенос окна между двумя мониторами
  **120 DPI (DISPLAY1)** и **96 DPI (DISPLAY2)**; проверяется смена DPI и
  непустой client area. На машинах с одним DPI тест делает SKIP.
- `cppui.chrome`: hit testing caption/min/max/close/resize, maximize/restore,
  minimize/restore, resize stress и DwmFlush. Snap Layouts, Alt+Space и Alt+F4
  проверены визуально; screenshots — в [M0.md](M0.md).
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

Debug benchmark: frame ~8.33 ms, layout ~0.124 ms. CTest: **8/8** в Debug и
Release; visual regression проходит за ~1.5 s при точном PNG-совпадении.
`DXGI_SWAP_EFFECT_FLIP_DISCARD` и `D3D11_CREATE_DEVICE_SINGLETHREADED` снизили
frame CPU с ~1.95 до ~1.8 ms. Debug-каталог был удалён и собран с нуля; 8/8.

## Cold start / idle / memory

- Cold start до появления HWND: **177 ms median** (161 / 177 / 199 ms, 3 запуска).
  In-app startup до первого кадра — **101 ms**; внешние 177 ms включают запуск процесса.
  Target 150 ms по внешнему замеру не выполнен, hard limit 250 ms выполнен.
- Idle CPU: **0.000 ms за 8 s** (≈0% от суммарного CPU).
- Private memory Release: **43.8 MB** (hard 60 MB пройден; target 40 MB превышен на 3.8 MB).
  Working set **64.4 MB** включает shared GPU/font pages.
- Clean build directory Debug удалён и собран заново: **8/8** тестов зелёные.
  Это не заменяет проверку на отдельной чистой машине; clean-machine waived.

## Статус M0

M0 принят с environment-waivers; полный sign-off — в [M0.md](M0.md).

- [x] DPI-матрица 96 / 120 / 150 / 200 (150/200 simulated);
- [x] Light/Dark + density в матрице;
- [x] theme switch без перезапуска;
- [x] Tab/Shift+Tab, Space/Enter и focus;
- [x] unit/integration/visual/chrome/dpi тесты (8/8);
- [x] публичный top-level `Form` API;
- [x] реальный mixed-DPI переход 120 → 96 на двух мониторах;
- [x] Snap Layouts, Alt+Space, Alt+F4;
- [x] benchmark и performance-замеры;
- [x] CI workflow добавлен (local pass);
- [ ] clean-machine и remote CI — waived;
- [ ] flicker/input-to-photon — измеряются отдельно, не блокируют M0.

## Следующий шаг

v0.1 Form Starter: TextEdit, CheckBox/RadioButton, Panel/GroupPanel,
ProgressBar/Loader, PopupMenu, Dialog/MessageBox, ToolTip и общий `DensityMode`.
