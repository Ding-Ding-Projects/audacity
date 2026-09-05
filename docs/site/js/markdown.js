/* markdown.js: small hand-written markdown-to-HTML renderer (subset: headings,
   paragraphs, lists, code fences, inline code, bold/italic, links, blockquotes). */
(function (global) {
  'use strict';
  function escapeHtml(s) {
    return s.replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));
  }
  function inline(s) {
    s = escapeHtml(s);
    s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
    s = s.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    s = s.replace(/\*([^*]+)\*/g, '<em>$1</em>');
    s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>');
    return s;
  }
  function render(md) {
    const lines = md.replace(/\r\n/g, '\n').split('\n');
    let html = '';
    let i = 0;
    let inList = false;
    const headings = [];
    while (i < lines.length) {
      const line = lines[i];
      if (line.startsWith('```')) {
        const lang = line.slice(3).trim();
        const buf = [];
        i++;
        while (i < lines.length && !lines[i].startsWith('```')) { buf.push(lines[i]); i++; }
        html += '<pre><code class="lang-' + escapeHtml(lang) + '">' + escapeHtml(buf.join('\n')) + '</code></pre>';
        i++; continue;
      }
      const h = line.match(/^(#{1,6})\s+(.*)$/);
      if (h) {
        if (inList) { html += '</ul>'; inList = false; }
        const level = h[1].length;
        const text = h[2];
        const id = text.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
        headings.push({ level, text, id });
        html += '<h' + level + ' id="' + id + '">' + inline(text) + '</h' + level + '>';
        i++; continue;
      }
      if (/^[-*]\s+/.test(line)) {
        if (!inList) { html += '<ul>'; inList = true; }
        html += '<li>' + inline(line.replace(/^[-*]\s+/, '')) + '</li>';
        i++; continue;
      }
      if (inList) { html += '</ul>'; inList = false; }
      if (line.startsWith('>')) {
        html += '<blockquote>' + inline(line.replace(/^>\s?/, '')) + '</blockquote>';
        i++; continue;
      }
      if (line.trim() === '') { i++; continue; }
      html += '<p>' + inline(line) + '</p>';
      i++;
    }
    if (inList) html += '</ul>';
    return { html, headings };
  }
  global.MiniMarkdown = { render };
})(window);
