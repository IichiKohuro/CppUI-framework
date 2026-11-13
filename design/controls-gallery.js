(() => {
  const root = document.documentElement;
  const pages = {
    input: {
      index: '01',
      kicker: 'Foundation',
      title: 'Ввод и действия',
      subtitle: 'Кнопки, поля ввода, выбор и индикаторы — базовый набор формы.',
      count: '05'
    },
    selection: {
      index: '02',
      kicker: 'Pickers & Lists',
      title: 'Выбор и списки',
      subtitle: 'ComboBox, CheckedComboBox, DateEdit, ListBox, CheckedListBox и ListView.',
      count: '06'
    },
    data: {
      index: '03',
      kicker: 'Data Controls',
      title: 'Данные',
      subtitle: 'TreeList, GridView, PropertyGrid и VerticalGrid для табличных сценариев.',
      count: '04'
    },
    workspace: {
      index: '04',
      kicker: 'Data Workspace',
      title: 'Рабочее место',
      subtitle: 'TreeList, GridView и PropertyGrid в одном экране: inline editing, row states и плотность.',
      count: '01'
    },
    layout: {
      index: '05',
      kicker: 'Layout & Overlays',
      title: 'Макет и оверлеи',
      subtitle: 'TabControl, GroupPanel, SplitContainer, PopupMenu, ToolTip, Dialog и BusyOverlay. BarManager и StatusBar — в оболочке.',
      count: '06'
    }
  };

  const pageNodes = [...document.querySelectorAll('.page[data-page]')];
  const navNodes = [...document.querySelectorAll('.nav-item[data-page]')];
  const titleNode = document.getElementById('pageTitle');
  const subtitleNode = document.getElementById('pageSubtitle');
  const indexNode = document.getElementById('pageIndex');
  const kickerNode = document.getElementById('pageKicker');
  const countNode = document.getElementById('pageCount');
  const themeLabels = [...document.querySelectorAll('.js-theme-label')];
  const titleThemeButton = document.querySelector('.title-actions [data-toggle-theme]');
  const themeButtons = [...document.querySelectorAll('[data-toggle-theme]')];
  const densityButtons = [...document.querySelectorAll('[data-density]')];

  function parseHash() {
    const raw = decodeURIComponent(window.location.hash.replace(/^#/, ''));
    const parts = raw.split('/').filter(Boolean);
    const dark = parts.includes('dark');
    const page = parts.find((part) => pages[part]) || 'input';
    return { dark, page };
  }

  function applyPage(name) {
    const config = pages[name] || pages.input;
    pageNodes.forEach((node) => {
      const active = node.dataset.page === name;
      node.classList.toggle('is-active', active);
      node.setAttribute('aria-hidden', active ? 'false' : 'true');
    });
    navNodes.forEach((node) => {
      const active = node.dataset.page === name;
      node.classList.toggle('is-active', active);
      if (active) node.setAttribute('aria-current', 'page');
      else node.removeAttribute('aria-current');
    });
    if (titleNode) titleNode.textContent = config.title;
    if (subtitleNode) subtitleNode.textContent = config.subtitle;
    if (indexNode) indexNode.textContent = config.index;
    if (kickerNode) kickerNode.textContent = config.kicker;
    if (countNode) countNode.textContent = config.index + ' / 05';
    document.title = `CppUI — ${config.title}`;
  }

  function applyDensity(density) {
    const valid = ['compact', 'cozy', 'comfortable'];
    const next = valid.includes(density) ? density : 'cozy';
    root.dataset.density = next;
    densityButtons.forEach((button) => {
      const active = button.dataset.density === next;
      button.classList.toggle('is-active', active);
      button.setAttribute('aria-pressed', active ? 'true' : 'false');
    });
  }

  function applyTheme(theme) {
    const dark = theme === 'dark';
    root.dataset.theme = dark ? 'dark' : 'light';
    themeLabels.forEach((label) => {
      label.textContent = dark ? 'Тёмная тема' : 'Светлая тема';
    });
    themeButtons.forEach((button) => {
      button.setAttribute('aria-label', dark ? 'Переключить на светлую тему' : 'Переключить на тёмную тему');
    });
    if (titleThemeButton) {
      titleThemeButton.setAttribute('title', dark ? 'Переключить на светлую тему' : 'Переключить на тёмную тему');
    }
  }

  function toggleTheme() {
    applyTheme(root.dataset.theme === 'dark' ? 'light' : 'dark');
  }

  function fitStage() {
    const scale = Math.min(1, window.innerWidth / 1580, window.innerHeight / 1040);
    root.style.setProperty('--fit-scale', String(Math.max(0.35, scale)));
  }

  navNodes.forEach((node) => {
    node.addEventListener('click', () => applyPage(node.dataset.page));
  });

  themeButtons.forEach((button) => {
    button.addEventListener('click', toggleTheme);
  });

  densityButtons.forEach((button) => {
    button.addEventListener('click', () => applyDensity(button.dataset.density));
  });

  let openCellMenu = null;

  function closeCellMenu() {
    if (openCellMenu) {
      openCellMenu.remove();
      openCellMenu = null;
      document.querySelectorAll('.ws-cell-editor[aria-expanded="true"]').forEach((editor) => {
        editor.setAttribute('aria-expanded', 'false');
        editor.classList.remove('is-editing');
      });
    }
  }

  function openEditorMenu(editor) {
    closeCellMenu();
    const cell = editor.closest('.ws-status-cell');
    if (!cell) return;
    editor.classList.add('is-editing');
    editor.setAttribute('aria-expanded', 'true');
    const menu = document.createElement('div');
    menu.className = 'ws-inline-menu';
    menu.setAttribute('role', 'listbox');
    const options = [
      { label: 'Running', color: 'accent' },
      { label: 'Success', color: 'success' },
      { label: 'Warning', color: 'warning' },
      { label: 'Failed', color: 'failed' }
    ];
    const current = editor.querySelector('.ws-editor-label')?.textContent.trim();
    options.forEach((option) => {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'ws-inline-option' + (option.label === current ? ' is-current' : '');
      button.setAttribute('role', 'option');
      button.setAttribute('aria-selected', option.label === current ? 'true' : 'false');
      button.innerHTML = `<span class="status-dot ${option.color}"></span>${option.label}`;
      button.addEventListener('click', () => {
        const label = editor.querySelector('.ws-editor-label');
        if (label) label.textContent = option.label;
        closeCellMenu();
      });
      menu.appendChild(button);
    });
    cell.appendChild(menu);
    openCellMenu = menu;
  }

  document.addEventListener('click', (event) => {
    const editor = event.target.closest('.ws-cell-editor');
    if (editor) {
      event.stopPropagation();
      if (openCellMenu) closeCellMenu();
      else openEditorMenu(editor);
      return;
    }
    if (!event.target.closest('.ws-inline-menu')) closeCellMenu();
  });

  document.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') closeCellMenu();
  });

  const initial = parseHash();
  const hashParts = decodeURIComponent(window.location.hash.replace(/^#/, '')).split('/');
  const initialDensity = hashParts.find((part) => ['compact', 'cozy', 'comfortable'].includes(part)) || 'cozy';
  applyTheme(initial.dark ? 'dark' : 'light');
  applyDensity(initialDensity);
  applyPage(initial.page);
  fitStage();

  if (hashParts.includes('edit') && initial.page === 'workspace') {
    window.setTimeout(() => {
      const editor = document.querySelector('.ws-cell-editor');
      if (editor) openEditorMenu(editor);
    }, 120);
  }

  window.addEventListener('resize', fitStage);
  window.addEventListener('hashchange', () => {
    const state = parseHash();
    applyTheme(state.dark ? 'dark' : 'light');
    applyPage(state.page);
  });
})();
