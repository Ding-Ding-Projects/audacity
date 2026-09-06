import { execFileSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, writeFileSync, realpathSync, lstatSync } from 'node:fs';
import { resolve, join, dirname, basename } from 'node:path';
import { fileURLToPath } from 'node:url';

// Stage the public documentation root and record provenance from Git and GitHub.
// Release records are retrieved by gh, never inferred from filenames or clocks.
const root = resolve(fileURLToPath(new URL('../../..', import.meta.url)));
const output = resolve(process.argv[2] || join(root, 'build.site'));
if (dirname(output) !== root || !/^build\.site(?:-[a-zA-Z0-9-]+)?$/.test(basename(output))
    || realpathSync(root).toLowerCase() !== root.toLowerCase()) {
  throw new Error('Staging must use a new build.site directory directly inside the real checkout root.');
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
const candidates = releases.filter(r => !r.draft && /^v\d+\.\d+\.\d+-m3\.\d+$/.test(r.tag_name))
  .sort((a, b) => Date.parse(b.published_at) - Date.parse(a.published_at));
let release;
let releaseCommit;
for (const candidate of candidates) {
  const resolvedCommit = execFileSync('gh', ['api', `repos/${repository}/commits/${encodeURIComponent(candidate.tag_name)}`, '--jq', '.sha'], { cwd: root, encoding: 'utf8' }).trim();
  if (!/^[a-f0-9]{40}$/.test(resolvedCommit)) throw new Error('Release tag resolution returned invalid provenance.');
  if (process.env.MA_REQUIRE_MATCHING_RELEASE === '1' && resolvedCommit !== commit) continue;
  release = candidate;
  releaseCommit = resolvedCommit;
  break;
}
if (!release || !Number.isFinite(Date.parse(release.published_at))) throw new Error('No published release with valid provenance is available.');
const prefix = `https://github.com/${repository}/releases/download/${release.tag_name}/`;
const [, major, minor, patch, serial] = /^v(\d+)\.(\d+)\.(\d+)-m3\.(\d+)$/.exec(release.tag_name);
const packageVersion = `${major}.${minor}.${patch}-m3${serial.padStart(3, '0')}`;
const assets = release.assets.filter(a => a.size > 0 && a.browser_download_url.startsWith(prefix)
  && (['Setup.exe', 'RELEASES', 'SHA256SUMS'].includes(a.name)
    || a.name === `Audacity-${packageVersion}-full.nupkg`
    || a.name === `Audacity-${packageVersion}-delta.nupkg`))
  .map(a => ({ name: a.name, url: a.browser_download_url, size: a.size,
    sha256: /^sha256:[a-f0-9]{64}$/.test(a.digest || '') ? a.digest.slice(7) : null }));
if (new Set(assets.map(a => a.name)).size !== assets.length
    || !['Setup.exe', 'RELEASES', 'SHA256SUMS', `Audacity-${packageVersion}-full.nupkg`].every(name => assets.some(a => a.name === name))) {
  throw new Error('The release does not contain unique required Windows installer, hash and update assets.');
}
mkdirSync(output, { recursive: true });
cpSync(join(root, 'docs/site'), output, { recursive: true, filter: source => {
  if (lstatSync(source).isSymbolicLink()) throw new Error('Documentation source must not contain symbolic links.');
  // Developer tools and compiled interpreter caches are not public web assets.
  if (source === join(root, 'docs/site/scripts')
      || ['__pycache__', 'fetch-fonts.sh', 'tests.html'].includes(basename(source))
      || /\.py[co]$/i.test(source)) return false;
  return true;
} });
writeFileSync(join(output, '.nojekyll'), '');
writeFileSync(join(output, 'data/provenance.json'), JSON.stringify({
  schemaVersion: 1, version: `source-${commit.slice(0, 12)}`, commit,
  updatedAt: sourceUpdatedAt, provenanceKind: 'source-commit',
}, null, 2) + '\n');
writeFileSync(join(output, 'data/release.json'), JSON.stringify({
  tag: release.tag_name, name: release.name, updatedAt: release.published_at,
  url: release.html_url, prerelease: release.prerelease, commit: releaseCommit,
  notes: `${release.prerelease ? 'Prerelease' : 'Release'} ${release.tag_name}. See the linked release for its exact build and verification evidence.`,
  unsigned: true,
  unsignedReason: 'These Squirrel.Windows installers are unsigned and may trigger an unknown-publisher or SmartScreen warning. Published hashes provide an integrity comparison, not signature verification.',
  assets,
}, null, 2) + '\n');
console.log(JSON.stringify({ output, commit, version: `source-${commit.slice(0, 12)}`, release: release.tag_name, assets: assets.length }));
