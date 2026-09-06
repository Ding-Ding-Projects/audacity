import { execFileSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, writeFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';

// Stage the public documentation root and record provenance from Git and GitHub.
// Release records are retrieved by gh, never inferred from filenames or clocks.
const root = resolve(fileURLToPath(new URL('../../..', import.meta.url)));
const output = resolve(process.argv[2] || join(root, 'build.site'));
if (!output.startsWith(root + '/') && !output.startsWith(root + '\\')) {
  throw new Error('The staging output must be a new directory inside this checkout.');
}
if (existsSync(output)) throw new Error('Staging output already exists; choose a fresh output directory.');
const git = (...args) => execFileSync('git', args, { cwd: root, encoding: 'utf8' }).trim();
const commit = git('rev-parse', 'HEAD');
const sourceUpdatedAt = git('show', '-s', '--format=%cI', 'HEAD');
if (!/^[a-f0-9]{40}$/.test(commit) || !Number.isFinite(Date.parse(sourceUpdatedAt))) {
  throw new Error('Git source provenance is unavailable.');
}
const repository = 'Ding-Ding-Projects/audacity';
const releases = JSON.parse(execFileSync('gh', ['api', `repos/${repository}/releases`, '--paginate', '--slurp'], { cwd: root, encoding: 'utf8', maxBuffer: 8 * 1024 * 1024 })).flat();
const release = releases.filter(r => !r.draft && /^v\d+\.\d+\.\d+-m3\.\d+$/.test(r.tag_name))
  .sort((a, b) => Date.parse(b.published_at) - Date.parse(a.published_at))[0];
if (!release || !Number.isFinite(Date.parse(release.published_at))) throw new Error('No published release with valid provenance is available.');
const prefix = `https://github.com/${repository}/releases/download/`;
const assets = release.assets.filter(a => a.size > 0 && a.browser_download_url.startsWith(prefix)
  && (/\.exe$|\.nupkg$|^RELEASES$|^SHA256SUMS/i.test(a.name)))
  .map(a => ({ name: a.name, url: a.browser_download_url, size: a.size,
    sha256: /^sha256:[a-f0-9]{64}$/.test(a.digest || '') ? a.digest.slice(7) : null }));
if (!assets.some(a => /Setup\.exe$/i.test(a.name)) || !assets.some(a => a.name === 'RELEASES')
  || !assets.some(a => /-full\.nupkg$/i.test(a.name))) throw new Error('The release does not contain the required Windows installer and update assets.');
mkdirSync(output, { recursive: true });
cpSync(join(root, 'docs/site'), output, { recursive: true });
writeFileSync(join(output, '.nojekyll'), '');
writeFileSync(join(output, 'data/provenance.json'), JSON.stringify({
  schemaVersion: 1, version: `source-${commit.slice(0, 12)}`, commit,
  updatedAt: sourceUpdatedAt, provenanceKind: 'source-commit',
}, null, 2) + '\n');
writeFileSync(join(output, 'data/release.json'), JSON.stringify({
  tag: release.tag_name, name: release.name, updatedAt: release.published_at,
  url: release.html_url, prerelease: release.prerelease,
  notes: `Published ${release.tag_name}. See the linked release for its exact build and verification evidence.`,
  unsigned: true,
  unsignedReason: 'These Squirrel.Windows installers are unsigned and may trigger an unknown-publisher or SmartScreen warning. Published hashes provide an integrity comparison, not signature verification.',
  assets,
}, null, 2) + '\n');
console.log(JSON.stringify({ output, commit, version: `source-${commit.slice(0, 12)}`, release: release.tag_name, assets: assets.length }));
