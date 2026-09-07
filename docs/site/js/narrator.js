(function (root, factory) {
  if (typeof module !== 'undefined' && module.exports) module.exports = factory();
  else root.MaterialAudacityNarrator = factory();
})(typeof window === 'undefined' ? globalThis : window, function () {
  'use strict';
  const defaults = Object.freeze({ enabled: false, language: 'en', englishVoice: '', cantoneseVoice: '', rate: 1, pitch: 1, quiet: false, yieldToAssistiveTechnology: false });
  function normalize(value = {}) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) value = {};
    return {
      enabled: value.enabled === true,
      language: ['en', 'yue', 'both'].includes(value.language) ? value.language : 'en',
      englishVoice: typeof value.englishVoice === 'string' && value.englishVoice.length <= 1024 ? value.englishVoice : '',
      cantoneseVoice: typeof value.cantoneseVoice === 'string' && value.cantoneseVoice.length <= 1024 ? value.cantoneseVoice : '',
      rate: Number.isFinite(value.rate) ? Math.min(10, Math.max(0.1, value.rate)) : 1,
      pitch: Number.isFinite(value.pitch) ? Math.min(2, Math.max(0, value.pitch)) : 1,
      quiet: value.quiet === true,
      yieldToAssistiveTechnology: value.yieldToAssistiveTechnology === true,
    };
  }
  const matches = (voice, language) => language === 'en' ? /^en(?:-|$)/i.test(voice.lang) : /^(?:yue(?:-|$)|zh-HK$)/i.test(voice.lang);
  class Narrator {
    constructor(host, changed = () => {}) {
      this.host = host; this.changed = changed; this.options = normalize();
      this.queue = []; this.active = null; this.timer = null; this.speechTimer = null; this.cooldowns = new Map(); this.generation = 0;
      this.voices = []; this.lastResult = 'off'; this.disposed = false;
      this.onVoices = () => this.refreshVoices();
      this.onConnectivity = () => { if (!this.online()) this.stop(); this.changed(); };
      host.speechSynthesis?.addEventListener('voiceschanged', this.onVoices);
      host.addEventListener?.('online', this.onConnectivity); host.addEventListener?.('offline', this.onConnectivity);
      this.refreshVoices();
    }
    available() { return !!(this.host.speechSynthesis && this.host.SpeechSynthesisUtterance); }
    online() { return this.host.navigator?.onLine !== false; }
    refreshVoices() {
      try { this.voices = this.host.speechSynthesis?.getVoices() || []; } catch (_) { this.voices = []; }
      this.changed();
    }
    configure(value) {
      const next = normalize(value);
      if (JSON.stringify(next) === JSON.stringify(this.options)) return;
      this.stop();
      this.options = next;
      this.changed();
    }
    voiceStatus(language) {
      const choices = this.voices.filter(voice => matches(voice, language));
      const requested = this.options[language === 'en' ? 'englishVoice' : 'cantoneseVoice'];
      const selected = choices.find(voice => voice.voiceURI === requested);
      const fallback = choices.find(voice => voice.localService && voice.default) || choices.find(voice => voice.localService) || choices[0];
      const effective = selected || fallback;
      return { available: this.available(), requested, choices, effective, missingSelection: !!requested && !selected,
        networkBacked: effective ? !effective.localService : false, offline: !!effective && !effective.localService && !this.online() };
    }
    enqueue(category, english, cantonese, important = false) {
      if (this.disposed || !this.options.enabled || this.options.quiet || this.options.yieldToAssistiveTechnology || !this.available()) return false;
      if (typeof category !== 'string' || !category || category.length > 64 || typeof english !== 'string' || english.length > 2000 || typeof cantonese !== 'string' || cantonese.length > 2000) return false;
      const now = Date.now();
      if (!important && now - (this.cooldowns.get(category) || 0) < 5000) return false;
      const item = { category, english, cantonese, important };
      const existing = this.queue.findIndex(entry => entry.category === category && !entry.important);
      if (existing >= 0 && !important) this.queue[existing] = item;
      else if (this.queue.length < 32) this.queue.push(item);
      else {
        const ordinary = this.queue.findIndex(entry => !entry.important);
        if (ordinary < 0 || !important) { this.lastResult = 'queue-full'; this.changed(); return false; }
        this.queue.splice(ordinary, 1); this.queue.push(item);
      }
      if (important) {
        this.queue.splice(this.queue.indexOf(item), 1);
        const beforeOrdinary = this.queue.findIndex(entry => !entry.important);
        this.queue.splice(beforeOrdinary < 0 ? this.queue.length : beforeOrdinary, 0, item);
        if (this.timer && !this.active) { this.host.clearTimeout(this.timer); this.timer = null; }
      }
      if (!this.timer && !this.active) this.timer = this.host.setTimeout(() => { this.timer = null; this.drain(); }, important ? 0 : 250);
      return true;
    }
    drain() {
      if (this.disposed || this.active || !this.queue.length) return;
      const item = this.queue.shift();
      this.cooldowns.set(item.category, Date.now());
      if (this.cooldowns.size > 64) this.cooldowns.delete(this.cooldowns.keys().next().value);
      const fallback = this.options.language !== 'en' && !item.cantonese.trim();
      const languages = fallback ? ['en'] : this.options.language === 'both' ? ['en', 'yue'] : [this.options.language];
      const parts = languages.map(language => ({ language, text: language === 'en' ? item.english : item.cantonese })).filter(part => part.text.trim());
      const generation = this.generation;
      const next = () => {
        if (this.disposed || generation !== this.generation) return;
        const part = parts.shift();
        if (!part) { this.active = null; this.drain(); return; }
        const status = this.voiceStatus(part.language);
        if (!part.text.trim() || !status.effective || status.offline) {
          this.lastResult = status.offline ? 'offline' : 'voice-unavailable'; this.changed(); next(); return;
        }
        const utterance = new this.host.SpeechSynthesisUtterance(part.text);
        utterance.voice = status.effective; utterance.lang = status.effective.lang;
        utterance.rate = this.options.rate; utterance.pitch = this.options.pitch;
        this.active = utterance; this.lastResult = 'speaking'; this.changed();
        let settled = false;
        const finish = result => {
          if (settled || generation !== this.generation) return;
          settled = true; this.host.clearTimeout(deadline); this.speechTimer = null; this.active = null;
          this.lastResult = result; this.changed(); next();
        };
        const deadline = this.host.setTimeout(() => {
          if (generation !== this.generation) return;
          settled = true; this.speechTimer = null; this.active = null; this.host.speechSynthesis.cancel();
          this.lastResult = 'speech-timeout'; this.changed(); next();
        }, 30000);
        this.speechTimer = deadline;
        utterance.onend = () => finish(fallback ? 'spoken-english-fallback' : 'spoken');
        utterance.onerror = () => finish('speech-unavailable');
        try { this.host.speechSynthesis.speak(utterance); } catch (_) { finish('speech-unavailable'); }
      };
      next();
    }
    stop() {
      this.generation++; this.queue = [];
      if (this.timer) this.host.clearTimeout(this.timer);
      if (this.speechTimer) this.host.clearTimeout(this.speechTimer);
      this.speechTimer = null;
      this.timer = null;
      if (this.active) this.host.speechSynthesis?.cancel();
      this.active = null; this.lastResult = 'stopped';
    }
    dispose() {
      this.stop(); this.disposed = true;
      this.host.speechSynthesis?.removeEventListener('voiceschanged', this.onVoices);
      this.host.removeEventListener?.('online', this.onConnectivity); this.host.removeEventListener?.('offline', this.onConnectivity);
    }
    resume() {
      if (!this.disposed) return;
      this.disposed = false;
      this.host.speechSynthesis?.addEventListener('voiceschanged', this.onVoices);
      this.host.addEventListener?.('online', this.onConnectivity); this.host.addEventListener?.('offline', this.onConnectivity);
      this.refreshVoices();
    }
  }
  return Object.freeze({ Narrator, normalize, defaults });
});
