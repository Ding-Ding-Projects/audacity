const { test } = require('node:test');
const assert = require('node:assert/strict');
const { parse, createReplacer, limits } = require('../js/personal-vocabulary.js');
const encode = entries => JSON.stringify({ schemaVersion: 1, entries });

test('accepts the versioned object schema with immutable safe dictionaries', () => {
  const record = parse(encode({ Home: 'Start' }));
  assert.equal(record.entries.Home, 'Start');
  assert.equal(Object.getPrototypeOf(record.entries), null);
  assert.ok(Object.isFrozen(record.entries));
});

for (const [name, input] of Object.entries({
  'unsupported version': '{"schemaVersion":2,"entries":{}}',
  'missing version': '{"entries":{}}',
  'unexpected field': '{"schemaVersion":1,"entries":{},"extra":true}',
  'legacy array': '{"version":1,"entries":[]}',
  'nested replacement': '{"schemaVersion":1,"entries":{"Home":{}}}',
  'duplicate root': '{"schemaVersion":1,"schemaVersion":1,"entries":{}}',
  'duplicate entry': '{"schemaVersion":1,"entries":{"Home":"A","Home":"B"}}',
  'unsafe entry': '{"schemaVersion":1,"entries":{"__proto__":"A"}}',
  'numeric replacement': '{"schemaVersion":1,"entries":{"Home":42}}',
  'trailing input': '{"schemaVersion":1,"entries":{}}true',
  'empty key': encode({ '': 'A' }),
  'long key': encode({ ['x'.repeat(limits.key + 1)]: 'A' }),
  'long replacement': encode({ Home: 'x'.repeat(limits.value + 1) }),
  'control replacement': encode({ Home: String.fromCharCode(0) }),
  'unpaired surrogate': encode({ Home: String.fromCharCode(0xd800) }),
  'UTF-8 byte bound': ' '.repeat(limits.bytes + 1),
  'entry count bound': encode(Object.fromEntries(Array.from({ length: limits.entries + 1 }, (_, i) => ['k' + i, 'x']))),
})) test('rejects ' + name + ' without reflecting input', () => {
  assert.throws(() => parse(input), { message: 'Invalid personal vocabulary file.' });
});

test('accepts the exact entry count limit', () => {
  const record = parse(encode(Object.fromEntries(Array.from({ length: limits.entries }, (_, i) => ['k' + i, 'x']))));
  assert.equal(Object.keys(record.entries).length, limits.entries);
});

test('decoded duplicate keys cannot evade detection', () => {
  const escapedHome = String.fromCharCode(92) + 'u0048ome';
  assert.throws(() => parse('{"schemaVersion":1,"entries":{"Home":"A","' + escapedHome + '":"B"}}'));
});

test('replacement is longest-first, word-bounded, and never cascades', () => {
  const replace = createReplacer(parse(encode({ Audio: 'Sound', 'Audio editor': 'Studio', Studio: 'Second' })));
  assert.equal(replace('Audio editor. Audio. Audiobook. Studio.'), 'Studio. Sound. Audiobook. Second.');
});

test('HTML-looking replacements remain literal strings for text-node rendering', () => {
  const literal = '<img src="invalid" onerror="throw 1">';
  assert.equal(createReplacer(parse(encode({ Home: literal })))('Home'), literal);
});

test('replacement expansion is bounded and falls back to original text atomically', () => {
  const text = Array(100).fill('A').join(' ');
  assert.equal(createReplacer(parse(encode({ A: 'x'.repeat(1000) })))(text), text);
});
