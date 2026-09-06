const { test } = require('node:test');
const assert = require('node:assert/strict');
const { Narrator, normalize } = require('../js/narrator.js');
const en = { voiceURI: 'engine/en-1', name: 'Shared name', lang: 'en-CA', localService: true, default: true };
const yue = { voiceURI: 'engine/yue-1', name: 'Shared name', lang: 'zh-HK', localService: true };
function fixture(voices = [en, yue]) {
  const timers = new Map(), listeners = new Map(), spoken = []; let sequence = 0, cancelCount = 0;
  const synth = { getVoices: () => voices, speak: utterance => spoken.push(utterance), cancel: () => cancelCount++,
    addEventListener: (key, fn) => listeners.set(key, fn), removeEventListener: key => listeners.delete(key) };
  const host = { speechSynthesis: synth, SpeechSynthesisUtterance: class { constructor(text) { this.text = text; } }, navigator: { onLine: true },
    setTimeout: (fn, delay) => { timers.set(++sequence, { fn, delay }); return sequence; }, clearTimeout: id => timers.delete(id),
    addEventListener: (key, fn) => listeners.set(key, fn), removeEventListener: key => listeners.delete(key) };
  const narrator = new Narrator(host);
  const tick = () => { const entry = [...timers].sort((a, b) => a[1].delay - b[1].delay)[0]; if (entry) { timers.delete(entry[0]); entry[1].fn(); } };
  return { narrator, host, spoken, timers, listeners, tick, cancelCount: () => cancelCount, setVoices: next => { voices = next; listeners.get('voiceschanged')(); } };
}
test('off by default and never invokes speech before opt-in', () => {
  const f = fixture(); assert.equal(f.narrator.enqueue('event', 'Hello', '你好'), false); assert.equal(f.timers.size, 0);
});
test('missing platform remains unavailable', () => {
  const f = fixture(); f.narrator.dispose(); delete f.host.speechSynthesis;
  const n = new Narrator(f.host); n.configure({ enabled: true });
  assert.equal(n.available(), false); assert.equal(n.enqueue('event', 'Hello', '你好'), false);
});
test('late enumeration updates choices and preserves requested stable identity', () => {
  const f = fixture([]); f.narrator.configure({ enabled: true, englishVoice: en.voiceURI });
  assert.equal(f.narrator.voiceStatus('en').missingSelection, true);
  f.setVoices([en, yue]); assert.equal(f.narrator.voiceStatus('en').effective, en);
  assert.equal(f.narrator.voiceStatus('yue').effective, yue);
  f.setVoices([{ ...en, voiceURI: 'replacement' }]);
  assert.equal(f.narrator.voiceStatus('en').requested, en.voiceURI);
  assert.equal(f.narrator.voiceStatus('en').missingSelection, true);
});
test('does not substitute Mandarin for missing Cantonese', () => {
  const f = fixture([en, { ...yue, lang: 'zh-CN' }]);
  assert.equal(f.narrator.voiceStatus('yue').effective, undefined);
});
test('both languages are strictly serialized', () => {
  const f = fixture(); f.narrator.configure({ enabled: true, language: 'both' });
  f.narrator.enqueue('event', 'Hello', '你好'); f.tick();
  assert.equal(f.spoken.length, 1); assert.equal(f.spoken[0].voice.voiceURI, en.voiceURI);
  f.spoken[0].onend(); assert.equal(f.spoken.length, 2); assert.equal(f.spoken[1].text, '你好');
  assert.equal(f.spoken[1].voice.voiceURI, yue.voiceURI); f.spoken[1].onend(); assert.equal(f.timers.size, 0);
});
test('latest queued ordinary event replaces its predecessor before debounce', () => {
  const f = fixture(); f.narrator.configure({ enabled: true });
  f.narrator.enqueue('theme', 'First', '一'); f.narrator.enqueue('theme', 'Second', '二'); f.tick();
  assert.equal(f.spoken.length, 1); assert.equal(f.spoken[0].text, 'Second');
});
test('important events bypass category cooldown', () => {
  const f = fixture(); f.narrator.configure({ enabled: true });
  f.narrator.enqueue('operation', 'Done', '完成'); f.tick(); f.spoken[0].onend();
  assert.equal(f.narrator.enqueue('operation', 'Again', '再一次'), false);
  assert.equal(f.narrator.enqueue('operation', 'Operation failed. Retry.', '操作失敗，請重試。', true), true);
  f.tick(); assert.equal(f.spoken.length, 2);
});
test('network voice is disclosed and is not invoked while offline', () => {
  const f = fixture([{ ...en, localService: false }]); f.narrator.configure({ enabled: true }); f.host.navigator.onLine = false;
  assert.equal(f.narrator.voiceStatus('en').networkBacked, true); assert.equal(f.narrator.voiceStatus('en').offline, true);
  f.narrator.enqueue('event', 'Hello', '你好'); f.tick(); assert.equal(f.spoken.length, 0); assert.equal(f.narrator.lastResult, 'offline');
});
test('quiet and assistive-technology yield stop speech and stale callbacks', () => {
  for (const option of ['quiet', 'yieldToAssistiveTechnology']) {
    const f = fixture(); f.narrator.configure({ enabled: true, language: 'both' });
    f.narrator.enqueue('event', 'Hello', '你好'); f.tick();
    f.narrator.configure({ enabled: true, [option]: true }); f.spoken[0].onend();
    assert.equal(f.spoken.length, 1); assert.equal(f.timers.size, 0); assert.equal(f.cancelCount(), 1);
    assert.equal(f.narrator.enqueue('event', 'Again', '再一次', true), false);
  }
});
test('rate/pitch bounds and restored selections are deterministic', () => {
  assert.equal(normalize({ rate: -1 }).rate, 0.1); assert.equal(normalize({ rate: 50 }).rate, 10);
  assert.equal(normalize({ pitch: -1 }).pitch, 0); assert.equal(normalize({ pitch: 9 }).pitch, 2);
  assert.equal(normalize({ rate: NaN, pitch: Infinity }).rate, 1);
  const config = { enabled: true, rate: 0.1, pitch: 2, englishVoice: en.voiceURI, cantoneseVoice: yue.voiceURI };
  const f = fixture(); f.narrator.configure(JSON.parse(JSON.stringify(config))); f.narrator.enqueue('event', 'Hello', '你好'); f.tick();
  assert.equal(f.spoken[0].rate, 0.1); assert.equal(f.spoken[0].pitch, 2);
});
test('speech deadline releases the queue without stale completion duplication', () => {
  const f = fixture(); f.narrator.configure({ enabled: true }); f.narrator.enqueue('event', 'Hello', '你好'); f.tick(); f.tick();
  assert.equal(f.narrator.lastResult, 'speech-timeout'); assert.equal(f.cancelCount(), 1);
  f.spoken[0].onend(); assert.equal(f.narrator.lastResult, 'speech-timeout');
});
test('teardown removes late-voice and connectivity listeners', () => {
  const f = fixture(); f.narrator.dispose(); assert.equal(f.listeners.size, 0); assert.equal(f.timers.size, 0);
});
test('restoring a cached page reattaches listeners without replaying old speech', () => {
  const f = fixture(); f.narrator.configure({ enabled: true }); f.narrator.enqueue('event', 'Old', '舊');
  f.narrator.dispose(); f.narrator.resume(); assert.equal(f.listeners.size, 3); assert.equal(f.timers.size, 0);
  f.narrator.enqueue('event', 'New', '新'); f.tick(); assert.equal(f.spoken[0].text, 'New');
});
test('malformed restored settings normalize without enabling narration', () => {
  for (const value of [null, [], true, 'enabled']) assert.equal(normalize(value).enabled, false);
});
test('important event replaces ordinary debounce and precedes queued ordinary speech', () => {
  const f = fixture(); f.narrator.configure({ enabled: true });
  f.narrator.enqueue('ordinary', 'Later', '稍後'); f.narrator.enqueue('failure', 'Retry now', '立即重試', true);
  assert.equal([...f.timers.values()][0].delay, 0); f.tick(); assert.equal(f.spoken[0].text, 'Retry now');
});
test('missing Cantonese wording speaks English with an explicit fallback result', () => {
  for (const language of ['yue', 'both']) {
    const f = fixture(); f.narrator.configure({ enabled: true, language }); f.narrator.enqueue('event', 'Original wording', ''); f.tick();
    assert.equal(f.spoken.length, 1); assert.equal(f.spoken[0].lang, en.lang); f.spoken[0].onend();
    assert.equal(f.narrator.lastResult, 'spoken-english-fallback');
  }
});
test('unrelated settings updates do not cancel active narration', () => {
  const f = fixture(); f.narrator.configure({ enabled: true }); f.narrator.enqueue('event', 'Hello', '你好'); f.tick();
  f.narrator.configure({ enabled: true }); assert.equal(f.cancelCount(), 0); assert.equal(f.narrator.active, f.spoken[0]);
});
