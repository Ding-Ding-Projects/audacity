(function (root, factory) {
  if (typeof module !== 'undefined' && module.exports) module.exports = factory(require('../locales/yue-HK.json'));
  else root.MaterialAudacityPresentation = factory(root.MaterialAudacityYue);
})(typeof window === 'undefined' ? globalThis : window, function (data) {
  'use strict';
  const dictionary = Object.assign(Object.create(null), data && data.strings, data && data.regexBuilder);
  const ready = Object.keys(dictionary).length > 0;
  const placeholders = /\{([a-zA-Z][a-zA-Z0-9]*)\}/g;
  const escapePattern = value => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const templates = Object.keys(dictionary).filter(key => /\{[a-zA-Z][a-zA-Z0-9]*\}/.test(key)).map(key => {
    const names = [...key.matchAll(placeholders)].map(match => match[1]);
    const translatedNames = [...dictionary[key].matchAll(placeholders)].map(match => match[1]);
    if (names.length > 8 || new Set(names).size !== names.length || names.slice().sort().join('|') !== translatedNames.sort().join('|')) throw new TypeError('Invalid translation placeholders.');
    const segments = key.split(placeholders);
    return { key, names, pattern: new RegExp('^' + segments.map((part, index) => index % 2 ? '([^\\r\\n]{1,4096}?)' : escapePattern(part)).join('') + '$') };
  });
  function resolveTemplate(source) {
    if (source.length > 16384) return null;
    for (const template of templates) {
      const match = template.pattern.exec(source);
      if (match) return { ...template, values: Object.fromEntries(template.names.map((name, index) => [name, match[index + 1]])) };
    }
    return null;
  }
  function templateParts(template, language, values) {
    return template.split(placeholders).map((part, index) => index % 2
      ? { language, text: values[part], data: true, dataLanguage: 'en' }
      : { language, text: part }).filter(part => part.text !== '');
  }
  const feedback = {
    'Personal vocabulary loaded': {
      en: ['Personal vocabulary loaded', 'Personal vocabulary loaded. Your local wording is ready.', 'Personal vocabulary loaded. The interface has your phrasebook.', 'Personal vocabulary loaded. The words have read the memo.', 'Personal vocabulary loaded. The words finally did their homework.'],
      yue: ['個人詞彙已載入。', '個人詞彙已載入，本機用字準備好。', '個人詞彙已載入，介面有你本詞彙簿。', '個人詞彙已載入，文字已經收到通知。', '個人詞彙已載入，文字終於做齊功課。'],
    },
    'Personal vocabulary cleared': {
      en: ['Personal vocabulary cleared', 'Personal vocabulary cleared. Original wording restored.', 'Personal vocabulary cleared. The original words are back.', 'Personal vocabulary cleared. The words are back in their own seats.', 'Personal vocabulary cleared. The phrasebook has clocked out.'],
      yue: ['個人詞彙已清除。', '個人詞彙已清除，原本用字已還原。', '個人詞彙已清除，原本啲字返晒嚟。', '個人詞彙已清除，啲字坐返自己個位。', '個人詞彙已清除，詞彙簿收工喇。'],
    },
  };
  function level(value) { return Number.isInteger(value) ? Math.max(1, Math.min(5, value)) - 1 : 0; }
  function parts(source, options = {}) {
    if (typeof source !== 'string' || !source.trim()) return [{ language: 'en', text: source }];
    const key = source.trim();
    const language = options.forcePlainEnglish ? 'en' : options.language || 'en';
    const template = !Object.hasOwn(dictionary, key) && resolveTemplate(key);
    if (template) {
      const englishParts = templateParts(template.key, 'en', template.values);
      const cantoneseParts = templateParts(dictionary[template.key], 'yue', template.values);
      const prefix = source.slice(0, source.indexOf(key));
      const suffix = source.slice(source.indexOf(key) + key.length);
      for (const group of [englishParts, cantoneseParts]) {
        if (prefix) group.unshift({ language: group[0].language, text: prefix });
        if (suffix) group.push({ language: group[group.length - 1].language, text: suffix });
      }
      return language === 'yue' ? cantoneseParts : language === 'bilingual' ? englishParts.concat(cantoneseParts) : englishParts;
    }
    let english = key;
    let cantonese = dictionary[key];
    if (Object.hasOwn(feedback, key) && !options.control) {
      english = feedback[key].en[level(options.forcePlainEnglish ? 1 : options.funnyEnglish)];
      cantonese = feedback[key].yue[level(options.forcePlainEnglish ? 1 : options.funnyCantonese)];
    }
    if (cantonese === undefined || language === 'en') return [{ language: 'en', text: source.replace(key, () => english) }];
    if (language === 'yue') return [{ language: 'yue', text: source.replace(key, () => cantonese) }];
    if (language === 'bilingual') return [{ language: 'en', text: source.replace(key, () => english) }, { language: 'yue', text: cantonese }];
    return [{ language: 'en', text: source.replace(key, () => english) }];
  }
  function text(source, options = {}) {
    return joinParts(parts(source, options));
  }
  function joinParts(values) {
    let lastLanguage;
    return values.map(part => {
      const separator = lastLanguage && lastLanguage !== part.language ? ' / ' : '';
      lastLanguage = part.language;
      return separator + part.text;
    }).join('');
  }
  function provenance(version, localTime, language) {
    return joinParts(provenanceParts(version, localTime, language));
  }
  function provenanceParts(version, localTime, language) {
    const key = 'Documentation version: {version} · Source updated: {localTime}';
    const english = templateParts(key, 'en', { version, localTime });
    if (!Object.hasOwn(dictionary, key)) return english;
    const cantonese = templateParts(dictionary[key], 'yue', { version, localTime });
    return language === 'yue' ? cantonese : language === 'bilingual' ? english.concat(cantonese) : english;
  }
  return Object.freeze({ ready, text, parts, has: source => typeof source === 'string' && (Object.hasOwn(dictionary, source.trim()) || Object.hasOwn(feedback, source.trim()) || !!resolveTemplate(source.trim())), provenance, provenanceParts, staticStrings: Object.keys(dictionary).length });
});
