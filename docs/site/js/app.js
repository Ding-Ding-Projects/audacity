(function () {
  'use strict';

  const LS = {
    settings: 'ma.settings.v1',
    history: 'ma.history.v1',
    favorites: 'ma.favorites.v1',
    vocab: 'ma.vocabulary.v1',
  };

  const DEFAULT_SETTINGS = {
    theme: 'system',
    seedColor: '#926BFF',
    density: 'default',
    fontScale: 1,
    language: 'en',
    funnyEnglish: 1,
    funnyCantonese: 1,
    emojiInDialogs: true,
    reducedMotionOverride: 'auto',
    adhd: { focus: false, lowStimulation: false, timeAwareness: false, oneThing: false, momentum: false },
    docsDock: 'left',
  };

  function loadSettings() {
    try {
      const raw = JSON.parse(localStorage.getItem(LS.settings) || 'null');
      return Object.assign({}, DEFAULT_SETTINGS, raw || {}, { adhd: Object.assign({}, DEFAULT_SETTINGS.adhd, (raw && raw.adhd) || {}) });
    } catch (e) { return Object.assign({}, DEFAULT_SETTINGS); }
  }
  let settings = loadSettings();
  function saveSettings(next, actionLabel) {
    settings = next;
    try { localStorage.setItem(LS.settings, JSON.stringify(settings)); } catch (e) { /* ignore */ }
    appendHistory(actionLabel || 'Settings changed');
    applyTheme();
  }

  // ---------- History (append-only) ----------
  function loadHistory() {
    try { return JSON.parse(localStorage.getItem(LS.history) || '[]'); } catch (e) { return []; }
  }
  function appendHistory(action) {
    const list = loadHistory();
    list.push({ at: new Date().toISOString(), action });
    try { localStorage.setItem(LS.history, JSON.stringify(list.slice(-500))); } catch (e) { /* ignore */ }
  }

  // ---------- Notifications ----------
  const notifications = [];
  function notify(message, kind) {
    notifications.unshift({ at: new Date().toISOString(), message, kind: kind || 'info' });
    renderNotifCentre();
    showSnackbar(message);
  }
  function showSnackbar(message) {
    const stack = document.getElementById('snackbar-stack');
    const el = document.createElement('div');
    el.className = 'snackbar';
    el.setAttribute('role', 'status');
    el.textContent = message;
    stack.appendChild(el);
    setTimeout(() => el.remove(), 5000);
  }
  function renderNotifCentre() {
    const list = document.getElementById('notif-list');
    list.innerHTML = '';
    notifications.slice(0, 50).forEach((n) => {
      const div = document.createElement('div');
      div.className = 'notif-item';
      div.textContent = new Date(n.at).toLocaleTimeString() + ' · ' + n.message;
      list.appendChild(div);
    });
  }

  // ---------- Theme application via M3Color ----------
  function applyTheme() {
    const isDark = settings.theme === 'dark' || (settings.theme === 'system' && matchMedia('(prefers-color-scheme: dark)').matches);
    let scheme;
    try { scheme = window.M3Color.buildScheme(settings.seedColor, isDark); }
    catch (e) { scheme = window.M3Color.buildScheme('#926BFF', isDark); }
    const root = document.documentElement;
    Object.entries(scheme).forEach(([role, hex]) => {
      root.style.setProperty('--md-sys-color-' + role, hex);
      // also expose kebab-case for readability in CSS authored above
      const kebab = role.replace(/([A-Z])/g, '-$1').toLowerCase();
      root.style.setProperty('--md-sys-color-' + kebab, hex);
    });
    root.style.setProperty('--md-font-scale', String(settings.fontScale || 1));
    root.setAttribute('data-density', settings.density);
    const reduceMotion = settings.reducedMotionOverride === 'reduce' ? true : settings.reducedMotionOverride === 'allow' ? false : null;
    if (reduceMotion === true) root.setAttribute('data-reduced-motion', 'true');
    else root.removeAttribute('data-reduced-motion');
    root.lang = settings.language === 'yue' ? 'yue' : 'en';
  }

  // ---------- Personal vocabulary ----------
  function loadVocab() {
    try { return JSON.parse(localStorage.getItem(LS.vocab) || 'null'); } catch (e) { return null; }
  }
  function applyVocabulary(text) {
    const v = loadVocab();
    if (!v || !Array.isArray(v.entries)) return text;
    let out = text;
    v.entries.forEach((e) => {
      if (e && typeof e.from === 'string' && e.from) {
        out = out.split(e.from).join(e.to != null ? e.to : '');
      }
    });
    return out;
  }

  // ---------- Routing ----------
  const routes = ['home', 'features', 'downloads', 'changelog', 'docs', 'accessibility', 'privacy', 'settings'];
  function currentRoute() {
    const h = location.hash.replace('#', '');
    return routes.includes(h) ? h : 'home';
  }
  function navigate(route) { location.hash = '#' + route; }

  function setActiveNav(route) {
    document.querySelectorAll('.nav-item').forEach((a) => {
      const active = a.dataset.route === route;
      if (active) { a.setAttribute('aria-current', 'page'); a.classList.add('active'); }
      else { a.removeAttribute('aria-current'); a.classList.remove('active'); }
    });
  }

  function render() {
    const route = currentRoute();
    setActiveNav(route);
    const outlet = document.getElementById('route-outlet');
    outlet.innerHTML = '';
    const renderer = renderers[route] || renderers.home;
    renderer(outlet);
    appendHistory('Viewed ' + route);
  }

  // ---------- Filter helper wiring a text input + regex builder to a list ----------
  function wireFilter(input, items, renderList, inventoryId) {
    let regexState = null;
    function apply() {
      const q = input.value;
      let filtered;
      if (regexState && regexState.pattern) {
        try {
          const re = new RegExp(regexState.pattern, regexState.flags || 'i');
          filtered = items.filter((it) => re.test(it.text));
        } catch (e) { filtered = items; }
      } else if (q) {
        const ql = q.toLowerCase();
        filtered = items.filter((it) => it.text.toLowerCase().includes(ql));
      } else {
        filtered = items;
      }
      renderList(filtered);
    }
    input.addEventListener('input', apply);
    window.RegexBuilder.attach(input, {
      inventoryId,
      onApply(state) { regexState = state; apply(); },
    });
    apply();
    return apply;
  }

  // ---------- Renderers ----------
  const renderers = {};

  renderers.home = function (root) {
    root.innerHTML =
      '<h1>Material Audacity</h1>' +
      '<p>A Material 3 rewrite of the Audacity 4 shell, keeping the audio engine and rebuilding the interface on real Material components.</p>' +
      '<p id="release-line">Loading release information…</p>' +
      '<div role="tablist" aria-label="About Material Audacity" class="md-tablist" id="home-tabs"></div>' +
      '<div id="home-tabpanels"></div>';

    fetch('data/release.json').then((r) => r.json()).then((rel) => {
      document.getElementById('release-line').textContent =
        'Version: ' + rel.tag + ' · Updated: ' + new Date(rel.updatedAt).toDateString() + ' · Commit: ' + rel.commit.slice(0, 10);
    }).catch(() => { document.getElementById('release-line').textContent = 'Release information is unavailable right now.'; });

    const tabs = [
      { id: 'about', label: 'About', body: '<p>Material Audacity brings Material 3 dynamic color, shape, motion, and components to the Audacity shell, plus a command palette, a regex builder everywhere search happens, personal vocabulary substitution, and local history.</p>' },
      { id: 'build', label: 'Build', body: '<p>Windows packages are built with Squirrel.Windows through a public builder that never sees this project\'s name. Code signing is permanently disabled.</p>' },
      { id: 'features', label: 'Features', body: '<p>See the Features page for the full list: command palette, regex builder, tabs, languages and funny levels, personal vocabulary, local history, changelog, notifications, super confirmation, appearance editors, toy locks and authenticator, Ollama suite manager, ADHD modes, and platform installers.</p>' },
      { id: 'downloads', label: 'Downloads', body: '<p>See the Downloads page for the latest assets and checksums.</p>' },
      { id: 'license', label: 'License', body: '<p>Material Audacity is distributed under the same license as Audacity (GPLv2 or later). See LICENSE.txt in the repository.</p>' },
    ];
    const tablist = document.getElementById('home-tabs');
    const panels = document.getElementById('home-tabpanels');
    tabs.forEach((t, i) => {
      const btn = document.createElement('button');
      btn.className = 'md-tab'; btn.id = 'hometab-' + t.id; btn.setAttribute('role', 'tab');
      btn.setAttribute('aria-selected', i === 0 ? 'true' : 'false');
      btn.setAttribute('aria-controls', 'homepanel-' + t.id);
      btn.textContent = t.label;
      btn.addEventListener('click', () => selectHomeTab(t.id));
      tablist.appendChild(btn);
      const panel = document.createElement('div');
      panel.className = 'md-tabpanel'; panel.id = 'homepanel-' + t.id; panel.setAttribute('role', 'tabpanel');
      panel.setAttribute('aria-labelledby', 'hometab-' + t.id);
      panel.hidden = i !== 0;
      panel.innerHTML = t.body;
      panels.appendChild(panel);
    });
    function selectHomeTab(id) {
      tabs.forEach((t) => {
        document.getElementById('hometab-' + t.id).setAttribute('aria-selected', String(t.id === id));
        document.getElementById('homepanel-' + t.id).hidden = t.id !== id;
      });
    }
  };

  const FEATURES = [
    ['Material 3 rewrite', 'Dynamic color from a single seed, full type/shape/motion tokens, real Material components.'],
    ['Command palette', 'Ctrl+Shift+F opens a palette indexing every section, doc heading, and setting, with live inline controls.'],
    ['Regex builder', 'An anchored regex builder beside every search and filter field, guided and raw, with captures and explanations.'],
    ['Tabs everywhere', 'Browser-style persistent tabs for documentation, with drag reorder, pinning, and an overflow menu.'],
    ['Languages and funny levels', 'English, playful Hong Kong-style Cantonese, and bilingual modes, each with a 1-5 tone dial.'],
    ['Personal vocabulary upload', 'Upload a JSON substitution list applied to visible text only, client-side, never logged.'],
    ['Local history', 'An append-only log of settings changes with date, action, and regex search.'],
    ['Changelog', 'Every entry links its full commit SHA to GitHub.'],
    ['Notifications', 'A corner snackbar stack plus a full notification centre.'],
    ['Super confirmation', 'Destructive operations require a full super-confirmation before they run.'],
    ['Appearance editors', 'An infinite color picker: hue wheel, saturation/value field, and format translator across HEX/RGB/HSL/HSV/HWB/OKLCH/CMYK.'],
    ['Toy locks and authenticator', 'Playful lock screens and an authenticator surface for the desktop app.'],
    ['Ollama suite manager', 'Manage local Ollama models from within the app.'],
    ['ADHD modes', 'Focus, Low stimulation, Time awareness, One thing at a time, and Momentum · all off by default, never presented as medical.'],
    ['Windows Squirrel installer', 'Unsigned Squirrel.Windows packaging with background update checks.'],
    ['Linux builds', 'Native Linux build targets alongside the Windows installer.'],
  ];
  renderers.features = function (root) {
    root.innerHTML =
      '<h1>Features</h1>' +
      '<div class="filter-row"><div class="md-field"><label for="features-filter-input">Filter features</label><input type="text" id="features-filter-input"></div></div>' +
      '<div class="card-grid" id="features-grid"></div>';
    const items = FEATURES.map(([t, d]) => ({ text: t + ' ' + d, title: t, desc: d }));
    function list(filtered) {
      const grid = document.getElementById('features-grid');
      grid.innerHTML = '';
      filtered.forEach((it) => {
        const card = document.createElement('div');
        card.className = 'md-card';
        card.innerHTML = '<h3>' + it.title + '</h3><p>' + it.desc + '</p>';
        grid.appendChild(card);
      });
    }
    wireFilter(document.getElementById('features-filter-input'), items, list, 'features-filter');
  };

  renderers.downloads = function (root) {
    root.innerHTML =
      '<h1>Downloads</h1>' +
      '<div class="filter-row"><div class="md-field"><label for="downloads-filter-input">Filter assets</label><input type="text" id="downloads-filter-input"></div></div>' +
      '<div id="downloads-body">Loading…</div>';
    fetch('data/release.json').then((r) => r.json()).then((rel) => {
      const body = document.getElementById('downloads-body');
      const unsignedNote = '<div class="md-card"><h3>This installer is unsigned</h3><p>' + rel.unsignedReason + '</p></div>';
      let assetsHtml;
      const items = (rel.assets || []).map((a) => ({ text: a.name, asset: a }));
      function list(filtered) {
        const el = document.getElementById('assets-list');
        if (!el) return;
        if (!filtered.length) { el.innerHTML = '<p>No release assets are published yet. Tag: ' + rel.tag + '.</p>'; return; }
        el.innerHTML = '<ul>' + filtered.map((it) => {
          const a = it.asset;
          return '<li><a href="' + a.url + '">' + a.name + '</a>' + (a.sha256 ? ' · SHA-256: <code>' + a.sha256 + '</code>' : '') + '</li>';
        }).join('') + '</ul>';
      }
      body.innerHTML = '<p>' + rel.notes + '</p>' + unsignedNote + '<div id="assets-list"></div>';
      wireFilter(document.getElementById('downloads-filter-input'), items, list, 'downloads-filter');
    }).catch(() => { document.getElementById('downloads-body').textContent = 'Release information is unavailable right now.'; });
  };

  renderers.changelog = function (root) {
    root.innerHTML =
      '<h1>Changelog</h1>' +
      '<div class="filter-row"><div class="md-field"><label for="changelog-filter-input">Filter changelog</label><input type="text" id="changelog-filter-input"></div></div>' +
      '<div id="changelog-list">Loading…</div>';
    fetch('data/changelog.json').then((r) => r.json()).then((entries) => {
      const items = entries.map((e) => ({ text: e.summary + ' ' + e.detail + ' ' + e.sha, entry: e }));
      function list(filtered) {
        const el = document.getElementById('changelog-list');
        el.innerHTML = filtered.map((it) => {
          const e = it.entry;
          return '<div class="md-card" style="margin-bottom:12px"><h3>' + e.summary + '</h3><p>' + e.date + ' · <a href="https://github.com/Ding-Ding-Projects/audacity/commit/' + e.sha + '"><code>' + e.shaDisplay + '</code></a></p><p>' + e.detail + '</p></div>';
        }).join('') || '<p>No entries match.</p>';
      }
      wireFilter(document.getElementById('changelog-filter-input'), items, list, 'changelog-filter');
    }).catch(() => { document.getElementById('changelog-list').textContent = 'Changelog is unavailable right now.'; });
  };

  renderers.accessibility = function (root) {
    root.innerHTML = '<h1>Accessibility statement</h1>' +
      '<p>This site targets keyboard reachability everywhere, visible focus rings, correct roles and accessible names, at least 4.5:1 contrast for text, usability at 320px width and 200% zoom, and lang="yue" on Cantonese text.</p>' +
      '<p>Tabs use tablist/tab/tabpanel roles, switches use role="switch", menus use role="menu", and dialogs use the native dialog element.</p>' +
      '<p>Motion respects prefers-reduced-motion, and Settings offers an explicit override.</p>' +
      '<p>If you find an accessibility problem, please open an issue on the repository.</p>';
  };

  renderers.privacy = function (root) {
    root.innerHTML = '<h1>Privacy</h1>' +
      '<p>This site makes no network requests to third parties and loads no external fonts or scripts at runtime. All settings, favorites, personal vocabulary, and history are stored only in your browser\'s local storage.</p>' +
      '<p>Personal vocabulary substitutions are applied entirely client-side and are never logged or transmitted.</p>' +
      '<p>The application itself performs unsigned update checks over HTTPS against its own release feed; see Downloads for details.</p>';
  };

  // ---------- Docs browser with tabs ----------
  const DOC_PAGES = [
    { id: 'getting-started', title: 'Getting started', file: 'docs/getting-started.md' },
    { id: 'architecture', title: 'Architecture overview', file: 'docs/architecture.md' },
    { id: 'regex-builder', title: 'The regex builder', file: 'docs/regex-builder.md' },
    { id: 'command-palette', title: 'The command palette', file: 'docs/command-palette.md' },
    { id: 'language-modes', title: 'Language modes', file: 'docs/language-modes.md' },
    { id: 'funny-levels', title: 'Funny levels', file: 'docs/funny-levels.md' },
    { id: 'attention-support-modes', title: 'Attention support modes', file: 'docs/attention-support-modes.md' },
    { id: 'dim-sum-surprise', title: 'Dim sum surprise', file: 'docs/dim-sum-surprise.md' },
    { id: 'school-mode', title: 'School mode', file: 'docs/school-mode.md' },
    { id: 'narrator', title: 'Narrator', file: 'docs/narrator.md' },
    { id: 'scheduled-settings', title: 'Scheduled settings', file: 'docs/scheduled-settings.md' },
    { id: 'personal-vocabulary', title: 'Personal vocabulary', file: 'docs/personal-vocabulary.md' },
    { id: 'notifications', title: 'Notifications', file: 'docs/notifications.md' },
    { id: 'super-confirmation', title: 'Super confirmation', file: 'docs/super-confirmation.md' },
    { id: 'ollama-suite-manager', title: 'Local model manager', file: 'docs/ollama-suite-manager.md' },
    { id: 'exports', title: 'Universal export', file: 'docs/exports.md' },
    { id: 'bulk-actions', title: 'Bulk actions', file: 'docs/bulk-actions.md' },
    { id: 'external-editor', title: 'External editor integration', file: 'docs/external-editor.md' },
    { id: 'docs-browser', title: 'In-app documentation browser', file: 'docs/docs-browser.md' },
    { id: 'automatic-updates', title: 'Automatic updates', file: 'docs/automatic-updates.md' },
    { id: 'no-nagging', title: 'No unsolicited interruptions', file: 'docs/no-nagging.md' },
    { id: 'appearance-editor', title: 'Per element appearance editor', file: 'docs/appearance-editor.md' },
    { id: 'app-rename', title: 'Renaming the application', file: 'docs/app-rename.md' },
    { id: 'toy-locks', title: 'Toy locks', file: 'docs/toy-locks.md' },
    { id: 'support-tickets', title: 'Support Tickets', file: 'docs/support-tickets.md' },
    { id: 'authenticator', title: 'Built in authenticator', file: 'docs/authenticator.md' },
    { id: 'tab-navigation', title: 'Tab navigation', file: 'docs/tab-navigation.md' },
    { id: 'local-history', title: 'Local version history', file: 'docs/local-history.md' },
    { id: 'emoji-switch', title: 'Emoji switch', file: 'docs/emoji-switch.md' },
    { id: 'changelog', title: 'Changelog and what is new', file: 'docs/changelog.md' },
    { id: 'status-reporting', title: 'Status reporting', file: 'docs/status-reporting.md' },
  ];
  const LS_TABS = 'ma.docTabs.v1';
  function loadTabState() {
    try { return JSON.parse(localStorage.getItem(LS_TABS) || 'null') || { open: [DOC_PAGES[0].id], pinned: [], active: DOC_PAGES[0].id }; }
    catch (e) { return { open: [DOC_PAGES[0].id], pinned: [], active: DOC_PAGES[0].id }; }
  }
  function saveTabState(s) { try { localStorage.setItem(LS_TABS, JSON.stringify(s)); } catch (e) { /* ignore */ } }

  renderers.docs = function (root) {
    let tabState = loadTabState();
    root.innerHTML =
      '<h1>Documentation</h1>' +
      '<div class="search-row"><div class="md-field"><label for="docs-search-input">Search documentation</label><input type="text" id="docs-search-input"></div></div>' +
      '<div class="search-row"><div class="md-field"><label for="tab-search-input">Search open tabs</label><input type="text" id="tab-search-input"></div>' +
      '<button type="button" class="md-text-button" id="close-not-containing">Close tabs not containing text</button>' +
      '<button type="button" class="md-text-button" id="close-containing">Close tabs containing text</button>' +
      '<label class="md-field"><span>Dock</span><select id="dock-select"><option value="left">Left</option><option value="right">Right</option><option value="top">Top</option><option value="bottom">Bottom</option></select></label>' +
      '</div>' +
      '<div class="docs-shell dock-' + settings.docsDock + '" id="docs-shell">' +
      '<div class="docs-strip" id="docs-strip" role="tablist" aria-label="Document tabs"></div>' +
      '<div class="docs-content markdown-body" id="docs-content"></div>' +
      '</div>';

    document.getElementById('dock-select').value = settings.docsDock;
    document.getElementById('dock-select').addEventListener('change', (e) => {
      settings = Object.assign({}, settings, { docsDock: e.target.value });
      saveSettings(settings, 'Changed docs dock to ' + e.target.value);
      document.getElementById('docs-shell').className = 'docs-shell dock-' + settings.docsDock;
    });

    const docsCache = {};
    function getDoc(id) {
      const page = DOC_PAGES.find((p) => p.id === id);
      if (docsCache[id]) return Promise.resolve(docsCache[id]);
      return fetch(page.file).then((r) => r.text()).then((md) => {
        const rendered = window.MiniMarkdown.render(md);
        docsCache[id] = Object.assign({ page }, rendered);
        return docsCache[id];
      });
    }

    function renderStrip() {
      const strip = document.getElementById('docs-strip');
      strip.innerHTML = '';
      const order = tabState.open.slice().sort((a, b) => {
        const pa = tabState.pinned.includes(a) ? 0 : 1;
        const pb = tabState.pinned.includes(b) ? 0 : 1;
        return pa - pb;
      });
      order.forEach((id) => {
        const page = DOC_PAGES.find((p) => p.id === id);
        const div = document.createElement('div');
        div.className = 'doc-tab' + (tabState.active === id ? ' active' : '');
        div.setAttribute('role', 'tab');
        div.setAttribute('draggable', 'true');
        div.setAttribute('aria-selected', String(tabState.active === id));
        div.dataset.id = id;
        div.innerHTML = '<button type="button" class="pin-btn' + (tabState.pinned.includes(id) ? ' pinned' : '') + '" title="Pin">📌</button>' +
          '<span class="tab-label" style="flex:1">' + page.title + '</span>' +
          '<button type="button" class="close-btn" title="Close">✕</button>';
        div.addEventListener('click', (e) => {
          if (e.target.classList.contains('pin-btn')) { togglePin(id); return; }
          if (e.target.classList.contains('close-btn')) { closeTab(id); return; }
          tabState.active = id; saveTabState(tabState); renderAll();
        });
        div.addEventListener('dragstart', (e) => e.dataTransfer.setData('text/plain', id));
        div.addEventListener('dragover', (e) => e.preventDefault());
        div.addEventListener('drop', (e) => {
          e.preventDefault();
          const dragged = e.dataTransfer.getData('text/plain');
          const from = tabState.open.indexOf(dragged);
          const to = tabState.open.indexOf(id);
          if (from >= 0 && to >= 0) {
            tabState.open.splice(from, 1);
            tabState.open.splice(to, 0, dragged);
            saveTabState(tabState); renderStrip();
          }
        });
        strip.appendChild(div);
      });
      const openBtnWrap = document.createElement('div');
      openBtnWrap.innerHTML = '<details style="margin-top:8px"><summary>Open more…</summary></details>';
      const details = openBtnWrap.querySelector('details');
      DOC_PAGES.filter((p) => !tabState.open.includes(p.id)).forEach((p) => {
        const b = document.createElement('button');
        b.type = 'button'; b.className = 'md-text-button'; b.style.display = 'block'; b.textContent = p.title;
        b.addEventListener('click', () => { tabState.open.push(p.id); tabState.active = p.id; saveTabState(tabState); renderAll(); });
        details.appendChild(b);
      });
      strip.appendChild(openBtnWrap);
    }
    function togglePin(id) {
      if (tabState.pinned.includes(id)) tabState.pinned = tabState.pinned.filter((x) => x !== id);
      else tabState.pinned.push(id);
      saveTabState(tabState); renderStrip();
    }
    function closeTab(id) {
      tabState.open = tabState.open.filter((x) => x !== id);
      if (tabState.active === id) tabState.active = tabState.open[0] || null;
      if (!tabState.open.length) { tabState.open = [DOC_PAGES[0].id]; tabState.active = DOC_PAGES[0].id; }
      saveTabState(tabState); renderAll();
    }
    function renderContent() {
      const content = document.getElementById('docs-content');
      if (!tabState.active) { content.innerHTML = '<p>No tab open.</p>'; return; }
      getDoc(tabState.active).then((doc) => { content.innerHTML = doc.html; });
    }
    function renderAll() { renderStrip(); renderContent(); }
    renderAll();

    document.getElementById('close-containing').addEventListener('click', () => {
      const q = document.getElementById('tab-search-input').value;
      if (!q) return;
      const matching = tabState.open.filter((id) => DOC_PAGES.find((p) => p.id === id).title.toLowerCase().includes(q.toLowerCase()));
      if (!matching.length) { notify('No tabs match "' + q + '"'); return; }
      if (!confirm('Close ' + matching.length + ' tab(s) containing "' + q + '"?')) return;
      tabState.open = tabState.open.filter((id) => !matching.includes(id));
      if (!tabState.open.includes(tabState.active)) tabState.active = tabState.open[0] || null;
      if (!tabState.open.length) { tabState.open = [DOC_PAGES[0].id]; tabState.active = DOC_PAGES[0].id; }
      saveTabState(tabState); renderAll();
    });
    document.getElementById('close-not-containing').addEventListener('click', () => {
      const q = document.getElementById('tab-search-input').value;
      if (!q) return;
      const matching = tabState.open.filter((id) => !DOC_PAGES.find((p) => p.id === id).title.toLowerCase().includes(q.toLowerCase()));
      if (!matching.length) { notify('No tabs would be closed'); return; }
      if (!confirm('Close ' + matching.length + ' tab(s) not containing "' + q + '"?')) return;
      tabState.open = tabState.open.filter((id) => !matching.includes(id));
      if (!tabState.open.includes(tabState.active)) tabState.active = tabState.open[0] || null;
      if (!tabState.open.length) { tabState.open = [DOC_PAGES[0].id]; tabState.active = DOC_PAGES[0].id; }
      saveTabState(tabState); renderAll();
    });

    window.RegexBuilder.attach(document.getElementById('tab-search-input'), { inventoryId: 'tab-search' });

    const searchItems = [];
    Promise.all(DOC_PAGES.map((p) => getDoc(p.id))).then((docs) => {
      docs.forEach((d) => { searchItems.push({ text: d.page.title, id: d.page.id }); d.headings.forEach((h) => searchItems.push({ text: h.text, id: d.page.id, anchor: h.id })); });
      function list(filtered) { /* results shown via title attr for simplicity */
        document.getElementById('docs-search-input').title = filtered.length + ' matches';
      }
      const apply = wireFilter(document.getElementById('docs-search-input'), searchItems, list, 'docs-search');
      document.getElementById('docs-search-input').addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          const q = e.target.value.toLowerCase();
          const hit = searchItems.find((it) => it.text.toLowerCase().includes(q));
          if (hit) { if (!tabState.open.includes(hit.id)) tabState.open.push(hit.id); tabState.active = hit.id; saveTabState(tabState); renderAll(); }
        }
      });
    });
  };

  // ---------- Settings ----------
  renderers.settings = function (root) {
    root.innerHTML =
      '<h1>Settings</h1>' +
      '<div class="search-row"><div class="md-field"><label for="settings-search-input">Search settings</label><input type="text" id="settings-search-input"></div></div>' +
      '<div id="settings-body"></div>' +
      '<h2>Local history</h2>' +
      '<div class="search-row"><div class="md-field"><label for="history-search-input">Search history</label><input type="text" id="history-search-input"></div></div>' +
      '<div id="history-list"></div>';

    const body = document.getElementById('settings-body');

    function row(id, label, desc, control) {
      const div = document.createElement('div');
      div.className = 'settings-row'; div.dataset.searchText = (label + ' ' + desc).toLowerCase();
      div.innerHTML = '<div><div>' + label + '</div><div class="desc">' + desc + '</div></div>';
      div.appendChild(control);
      return div;
    }
    function makeSwitch(checked, onToggle, label) {
      const btn = document.createElement('button');
      btn.type = 'button'; btn.className = 'md-switch'; btn.setAttribute('role', 'switch');
      btn.setAttribute('aria-checked', String(checked));
      btn.setAttribute('aria-label', label);
      btn.innerHTML = '<span class="track"><span class="thumb"></span></span>';
      btn.addEventListener('click', () => {
        const next = btn.getAttribute('aria-checked') !== 'true';
        btn.setAttribute('aria-checked', String(next));
        onToggle(next);
      });
      return btn;
    }
    function makeSelect(options, value, onChange, label) {
      const sel = document.createElement('select');
      if (label) sel.setAttribute('aria-label', label);
      options.forEach(([v, l]) => { const o = document.createElement('option'); o.value = v; o.textContent = l; sel.appendChild(o); });
      sel.value = value;
      sel.addEventListener('change', () => onChange(sel.value));
      return sel;
    }
    function makeRange(min, max, step, value, onChange, label) {
      const inp = document.createElement('input');
      inp.type = 'range'; inp.min = min; inp.max = max; inp.step = step; inp.value = value;
      if (label) inp.setAttribute('aria-label', label);
      inp.addEventListener('input', () => onChange(parseFloat(inp.value)));
      return inp;
    }

    // Theme
    body.appendChild(row('theme', 'Theme', 'Light, dark, or follow system.',
      makeSelect([['light', 'Light'], ['dark', 'Dark'], ['system', 'System']], settings.theme, (v) => { saveSettings(Object.assign({}, settings, { theme: v }), 'Changed theme to ' + v); notify('Theme set to ' + v); }, 'Theme')));

    // Seed color picker
    const colorSection = document.createElement('div');
    colorSection.className = 'settings-row'; colorSection.dataset.searchText = 'seed color picker hex rgb hsl hsv hwb oklch cmyk';
    colorSection.innerHTML = '<div style="width:100%"><div>Seed color</div><div class="desc">Infinite color picker: hue wheel, saturation/value field, numeric entry, and a format translator.</div><div class="color-picker-wrap" id="color-picker"></div></div>';
    body.appendChild(colorSection);
    buildColorPicker(document.getElementById('color-picker'));

    // Density
    body.appendChild(row('density', 'Density', 'Compact reduces vertical padding.',
      makeSelect([['default', 'Default'], ['compact', 'Compact']], settings.density, (v) => saveSettings(Object.assign({}, settings, { density: v }), 'Changed density to ' + v), 'Density')));

    // Font scale
    const fsRow = document.createElement('div'); fsRow.className = 'settings-row'; fsRow.dataset.searchText = 'font scale size';
    fsRow.innerHTML = '<div><div>Font scale</div><div class="desc">Scales all text, current: <span id="fs-val">' + settings.fontScale + '</span>×</div></div>';
    fsRow.appendChild(makeRange(0.8, 1.6, 0.05, settings.fontScale, (v) => { document.getElementById('fs-val').textContent = v.toFixed(2); saveSettings(Object.assign({}, settings, { fontScale: v }), 'Changed font scale to ' + v); }, 'Font scale'));
    body.appendChild(fsRow);

    // Language
    body.appendChild(row('language', 'Language', 'English, playful Hong Kong-style Cantonese, or bilingual.',
      makeSelect([['en', 'English'], ['yue', 'Cantonese (playful)'], ['bilingual', 'Bilingual']], settings.language, (v) => saveSettings(Object.assign({}, settings, { language: v }), 'Changed language to ' + v), 'Language')));

    const yueSample = document.createElement('p');
    yueSample.lang = 'yue';
    yueSample.textContent = '呢個係花名冊示範文字，用嚟試吓廣東話顯示。';
    const yueRow = document.createElement('div'); yueRow.className = 'settings-row'; yueRow.dataset.searchText = 'cantonese sample yue';
    yueRow.appendChild(yueSample);
    body.appendChild(yueRow);

    // Funny levels
    const feRow = document.createElement('div'); feRow.className = 'settings-row'; feRow.dataset.searchText = 'funny level english tone';
    feRow.innerHTML = '<div><div>English funny level</div><div class="desc">1-5, styles tone only. Facts are unchanged. Current: <span id="fe-val">' + settings.funnyEnglish + '</span></div></div>';
    feRow.appendChild(makeRange(1, 5, 1, settings.funnyEnglish, (v) => { document.getElementById('fe-val').textContent = v; saveSettings(Object.assign({}, settings, { funnyEnglish: v }), 'Changed English funny level to ' + v); }, 'English funny level'));
    body.appendChild(feRow);

    const fcRow = document.createElement('div'); fcRow.className = 'settings-row'; fcRow.dataset.searchText = 'funny level cantonese tone';
    fcRow.innerHTML = '<div><div>Cantonese funny level</div><div class="desc">1-5, styles tone only. Facts are unchanged. Current: <span id="fc-val">' + settings.funnyCantonese + '</span></div></div>';
    fcRow.appendChild(makeRange(1, 5, 1, settings.funnyCantonese, (v) => { document.getElementById('fc-val').textContent = v; saveSettings(Object.assign({}, settings, { funnyCantonese: v }), 'Changed Cantonese funny level to ' + v); }, 'Cantonese funny level'));
    body.appendChild(fcRow);

    // Emoji toggle
    body.appendChild(row('emoji', 'Emoji in dialogs', 'Toggle emoji decoration in dialog copy.',
      makeSwitch(settings.emojiInDialogs, (v) => saveSettings(Object.assign({}, settings, { emojiInDialogs: v }), 'Toggled emoji in dialogs'), 'Emoji in dialogs')));

    // Reduced motion override
    body.appendChild(row('motion', 'Reduced motion override', 'Follow system, force reduce, or force allow motion.',
      makeSelect([['auto', 'Follow system'], ['reduce', 'Reduce'], ['allow', 'Allow']], settings.reducedMotionOverride, (v) => saveSettings(Object.assign({}, settings, { reducedMotionOverride: v }), 'Changed motion override to ' + v), 'Reduced motion override')));

    // ADHD modes
    const adhdTitle = document.createElement('h3'); adhdTitle.textContent = 'ADHD modes'; body.appendChild(adhdTitle);
    const adhdNote = document.createElement('p'); adhdNote.className = 'desc';
    adhdNote.textContent = 'These are workflow preferences, not medical features. All are off by default.';
    body.appendChild(adhdNote);
    [['focus', 'Focus'], ['lowStimulation', 'Low stimulation'], ['timeAwareness', 'Time awareness'], ['oneThing', 'One thing at a time'], ['momentum', 'Momentum']].forEach(([key, label]) => {
      body.appendChild(row('adhd-' + key, label, 'Workflow preference, off by default.',
        makeSwitch(settings.adhd[key], (v) => { const adhd = Object.assign({}, settings.adhd, { [key]: v }); saveSettings(Object.assign({}, settings, { adhd }), 'Toggled ADHD mode: ' + label); }, label)));
    });

    // Personal vocabulary upload
    const vocabTitle = document.createElement('h3'); vocabTitle.textContent = 'Personal vocabulary'; body.appendChild(vocabTitle);
    const vocabWrap = document.createElement('div'); vocabWrap.className = 'settings-row'; vocabWrap.dataset.searchText = 'personal vocabulary upload substitution json';
    vocabWrap.innerHTML = '<div style="width:100%">' +
      '<div class="desc">Upload a JSON file: {"version":1,"entries":[{"from":"...","to":"..."}]}. Bounded to 256 KB and 2000 entries. Applied to visible text only, client-side, never logged.</div>' +
      '<label class="md-field" for="vocab-file"><span>Choose personal vocabulary JSON file</span><input type="file" id="vocab-file" accept="application/json"></label>' +
      '<div id="vocab-status"></div>' +
      '<button type="button" class="md-text-button" id="vocab-clear">Clear</button>' +
      '</div>';
    body.appendChild(vocabWrap);
    const status = vocabWrap.querySelector('#vocab-status');
    function refreshVocabStatus() {
      const v = loadVocab();
      status.textContent = v ? ('Loaded: ' + v.entries.length + ' entries.') : 'No file loaded.';
    }
    refreshVocabStatus();
    vocabWrap.querySelector('#vocab-file').addEventListener('change', (e) => {
      const f = e.target.files[0];
      if (!f) return;
      if (f.size > 256 * 1024) { status.textContent = 'Invalid: file exceeds 256 KB.'; return; }
      const reader = new FileReader();
      reader.onload = () => {
        try {
          const data = JSON.parse(reader.result);
          if (!data || !Array.isArray(data.entries) || data.entries.length > 2000) throw new Error('Bad shape or too many entries');
          localStorage.setItem(LS.vocab, JSON.stringify(data));
          appendHistory('Loaded personal vocabulary (' + data.entries.length + ' entries)');
          status.textContent = 'Loaded: ' + data.entries.length + ' entries.';
          notify('Personal vocabulary loaded');
        } catch (err) { status.textContent = 'Invalid: ' + err.message; }
      };
      reader.readAsText(f);
    });
    vocabWrap.querySelector('#vocab-clear').addEventListener('click', () => {
      localStorage.removeItem(LS.vocab);
      appendHistory('Cleared personal vocabulary');
      refreshVocabStatus();
      notify('Personal vocabulary cleared');
    });

    // Settings search wiring
    window.RegexBuilder.attach(document.getElementById('settings-search-input'), { inventoryId: 'settings-search' });
    document.getElementById('settings-search-input').addEventListener('input', (e) => {
      const q = e.target.value.toLowerCase();
      body.querySelectorAll('.settings-row').forEach((r) => {
        r.style.display = !q || (r.dataset.searchText || '').includes(q) ? '' : 'none';
      });
    });

    // History panel
    function renderHistory(filterText, regexState) {
      const list = loadHistory().slice().reverse();
      const el = document.getElementById('history-list');
      let filtered = list;
      if (regexState && regexState.pattern) {
        try { const re = new RegExp(regexState.pattern, regexState.flags || 'i'); filtered = list.filter((h) => re.test(h.action)); } catch (e) { /* ignore */ }
      } else if (filterText) {
        filtered = list.filter((h) => h.action.toLowerCase().includes(filterText.toLowerCase()));
      }
      el.innerHTML = filtered.slice(0, 200).map((h) => '<div class="history-item">' + new Date(h.at).toLocaleString() + ' · ' + h.action + '</div>').join('') || '<p>No history entries.</p>';
    }
    let historyRegex = null;
    document.getElementById('history-search-input').addEventListener('input', (e) => renderHistory(e.target.value, historyRegex));
    window.RegexBuilder.attach(document.getElementById('history-search-input'), { inventoryId: 'history-search', onApply(state) { historyRegex = state; renderHistory(document.getElementById('history-search-input').value, historyRegex); } });
    renderHistory('', null);
  };

  function buildColorPicker(container) {
    container.innerHTML =
      '<canvas id="hue-wheel" width="140" height="140"></canvas>' +
      '<canvas id="sv-field" width="140" height="140"></canvas>' +
      '<div class="format-grid" id="format-grid"></div>';
    const rgb = window.M3Color.hexToRgb(settings.seedColor);
    let hsv = rgbToHsv(rgb.r, rgb.g, rgb.b);

    const hueCanvas = document.getElementById('hue-wheel');
    const hctx = hueCanvas.getContext('2d');
    drawHueWheel(hctx, 70);
    const svCanvas = document.getElementById('sv-field');
    const sctx = svCanvas.getContext('2d');

    function drawSv() {
      const w = svCanvas.width, h = svCanvas.height;
      const img = sctx.createImageData(w, h);
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          const s = x / w, v = 1 - y / h;
          const [r, g, b] = hsvToRgb(hsv.h, s, v);
          const i = (y * w + x) * 4;
          img.data[i] = r; img.data[i + 1] = g; img.data[i + 2] = b; img.data[i + 3] = 255;
        }
      }
      sctx.putImageData(img, 0, 0);
    }
    drawSv();

    function commit() {
      const [r, g, b] = hsvToRgb(hsv.h, hsv.s, hsv.v);
      const hex = window.M3Color.rgbToHex(r, g, b);
      settings = Object.assign({}, settings, { seedColor: hex });
      saveSettings(settings, 'Changed seed color to ' + hex);
      renderFormats(r, g, b);
    }
    function renderFormats(r, g, b) {
      const grid = document.getElementById('format-grid');
      const formats = colorFormats(r, g, b);
      grid.innerHTML = Object.entries(formats).map(([k, v]) => '<label class="md-field"><span>' + k + '</span><input type="text" value="' + v + '" data-format="' + k + '"></label>').join('');
      grid.querySelectorAll('input').forEach((inp) => {
        inp.addEventListener('change', () => {
          const parsed = parseColorFormat(inp.dataset.format, inp.value);
          if (parsed) { hsv = rgbToHsv(parsed.r, parsed.g, parsed.b); drawSv(); commit(); }
        });
      });
    }
    renderFormats(rgb.r, rgb.g, rgb.b);

    hueCanvas.addEventListener('click', (e) => {
      const rect = hueCanvas.getBoundingClientRect();
      const cx = rect.width / 2, cy = rect.height / 2;
      const dx = e.clientX - rect.left - cx, dy = e.clientY - rect.top - cy;
      let ang = Math.atan2(dy, dx) * 180 / Math.PI; if (ang < 0) ang += 360;
      hsv.h = ang; drawSv(); commit();
    });
    svCanvas.addEventListener('click', (e) => {
      const rect = svCanvas.getBoundingClientRect();
      hsv.s = clamp01((e.clientX - rect.left) / rect.width);
      hsv.v = clamp01(1 - (e.clientY - rect.top) / rect.height);
      commit();
    });
  }
  function clamp01(v) { return Math.max(0, Math.min(1, v)); }
  function drawHueWheel(ctx, radius) {
    const cx = ctx.canvas.width / 2, cy = ctx.canvas.height / 2;
    for (let a = 0; a < 360; a++) {
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.arc(cx, cy, radius, (a - 1) * Math.PI / 180, (a + 1) * Math.PI / 180);
      ctx.closePath();
      const [r, g, b] = hsvToRgb(a, 1, 1);
      ctx.fillStyle = 'rgb(' + r + ',' + g + ',' + b + ')';
      ctx.fill();
    }
  }
  function hsvToRgb(h, s, v) {
    h = ((h % 360) + 360) % 360;
    const c = v * s, x = c * (1 - Math.abs((h / 60) % 2 - 1)), m = v - c;
    let r, g, b;
    if (h < 60) [r, g, b] = [c, x, 0]; else if (h < 120) [r, g, b] = [x, c, 0];
    else if (h < 180) [r, g, b] = [0, c, x]; else if (h < 240) [r, g, b] = [0, x, c];
    else if (h < 300) [r, g, b] = [x, 0, c]; else [r, g, b] = [c, 0, x];
    return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)];
  }
  function rgbToHsv(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b), d = max - min;
    let h = 0;
    if (d !== 0) {
      if (max === r) h = 60 * (((g - b) / d) % 6);
      else if (max === g) h = 60 * ((b - r) / d + 2);
      else h = 60 * ((r - g) / d + 4);
    }
    if (h < 0) h += 360;
    const s = max === 0 ? 0 : d / max;
    return { h, s, v: max };
  }
  function colorFormats(r, g, b) {
    const hex = window.M3Color.rgbToHex(r, g, b);
    const hsv = rgbToHsv(r, g, b);
    const hsl = rgbToHsl(r, g, b);
    const hwb = { h: hsv.h, w: Math.min(r, g, b) / 255, bl: 1 - Math.max(r, g, b) / 255 };
    const cmyk = rgbToCmyk(r, g, b);
    const oklch = rgbToOklch(r, g, b);
    return {
      HEX: hex,
      RGB: 'rgb(' + r + ', ' + g + ', ' + b + ')',
      HSL: 'hsl(' + hsl.h.toFixed(0) + ', ' + (hsl.s * 100).toFixed(0) + '%, ' + (hsl.l * 100).toFixed(0) + '%)',
      HSV: 'hsv(' + hsv.h.toFixed(0) + ', ' + (hsv.s * 100).toFixed(0) + '%, ' + (hsv.v * 100).toFixed(0) + '%)',
      HWB: 'hwb(' + hwb.h.toFixed(0) + ', ' + (hwb.w * 100).toFixed(0) + '%, ' + (hwb.bl * 100).toFixed(0) + '%)',
      OKLCH: 'oklch(' + oklch.l.toFixed(3) + ' ' + oklch.c.toFixed(3) + ' ' + oklch.h.toFixed(1) + ')',
      CMYK: 'cmyk(' + (cmyk.c * 100).toFixed(0) + '%, ' + (cmyk.m * 100).toFixed(0) + '%, ' + (cmyk.y * 100).toFixed(0) + '%, ' + (cmyk.k * 100).toFixed(0) + '%)',
    };
  }
  function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h = 0, s = 0; const l = (max + min) / 2;
    const d = max - min;
    if (d !== 0) {
      s = d / (1 - Math.abs(2 * l - 1));
      if (max === r) h = 60 * (((g - b) / d) % 6);
      else if (max === g) h = 60 * ((b - r) / d + 2);
      else h = 60 * ((r - g) / d + 4);
    }
    if (h < 0) h += 360;
    return { h, s, l };
  }
  function rgbToCmyk(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const k = 1 - Math.max(r, g, b);
    if (k === 1) return { c: 0, m: 0, y: 0, k: 1 };
    return { c: (1 - r - k) / (1 - k), m: (1 - g - k) / (1 - k), y: (1 - b - k) / (1 - k), k };
  }
  function rgbToOklch(r, g, b) {
    // sRGB -> linear -> OKLab -> OKLCH (Björn Ottosson's published formulas).
    const lin = (c) => { c /= 255; return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4); };
    const rl = lin(r), gl = lin(g), bl = lin(b);
    const l = 0.4122214708 * rl + 0.5363325363 * gl + 0.0514459929 * bl;
    const m = 0.2119034982 * rl + 0.6806995451 * gl + 0.1073969566 * bl;
    const s = 0.0883024619 * rl + 0.2817188376 * gl + 0.6299787005 * bl;
    const l_ = Math.cbrt(l), m_ = Math.cbrt(m), s_ = Math.cbrt(s);
    const L = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_;
    const A = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_;
    const B = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_;
    const C = Math.sqrt(A * A + B * B);
    let H = Math.atan2(B, A) * 180 / Math.PI; if (H < 0) H += 360;
    return { l: L, c: C, h: H };
  }
  function parseColorFormat(fmt, value) {
    try {
      if (fmt === 'HEX') return window.M3Color.hexToRgb(value);
      if (fmt === 'RGB') { const m = value.match(/(\d+)[,\s]+(\d+)[,\s]+(\d+)/); if (m) return { r: +m[1], g: +m[2], b: +m[3] }; }
      if (fmt === 'HSL') { const m = value.match(/([\d.]+)[,\s]+([\d.]+)%?[,\s]+([\d.]+)%?/); if (m) { const [r, g, b] = hslToRgb(+m[1], +m[2] / 100, +m[3] / 100); return { r, g, b }; } }
      if (fmt === 'HSV') { const m = value.match(/([\d.]+)[,\s]+([\d.]+)%?[,\s]+([\d.]+)%?/); if (m) { const [r, g, b] = hsvToRgb(+m[1], +m[2] / 100, +m[3] / 100); return { r, g, b }; } }
    } catch (e) { /* ignore, leave unset */ }
    return null;
  }
  function hslToRgb(h, s, l) {
    const c = (1 - Math.abs(2 * l - 1)) * s, x = c * (1 - Math.abs((h / 60) % 2 - 1)), m = l - c / 2;
    let r, g, b;
    if (h < 60) [r, g, b] = [c, x, 0]; else if (h < 120) [r, g, b] = [x, c, 0];
    else if (h < 180) [r, g, b] = [0, c, x]; else if (h < 240) [r, g, b] = [0, x, c];
    else if (h < 300) [r, g, b] = [x, 0, c]; else [r, g, b] = [c, 0, x];
    return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)];
  }

  // ---------- Command palette ----------
  function buildPaletteIndex() {
    const items = [];
    routes.forEach((r) => items.push({ kind: 'Section', label: r.charAt(0).toUpperCase() + r.slice(1), go: () => navigate(r) }));
    DOC_PAGES.forEach((p) => items.push({ kind: 'Doc', label: p.title, go: () => { navigate('docs'); } }));
    const settingLabels = ['Theme', 'Seed color', 'Density', 'Font scale', 'Language', 'English funny level', 'Cantonese funny level', 'Emoji in dialogs', 'Reduced motion override', 'ADHD modes', 'Personal vocabulary'];
    settingLabels.forEach((s) => items.push({ kind: 'Setting', label: s, go: () => navigate('settings') }));
    return items;
  }
  function initPalette() {
    const backdrop = document.getElementById('palette-backdrop');
    const input = document.getElementById('palette-input');
    const results = document.getElementById('palette-results');
    const index = buildPaletteIndex();
    let activeIdx = 0;
    let regexState = null;
    window.RegexBuilder.attach(input, { inventoryId: 'palette-search', onApply(s) { regexState = s; renderResults(); } });

    function renderResults() {
      const q = input.value;
      let filtered;
      if (regexState && regexState.pattern) {
        try { const re = new RegExp(regexState.pattern, regexState.flags || 'i'); filtered = index.filter((it) => re.test(it.label)); } catch (e) { filtered = index; }
      } else {
        filtered = q ? index.filter((it) => it.label.toLowerCase().includes(q.toLowerCase())) : index;
      }
      results.innerHTML = '';
      filtered.slice(0, 30).forEach((it, i) => {
        const div = document.createElement('div');
        div.className = 'palette-result' + (i === activeIdx ? ' active' : '');
        div.setAttribute('role', 'option');
        div.innerHTML = '<span>' + it.label + '</span><span class="kind">' + it.kind + '</span>';
        div.addEventListener('click', () => { it.go(); closePalette(); });
        results.appendChild(div);
      });
    }
    function openPalette() {
      backdrop.hidden = false; input.value = ''; activeIdx = 0; renderResults(); input.focus();
      document.addEventListener('keydown', onKey);
    }
    function closePalette() { backdrop.hidden = true; document.removeEventListener('keydown', onKey); }
    function onKey(e) {
      if (e.key === 'Escape') closePalette();
    }
    input.addEventListener('input', renderResults);
    document.getElementById('palette-btn').addEventListener('click', openPalette);
    backdrop.addEventListener('click', (e) => { if (e.target === backdrop) closePalette(); });
    document.addEventListener('keydown', (e) => {
      if (e.ctrlKey && e.shiftKey && (e.key === 'F' || e.key === 'f')) { e.preventDefault(); openPalette(); }
    });
  }

  // ---------- Notification centre wiring ----------
  function initNotifCentre() {
    const panel = document.getElementById('notif-panel');
    document.getElementById('notif-btn').addEventListener('click', () => panel.classList.toggle('open'));
    document.getElementById('notif-close').addEventListener('click', () => panel.classList.remove('open'));
  }

  // ---------- Boot ----------
  window.addEventListener('hashchange', render);
  document.addEventListener('DOMContentLoaded', () => {
    applyTheme();
    initPalette();
    initNotifCentre();
    render();
    notify('Welcome to Material Audacity');
  });
})();
