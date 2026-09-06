const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const presentation = require('../js/presentation.js');
const catalog = require('../locales/yue-HK.json');

test('browser catalog exactly matches the maintained JSON without injected globals', () => {
  const context = vm.createContext({});
  vm.runInContext(fs.readFileSync(path.join(__dirname, '../locales/yue-HK.js'), 'utf8'), context);
  assert.deepEqual(JSON.parse(JSON.stringify(context.MaterialAudacityYue)), catalog);
  assert.deepEqual(Object.keys(context), ['MaterialAudacityYue']);
});

test('every catalog string supports reversible independent language selection', () => {
  for (const [english, cantonese] of Object.entries({ ...catalog.strings, ...catalog.regexBuilder })) {
    assert.equal(typeof cantonese, 'string');
    assert.ok(cantonese.trim());
    assert.equal(presentation.text(english, { language: 'en', control: true }), english);
    assert.equal(presentation.text(english, { language: 'yue', control: true }), cantonese);
    assert.deepEqual(presentation.parts(english, { language: 'bilingual', control: true }), [
      { language: 'en', text: english }, { language: 'yue', text: cantonese },
    ]);
    assert.equal(presentation.text(english, { language: 'en', control: true }), english);
  }
});

test('unknown text and interpolation-looking values are preserved literally', () => {
  for (const source of ['constructor', '__proto__', 'unknown <b>value</b>', '  ', '$& $1']) {
    assert.equal(presentation.text(source, { language: 'yue' }), source);
  }
  assert.equal(presentation.has(null), false);
  assert.equal(presentation.has(42), false);
  assert.equal(presentation.text('  Home  ', { language: 'yue' }), '  主頁  ');
});

test('independent feedback levels preserve control wording and plain-English override', () => {
  const source = 'Personal vocabulary loaded';
  const options = { language: 'bilingual', funnyEnglish: 5, funnyCantonese: 2 };
  assert.deepEqual(presentation.parts(source, options), [
    { language: 'en', text: 'Personal vocabulary loaded. The words finally did their homework.' },
    { language: 'yue', text: '個人詞彙已載入，本機用字準備好。' },
  ]);
  assert.equal(presentation.text(source, { ...options, control: true, language: 'en' }), source);
  assert.equal(presentation.text(source, { ...options, forcePlainEnglish: true }), source);
  assert.equal(presentation.text(source, { language: 'en', funnyEnglish: 99 }), presentation.text(source, { language: 'en', funnyEnglish: 5 }));
});

test('localized provenance preserves the exact supplied version and timestamp', () => {
  const version = 'v4.0.0-m3.14+0123456789';
  const timestamp = '2026-09-06 17:42:09 GMT-04:00';
  for (const language of ['en', 'yue', 'bilingual']) {
    const value = presentation.provenance(version, timestamp, language);
    assert.equal(value.split(version).length - 1, language === 'bilingual' ? 2 : 1);
    assert.equal(value.split(timestamp).length - 1, language === 'bilingual' ? 2 : 1);
    const parts = presentation.provenanceParts(version, timestamp, language);
    assert.ok(parts.filter(part => !part.data).every(part => part.language === language || language === 'bilingual'));
    assert.ok(parts.filter(part => part.data).every(part => part.dataLanguage === 'en'));
    assert.deepEqual(parts.filter(part => part.data).map(part => part.text), language === 'bilingual' ? [version, timestamp, version, timestamp] : [version, timestamp]);
  }
});

test('missing browser catalog reports unavailable and retains original wording', () => {
  const context = vm.createContext({});
  vm.runInContext(fs.readFileSync(path.join(__dirname, '../js/presentation.js'), 'utf8'), context);
  assert.equal(context.MaterialAudacityPresentation.ready, false);
  assert.equal(context.MaterialAudacityPresentation.text('Home', { language: 'yue' }), 'Home');
  assert.ok(context.MaterialAudacityPresentation.provenanceParts('source-abc', 'time', 'yue').every(part => part.language === 'en'));
});

test('concrete parameterized copy translates while marking values as unchanged data', () => {
  const source = 'Close 3 tab(s) containing "<query>&$1"?';
  assert.equal(presentation.has(source), true);
  assert.equal(presentation.text(source, { language: 'yue' }), '要關閉 3 個包含「<query>&$1」嘅分頁嗎？');
  const parts = presentation.parts(source, { language: 'bilingual' });
  assert.deepEqual(parts.filter(part => part.data).map(part => part.text), ['3', '<query>&$1', '3', '<query>&$1']);
  assert.ok(parts.filter(part => part.data).every(part => part.dataLanguage === 'en'));
  assert.equal(presentation.text('Changed language to yue', { language: 'yue' }), '語言已改為 yue');
  assert.equal(presentation.text('Theme: dark · Lane: source', { language: 'yue' }), '主題：dark · 工作線：source');
  assert.equal(presentation.has('No tabs match "' + 'x'.repeat(4097) + '"'), false);
  assert.equal(presentation.text('  Changed language to yue  ', { language: 'yue' }), '  語言已改為 yue  ');
  assert.equal(presentation.text('  Changed language to yue  ', { language: 'en' }), '  Changed language to yue  ');
});
