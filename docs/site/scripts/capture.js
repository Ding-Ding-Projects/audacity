const { chromium } = require('playwright-core');
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const OUT_DIR = '/home/user/audacity/docs/site/captures';
const BASE = 'http://localhost:8791/index.html';
const COMMIT = execSync('git -C /home/user/audacity rev-parse HEAD').toString().trim();

fs.mkdirSync(OUT_DIR, { recursive: true });

const viewports = { desktop: { width: 1280, height: 800 }, mobile: { width: 390, height: 844 } };

async function shot(browser, { route, theme, viewport, name, action }) {
  const context = await browser.newContext({
    viewport: viewports[viewport],
    colorScheme: theme,
  });
  const page = await context.newPage();
  await page.goto(BASE + '#' + route, { waitUntil: 'load' });
  await page.waitForTimeout(300);
  if (action) await action(page);
  await page.waitForTimeout(200);
  const file = path.join(OUT_DIR, name + '.png');
  await page.screenshot({ path: file });
  await context.close();
  return { file: name + '.png', route, theme, viewport: [viewports[viewport].width, viewports[viewport].height], commit: COMMIT };
}

(async () => {
  const browser = await chromium.launch({
    executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome',
    args: ['--no-sandbox', '--disable-gpu', '--headless=new'],
  });
  const manifest = [];
  const jobs = [
    { route: 'home', theme: 'light', viewport: 'desktop', name: 'home-light-desktop' },
    { route: 'home', theme: 'dark', viewport: 'desktop', name: 'home-dark-desktop' },
    { route: 'home', theme: 'light', viewport: 'mobile', name: 'home-light-mobile' },
    { route: 'home', theme: 'dark', viewport: 'mobile', name: 'home-dark-mobile' },
    { route: 'features', theme: 'light', viewport: 'desktop', name: 'features-light-desktop' },
    { route: 'features', theme: 'dark', viewport: 'desktop', name: 'features-dark-desktop' },
    { route: 'features', theme: 'light', viewport: 'mobile', name: 'features-light-mobile' },
    { route: 'settings', theme: 'light', viewport: 'desktop', name: 'settings-light-desktop' },
    { route: 'settings', theme: 'dark', viewport: 'desktop', name: 'settings-dark-desktop' },
    { route: 'settings', theme: 'light', viewport: 'mobile', name: 'settings-light-mobile' },
    { route: 'docs', theme: 'light', viewport: 'desktop', name: 'docs-tabs-light-desktop' },
    { route: 'docs', theme: 'dark', viewport: 'desktop', name: 'docs-tabs-dark-desktop' },
    { route: 'docs', theme: 'light', viewport: 'mobile', name: 'docs-tabs-light-mobile' },
    {
      route: 'home', theme: 'light', viewport: 'desktop', name: 'palette-open-light-desktop',
      action: async (page) => { await page.keyboard.down('Control'); await page.keyboard.down('Shift'); await page.keyboard.press('F'); await page.keyboard.up('Shift'); await page.keyboard.up('Control'); },
    },
    {
      route: 'home', theme: 'dark', viewport: 'desktop', name: 'palette-open-dark-desktop',
      action: async (page) => { await page.keyboard.down('Control'); await page.keyboard.down('Shift'); await page.keyboard.press('F'); await page.keyboard.up('Shift'); await page.keyboard.up('Control'); },
    },
    {
      route: 'settings', theme: 'light', viewport: 'desktop', name: 'regex-builder-open-light-desktop',
      action: async (page) => { await page.click('#settings-search-input + .rgx-anchor .rgx-toggle-btn'); },
    },
    {
      route: 'settings', theme: 'dark', viewport: 'desktop', name: 'regex-builder-open-dark-desktop',
      action: async (page) => { await page.click('#settings-search-input + .rgx-anchor .rgx-toggle-btn'); },
    },
  ];
  for (const job of jobs) {
    try {
      const entry = await shot(browser, job);
      manifest.push(entry);
      console.log('OK', job.name);
    } catch (e) {
      console.error('FAIL', job.name, e.message);
    }
  }
  fs.writeFileSync(path.join(OUT_DIR, 'captures.json'), JSON.stringify(manifest, null, 2));
  await browser.close();
})();
