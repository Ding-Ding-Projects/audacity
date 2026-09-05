/*
 * regex-builder.js · anchored regex builder workbench, reused by every
 * search/filter field on the Material Audacity docs site.
 *
 * Exposes RegexBuilder.attach(field, options) which wires a small "." button
 * next to a text input/field that opens a popover with:
 *  - plain-text vs regex mode toggle (plain is default)
 *  - guided construction (character classes, anchors, groups, quantifiers,
 *    lookaround, backreferences, flags)
 *  - live match list + capture table against sample text (or the field's
 *    own candidate list, when supplied)
 *  - replacement preview
 *  - token-by-token explanation
 *  - saved test cases (localStorage), import/export JSON
 *  - a bounded, timed test run with a naive nested-quantifier warning
 */
(function (global) {
  'use strict';

  const LS_KEY = 'ma.regexBuilder.savedTests.v1';
  const MAX_TEST_INPUT = 20000; // bounded input length for timing tests

  function loadSaved() {
    try { return JSON.parse(localStorage.getItem(LS_KEY) || '[]'); } catch (e) { return []; }
  }
  function saveSaved(list) {
    try { localStorage.setItem(LS_KEY, JSON.stringify(list)); } catch (e) { /* ignore */ }
  }

  function hasNestedQuantifier(pattern) {
    // Very naive heuristic: a quantified group containing another quantified
    // token, e.g. (a+)+ or (a*)*, which is the classic catastrophic-backtracking shape.
    return /\([^()]*[+*][^()]*\)[+*]/.test(pattern);
  }

  function explainPattern(pattern) {
    const tokens = [];
    let i = 0;
    const push = (t, d) => tokens.push({ token: t, desc: d });
    while (i < pattern.length) {
      const c = pattern[i];
      if (c === '\\') {
        const n = pattern[i + 1] || '';
        push('\\' + n, escDesc(n));
        i += 2; continue;
      }
      if (c === '(') {
        if (pattern.startsWith('(?:', i)) { push('(?:', 'non-capturing group start'); i += 3; continue; }
        if (pattern.startsWith('(?<', i) && !pattern.startsWith('(?<=', i) && !pattern.startsWith('(?<!', i)) {
          const end = pattern.indexOf('>', i);
          const name = pattern.slice(i + 3, end);
          push('(?<' + name + '>', 'named group "' + name + '" start');
          i = end + 1; continue;
        }
        if (pattern.startsWith('(?=', i)) { push('(?=', 'positive lookahead start'); i += 3; continue; }
        if (pattern.startsWith('(?!', i)) { push('(?!', 'negative lookahead start'); i += 3; continue; }
        if (pattern.startsWith('(?<=', i)) { push('(?<=', 'positive lookbehind start'); i += 4; continue; }
        if (pattern.startsWith('(?<!', i)) { push('(?<!', 'negative lookbehind start'); i += 4; continue; }
        push('(', 'capturing group start'); i += 1; continue;
      }
      if (c === ')') { push(')', 'group end'); i += 1; continue; }
      if (c === '[') {
        const end = pattern.indexOf(']', i + 1);
        const cls = pattern.slice(i, end + 1);
        push(cls, 'character class'); i = end + 1; continue;
      }
      if ('^$.'.includes(c)) {
        push(c, c === '^' ? 'start of string/line' : c === '$' ? 'end of string/line' : 'any character');
        i += 1; continue;
      }
      if (c === '?' && pattern[i + 1] === '?') { push('??', 'lazy optional (0 or 1, lazy)'); i += 2; continue; }
      if ('*+?'.includes(c)) {
        const lazy = pattern[i + 1] === '?';
        push(c + (lazy ? '?' : ''), quantDesc(c) + (lazy ? ', lazy' : ''));
        i += lazy ? 2 : 1; continue;
      }
      if (c === '{') {
        const end = pattern.indexOf('}', i);
        const q = pattern.slice(i, end + 1);
        const lazy = pattern[end + 1] === '?';
        push(q + (lazy ? '?' : ''), 'repeat ' + q + (lazy ? ' (lazy)' : ''));
        i = end + 1 + (lazy ? 1 : 0); continue;
      }
      if (c === '|') { push('|', 'alternation (or)'); i += 1; continue; }
      push(c, 'literal "' + c + '"'); i += 1;
    }
    return tokens;
  }
  function escDesc(n) {
    const map = {
      d: 'digit [0-9]', D: 'non-digit', w: 'word character', W: 'non-word character',
      s: 'whitespace', S: 'non-whitespace', b: 'word boundary', B: 'non-word-boundary',
      n: 'newline', t: 'tab',
    };
    if (map[n]) return 'escaped: ' + map[n];
    if (/[1-9]/.test(n)) return 'backreference to group ' + n;
    return 'escaped literal "' + n + '"';
  }
  function quantDesc(c) {
    return c === '*' ? '0 or more' : c === '+' ? '1 or more' : 'optional (0 or 1)';
  }

  function compile(pattern, flags) {
    try { return { re: new RegExp(pattern, flags), error: null }; }
    catch (e) { return { re: null, error: e.message }; }
  }

  function runMatches(re, text) {
    const out = [];
    if (!re.global) re = new RegExp(re.source, re.flags + 'g');
    let m; let guard = 0;
    while ((m = re.exec(text)) && guard < 5000) {
      out.push({ match: m[0], index: m.index, groups: m.slice(1), named: m.groups || {} });
      if (m[0] === '') re.lastIndex++;
      guard++;
    }
    return out;
  }

  let seq = 0;
  function attach(field, opts) {
    opts = opts || {};
    const id = 'rgx-' + (++seq);
    const wrap = document.createElement('span');
    wrap.className = 'rgx-anchor';
    field.insertAdjacentElement('afterend', wrap);

    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'md-icon-button rgx-toggle-btn';
    btn.setAttribute('aria-haspopup', 'dialog');
    btn.setAttribute('aria-expanded', 'false');
    btn.title = 'Regex builder for ' + (opts.label || field.name || field.id || 'this field');
    btn.textContent = '.*';
    wrap.appendChild(btn);

    const pop = document.createElement('div');
    pop.className = 'rgx-popover';
    pop.id = id;
    pop.hidden = true;
    pop.setAttribute('role', 'dialog');
    pop.setAttribute('aria-label', 'Regex builder');
    pop.innerHTML =
      '<div class="rgx-row">' +
      '<label class="rgx-mode"><input type="checkbox" class="rgx-enable"> Use regex</label>' +
      '<label>Flags <input type="text" class="rgx-flags" value="i" size="4" aria-label="flags"></label>' +
      '</div>' +
      '<textarea class="rgx-pattern" rows="2" placeholder="Pattern, e.g. ^(?<word>\\w+)$" aria-label="Regular expression pattern"></textarea>' +
      '<div class="rgx-error" role="alert"></div>' +
      '<details class="rgx-guided"><summary>Guided tokens</summary><div class="rgx-chips"></div></details>' +
      '<textarea class="rgx-sample" rows="3" placeholder="Sample text to test against" aria-label="Sample text to test against"></textarea>' +
      '<div class="rgx-replace-row"><input type="text" class="rgx-replacement" placeholder="Replacement (optional)" aria-label="Replacement"><div class="rgx-replace-preview"></div></div>' +
      '<div class="rgx-timing"></div>' +
      '<div class="rgx-matches" aria-live="polite"></div>' +
      '<details class="rgx-explain"><summary>Explanation</summary><ol class="rgx-explain-list"></ol></details>' +
      '<div class="rgx-saved-row">' +
      '<button type="button" class="md-text-button rgx-save">Save test case</button>' +
      '<button type="button" class="md-text-button rgx-export">Export JSON</button>' +
      '<button type="button" class="md-text-button rgx-import">Import JSON</button>' +
      '<button type="button" class="md-text-button rgx-close">Close</button>' +
      '</div>' +
      '<ul class="rgx-saved-list"></ul>';
    wrap.appendChild(pop);

    const chips = [
      ['\\d', 'digit'], ['\\w', 'word char'], ['\\s', 'whitespace'], ['.', 'any'],
      ['^', 'start'], ['$', 'end'], ['*', '0+'], ['+', '1+'], ['?', '0/1'],
      ['{2,4}', 'range'], ['(...)', 'group'], ['(?:...)', 'non-capture'],
      ['(?<name>...)', 'named group'], ['\\1', 'backreference'],
      ['(?=...)', 'lookahead'], ['(?!...)', 'neg. lookahead'],
      ['(?<=...)', 'lookbehind'], ['(?<!...)', 'neg. lookbehind'], ['|', 'or'],
    ];
    const chipsEl = pop.querySelector('.rgx-chips');
    chips.forEach(([tok, label]) => {
      const c = document.createElement('button');
      c.type = 'button';
      c.className = 'rgx-chip';
      c.textContent = tok;
      c.title = label;
      c.addEventListener('click', () => {
        const ta = pop.querySelector('.rgx-pattern');
        const pos = ta.selectionStart || ta.value.length;
        ta.value = ta.value.slice(0, pos) + tok + ta.value.slice(pos);
        ta.focus();
        update();
      });
      chipsEl.appendChild(c);
    });

    const enableEl = pop.querySelector('.rgx-enable');
    const flagsEl = pop.querySelector('.rgx-flags');
    const patternEl = pop.querySelector('.rgx-pattern');
    const errorEl = pop.querySelector('.rgx-error');
    const sampleEl = pop.querySelector('.rgx-sample');
    const replEl = pop.querySelector('.rgx-replacement');
    const replPreview = pop.querySelector('.rgx-replace-preview');
    const timingEl = pop.querySelector('.rgx-timing');
    const matchesEl = pop.querySelector('.rgx-matches');
    const explainEl = pop.querySelector('.rgx-explain-list');
    const savedListEl = pop.querySelector('.rgx-saved-list');

    if (opts.sample) sampleEl.value = opts.sample;

    function currentActive() {
      return enableEl.checked;
    }

    function update() {
      const pattern = patternEl.value;
      const flags = flagsEl.value.replace(/[^gimsuy]/g, '');
      errorEl.textContent = '';
      matchesEl.innerHTML = '';
      explainEl.innerHTML = '';
      timingEl.textContent = '';
      replPreview.textContent = '';

      if (!pattern) { return; }
      const { re, error } = compile(pattern, flags);
      if (error) { errorEl.textContent = 'Invalid pattern: ' + error; return; }

      if (hasNestedQuantifier(pattern)) {
        const warn = document.createElement('div');
        warn.className = 'rgx-warning';
        warn.textContent = 'Warning: nested quantifiers detected (e.g. (a+)+). This pattern can backtrack catastrophically on adversarial input.';
        matchesEl.appendChild(warn);
      }

      explainPattern(pattern).forEach((t) => {
        const li = document.createElement('li');
        li.innerHTML = '<code>' + escapeHtml(t.token) + '</code> · ' + escapeHtml(t.desc);
        explainEl.appendChild(li);
      });

      const sample = sampleEl.value.slice(0, MAX_TEST_INPUT);
      const t0 = performance.now();
      const matches = runMatches(re, sample);
      const t1 = performance.now();
      timingEl.textContent = matches.length + ' match(es) in ' + (t1 - t0).toFixed(2) + ' ms over ' + sample.length + ' chars' + (sample.length >= MAX_TEST_INPUT ? ' (truncated to bound)' : '');

      if (matches.length) {
        const table = document.createElement('table');
        table.className = 'rgx-table';
        table.innerHTML = '<thead><tr><th>#</th><th>Match</th><th>Index</th><th>Groups</th></tr></thead>';
        const tbody = document.createElement('tbody');
        matches.slice(0, 200).forEach((m, idx) => {
          const tr = document.createElement('tr');
          const groupStr = m.groups.map((g, i2) => (i2 + 1) + ':' + (g == null ? '∅' : g)).join(', ') +
            (Object.keys(m.named).length ? ' | named: ' + Object.entries(m.named).map(([k, v]) => k + '=' + v).join(', ') : '');
          tr.innerHTML = '<td>' + (idx + 1) + '</td><td><code>' + escapeHtml(m.match) + '</code></td><td>' + m.index + '</td><td>' + escapeHtml(groupStr) + '</td>';
          tbody.appendChild(tr);
        });
        table.appendChild(tbody);
        matchesEl.appendChild(table);
      }

      if (replEl.value) {
        try { replPreview.textContent = sample.replace(new RegExp(pattern, flags.includes('g') ? flags : flags + 'g'), replEl.value); }
        catch (e) { replPreview.textContent = 'Replacement error: ' + e.message; }
      }

      if (typeof opts.onChange === 'function' && currentActive()) {
        opts.onChange({ pattern, flags, re });
      }
    }

    function applyToField() {
      if (currentActive() && patternEl.value) {
        field.dataset.regexActive = 'true';
        field.dataset.regexPattern = patternEl.value;
        field.dataset.regexFlags = flagsEl.value;
      } else {
        delete field.dataset.regexActive;
      }
      if (typeof opts.onApply === 'function') opts.onApply(currentActive() ? { pattern: patternEl.value, flags: flagsEl.value } : null);
    }

    [enableEl, flagsEl].forEach((el) => el.addEventListener('input', () => { update(); applyToField(); }));
    patternEl.addEventListener('input', () => { update(); applyToField(); });
    sampleEl.addEventListener('input', update);
    replEl.addEventListener('input', update);

    function renderSaved() {
      savedListEl.innerHTML = '';
      loadSaved().filter((s) => s.field === (opts.inventoryId || '')).forEach((s, idx) => {
        const li = document.createElement('li');
        li.innerHTML = '<code>' + escapeHtml(s.pattern) + '</code> <span class="rgx-flags-tag">' + escapeHtml(s.flags) + '</span>';
        const use = document.createElement('button');
        use.type = 'button'; use.className = 'md-text-button'; use.textContent = 'Use';
        use.addEventListener('click', () => { patternEl.value = s.pattern; flagsEl.value = s.flags; enableEl.checked = true; update(); applyToField(); });
        li.appendChild(use);
        savedListEl.appendChild(li);
      });
    }

    pop.querySelector('.rgx-save').addEventListener('click', () => {
      if (!patternEl.value) return;
      const all = loadSaved();
      all.push({ field: opts.inventoryId || '', pattern: patternEl.value, flags: flagsEl.value, savedAt: new Date().toISOString() });
      saveSaved(all);
      renderSaved();
    });
    pop.querySelector('.rgx-export').addEventListener('click', () => {
      const blob = JSON.stringify(loadSaved().filter((s) => s.field === (opts.inventoryId || '')), null, 2);
      window.prompt('Copy exported test cases JSON:', blob);
    });
    pop.querySelector('.rgx-import').addEventListener('click', () => {
      const text = window.prompt('Paste test cases JSON to import:');
      if (!text) return;
      try {
        const arr = JSON.parse(text);
        const all = loadSaved().concat(arr.map((a) => Object.assign({ field: opts.inventoryId || '' }, a)));
        saveSaved(all);
        renderSaved();
      } catch (e) { errorEl.textContent = 'Import failed: ' + e.message; }
    });
    pop.querySelector('.rgx-close').addEventListener('click', close);

    function open() {
      pop.hidden = false;
      btn.setAttribute('aria-expanded', 'true');
      renderSaved();
      patternEl.focus();
      document.addEventListener('keydown', onKey);
    }
    function close() {
      pop.hidden = true;
      btn.setAttribute('aria-expanded', 'false');
      document.removeEventListener('keydown', onKey);
      btn.focus();
    }
    function onKey(e) { if (e.key === 'Escape') close(); }

    btn.addEventListener('click', () => { pop.hidden ? open() : close(); });

    return { open, close, element: pop };
  }

  function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  }

  global.RegexBuilder = { attach, explainPattern, hasNestedQuantifier, compile };
})(window);
