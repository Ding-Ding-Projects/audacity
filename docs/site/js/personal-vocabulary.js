(function (root) {
  'use strict';
  const LIMITS = Object.freeze({ bytes: 262144, entries: 4096, key: 160, value: 1000, text: 65536 });
  const unsafe = new Set(['__proto__', 'prototype', 'constructor']);
  function parse(text) {
    function invalid() { throw new Error('Invalid personal vocabulary file.'); }
    if (typeof text !== 'string' || new TextEncoder().encode(text).length > LIMITS.bytes) invalid();
    let i = 0;
    const space = () => { while (/[\t\n\r ]/.test(text[i] || '\u0000')) i++; };
    function token(value) { space(); if (text[i] !== value) invalid(); i++; }
    function string() {
      space(); const start = i; token('"');
      while (i < text.length) {
        const c = text[i++];
        if (c === '"') {
          let value; try { value = JSON.parse(text.slice(start, i)); } catch (_) { invalid(); }
          for (let p = 0; p < value.length; p++) {
            const n = value.charCodeAt(p);
            if (n >= 0xd800 && n <= 0xdbff) { const next = value.charCodeAt(++p); if (!(next >= 0xdc00 && next <= 0xdfff)) invalid(); }
            else if (n >= 0xdc00 && n <= 0xdfff) invalid();
          }
          return value;
        }
        if (c === '\\') i++;
      }
      invalid();
    }
    function object(readValue, max) {
      token('{'); const result = Object.create(null); let count = 0;
      space(); if (text[i] === '}') { i++; return result; }
      for (;;) {
        const key = string();
        if (Object.hasOwn(result, key) || unsafe.has(key) || ++count > max) invalid();
        token(':'); result[key] = readValue(key); space();
        if (text[i] === '}') { i++; return result; }
        token(',');
      }
    }
    const record = object(key => {
      if (key === 'schemaVersion') { space(); if (text[i++] !== '1') invalid(); return 1; }
      if (key === 'entries') return object(entry => {
        if (!entry || Array.from(entry).length > LIMITS.key) invalid();
        const value = string();
        if (Array.from(value).length > LIMITS.value || /[\u0000-\u0008\u000b\u000c\u000e-\u001f\u007f]/u.test(value)) invalid();
        return value;
      }, LIMITS.entries);
      invalid();
    }, 2);
    space();
    if (i !== text.length || record.schemaVersion !== 1 || !record.entries) invalid();
    return Object.freeze({ schemaVersion: 1, entries: Object.freeze(record.entries) });
  }
  function createReplacer(record) {
    const trie = new Map();
    for (const [key, value] of Object.entries(record.entries)) {
      let node = trie;
      for (const c of key) { if (!node.has(c)) node.set(c, new Map()); node = node.get(c); }
      node.value = value;
    }
    const word = c => !!c && /[A-Za-z0-9_]/u.test(c);
    return text => {
      if (text.length > LIMITS.text) return text;
      const chars = Array.from(text); const out = []; let length = 0;
      for (let i = 0; i < chars.length;) {
        let node = trie; let end = i; let replacement;
        for (let j = i; j < chars.length && node.has(chars[j]); j++) {
          node = node.get(chars[j]);
          if (node.value !== undefined && !(word(chars[i]) && word(chars[i - 1]))
              && !(word(chars[j]) && word(chars[j + 1]))) { end = j + 1; replacement = node.value; }
        }
        const value = replacement === undefined ? chars[i++] : (i = end, replacement);
        length += value.length; if (length > LIMITS.text) return text;
        out.push(value);
      }
      return out.join('');
    };
  }
  const api = Object.freeze({ parse, createReplacer, limits: LIMITS });
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.PersonalVocabulary = api;
})(typeof window === 'undefined' ? globalThis : window);
