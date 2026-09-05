/*
 * a11y-check.js — basic accessibility sanity checks run against the live
 * page with Playwright + Chromium: missing alt text, missing form labels,
 * and low-contrast text (computed from getComputedStyle).
 *
 * Usage: PLAYWRIGHT_BROWSERS_PATH=/opt/pw-browsers node a11y-check.js
 * (playwright-core must be installed in this script's directory or reachable
 * via NODE_PATH; run `npm install playwright-core` in a scratch dir first.)
 */
const { chromium } = require('playwright-core');

const BASE = process.env.MA_BASE_URL || 'http://localhost:8791/index.html';
const ROUTES = ['home', 'features', 'downloads', 'changelog', 'docs', 'accessibility', 'privacy', 'settings'];

function relLuminance([r, g, b]) {
  const f = (c) => { c /= 255; return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4); };
  return 0.2126 * f(r) + 0.7152 * f(g) + 0.0722 * f(b);
}
function contrastRatio(a, b) {
  const la = relLuminance(a) + 0.05, lb = relLuminance(b) + 0.05;
  return la > lb ? la / lb : lb / la;
}
function parseRgb(str) {
  const m = str.match(/rgba?\(([\d.]+),\s*([\d.]+),\s*([\d.]+)/);
  return m ? [parseFloat(m[1]), parseFloat(m[2]), parseFloat(m[3])] : null;
}

(async () => {
  const browser = await chromium.launch({ executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome', args: ['--no-sandbox', '--headless=new'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
  const problems = [];

  for (const route of ROUTES) {
    await page.goto(BASE + '#' + route, { waitUntil: 'load' });
    await page.waitForTimeout(300);

    const imgIssues = await page.$$eval('img', (imgs) => imgs.filter((i) => !i.hasAttribute('alt')).map((i) => i.src));
    imgIssues.forEach((src) => problems.push(route + ': <img> missing alt: ' + src));

    const inputIssues = await page.evaluate(() => {
      const bad = [];
      document.querySelectorAll('input, select, textarea').forEach((el) => {
        const hasLabel = el.labels && el.labels.length > 0;
        const hasAria = el.getAttribute('aria-label') || el.getAttribute('aria-labelledby');
        if (!hasLabel && !hasAria) bad.push(el.tagName + '#' + (el.id || '(no id)'));
      });
      return bad;
    });
    inputIssues.forEach((s) => problems.push(route + ': form control without label: ' + s));

    const contrastIssues = await page.evaluate(() => {
      const bad = [];
      const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_ELEMENT);
      let node;
      while ((node = walker.nextNode())) {
        if (!node.textContent || !node.textContent.trim()) continue;
        const hasDirectText = Array.from(node.childNodes).some((n) => n.nodeType === 3 && n.textContent.trim());
        if (!hasDirectText) continue;
        const cs = getComputedStyle(node);
        if (cs.display === 'none' || cs.visibility === 'hidden') continue;
        bad.push({ tag: node.tagName, id: node.id, color: cs.color, bg: cs.backgroundColor, fontSize: cs.fontSize });
      }
      return bad;
    });
    // Resolve effective background by walking up if transparent, done in-page:
    const resolved = await page.evaluate(() => {
      const out = [];
      const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_ELEMENT);
      let node;
      while ((node = walker.nextNode())) {
        if (!node.textContent || !node.textContent.trim()) continue;
        const hasDirectText = Array.from(node.childNodes).some((n) => n.nodeType === 3 && n.textContent.trim());
        if (!hasDirectText) continue;
        let bgNode = node;
        let bg = getComputedStyle(bgNode).backgroundColor;
        while (bg === 'rgba(0, 0, 0, 0)' && bgNode.parentElement) { bgNode = bgNode.parentElement; bg = getComputedStyle(bgNode).backgroundColor; }
        out.push({ tag: node.tagName, id: node.id, color: getComputedStyle(node).color, bg, fontSize: parseFloat(getComputedStyle(node).fontSize) });
      }
      return out;
    });
    resolved.forEach((item) => {
      const fg = parseRgb(item.color);
      const bg = parseRgb(item.bg);
      if (!fg || !bg) return;
      const ratio = contrastRatio(fg, bg);
      const threshold = item.fontSize >= 18 ? 3.0 : 4.5;
      if (ratio < threshold) {
        problems.push(route + ': low contrast on <' + item.tag.toLowerCase() + (item.id ? '#' + item.id : '') + '> ratio=' + ratio.toFixed(2) + ' (needs ' + threshold + ')');
      }
    });
  }

  await browser.close();
  if (problems.length) {
    console.log('ACCESSIBILITY ISSUES FOUND: ' + problems.length);
    problems.forEach((p) => console.log(' - ' + p));
    process.exitCode = 1;
  } else {
    console.log('No accessibility issues found by this basic check.');
  }
})();
