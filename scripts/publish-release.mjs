// Draft staging keeps interrupted uploads private and safe to retry.
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { createHash } from 'node:crypto';

const hash = data => createHash('sha256').update(data).digest('hex');
const buildTag = /^v\d+\.\d+\.\d+-build\.([1-9]\d*)$/;

export function makePlan({ repository, ref, sha, version, runNumber }) {
  if (ref !== 'refs/heads/main') throw new Error('Releases are only allowed from main');
  if (!/^[\w.-]+\/[\w.-]+$/.test(repository ?? '')) throw new Error('Invalid repository');
  if (!/^[a-f0-9]{40}$/.test(sha ?? '')) throw new Error('Invalid commit SHA');
  if (!/^\d+\.\d+\.\d+$/.test(version ?? '')) throw new Error('Invalid VERSION');
  if (!/^[1-9]\d*$/.test(runNumber ?? '')) throw new Error('Invalid workflow run number');
  return { repository, ref, sha, version, runNumber, tag: `v${version}-build.${runNumber}` };
}

export function loadAssets(directory, version) {
  const prefix = `FMini-SMK-37-${version}`;
  const names = [`${prefix}-Windows-x64.zip`, `${prefix}-sources.zip`];
  const manifestName = `${prefix}-SHA256SUMS.txt`;
  const expected = [...names, manifestName].sort();
  if (JSON.stringify(fs.readdirSync(directory).sort()) !== JSON.stringify(expected)) {
    throw new Error('Release files must contain exactly the public binary, sources and checksum manifest');
  }
  const manifest = fs.readFileSync(path.join(directory, manifestName), 'utf8').replace(/^\uFEFF/, '').trim();
  const entries = new Map();
  for (const line of manifest.split(/\r?\n/)) {
    const match = /^([a-f0-9]{64})  (.+)$/i.exec(line);
    if (!match || !names.includes(match[2]) || entries.has(match[2])) throw new Error('Invalid checksum manifest');
    entries.set(match[2], match[1].toLowerCase());
  }
  if (entries.size !== names.length) throw new Error('Incomplete checksum manifest');
  const assets = names.map((name, index) => {
    const data = fs.readFileSync(path.join(directory, name));
    if (hash(data) !== entries.get(name)) throw new Error(`Checksum mismatch: ${name}`);
    // Stable names keep /releases/latest/download links valid across versions.
    return { name: index === 0 ? 'FMini-SMK-37-Windows-x64.zip' : 'FMini-SMK-37-sources.zip', data };
  });
  assets.push({ name: 'SHA256SUMS.txt', data: Buffer.from(
    assets.map(asset => `${hash(asset.data)}  ${asset.name}\n`).join(''), 'utf8') });
  return assets;
}

export function createGitHubApi(token, fetchRequest = fetch) {
  if (!token) throw new Error('GITHUB_TOKEN is required');
  return async (method, route, body, { allowMissing = false } = {}) => {
    const url = new URL(route, 'https://api.github.com');
    if (url.protocol !== 'https:' || !['api.github.com', 'uploads.github.com'].includes(url.hostname)) {
      throw new Error('Unexpected GitHub API host');
    }
    const binary = Buffer.isBuffer(body);
    const response = await fetchRequest(url, {
      method,
      headers: {
        Authorization: `Bearer ${token}`,
        Accept: 'application/vnd.github+json',
        'X-GitHub-Api-Version': '2022-11-28',
        'User-Agent': 'fmini-smk37-release',
        ...(body === undefined ? {} : { 'Content-Type': binary ? 'application/octet-stream' : 'application/json' }),
      },
      body: body === undefined ? undefined : binary ? body : JSON.stringify(body),
      signal: AbortSignal.timeout(60_000),
    });
    if (response.status === 404 && method === 'GET' && allowMissing) return null;
    if (!response.ok) throw new Error(`GitHub API ${method} ${url.pathname}: HTTP ${response.status}`);
    return response.status === 204 ? null : response.json();
  };
}

export async function publishRelease(plan, assets, api) {
  const base = `/repos/${plan.repository}`;
  const lookup = route => api('GET', base + route, undefined, { allowMissing: true });
  let release = await lookup(`/releases/tags/${plan.tag}`);
  // The by-tag endpoint only finds published releases. Authenticated listing
  // also includes pending drafts, which may be beyond the first page on a retry.
  for (let page = 1; !release; page++) {
    const releases = await api('GET', `${base}/releases?per_page=100&page=${page}`);
    release = releases.find(item => item.draft && item.tag_name === plan.tag);
    if (releases.length < 100) break;
  }
  // Ref lookup returns 404 for a missing tag; commit lookup returns 422 instead.
  const tagRef = await lookup(`/git/ref/tags/${plan.tag}`);
  const tagCommit = tagRef ? await api('GET', `${base}/commits/${plan.tag}`) : null;
  if ((tagCommit && tagCommit.sha !== plan.sha) || (release && release.target_commitish !== plan.sha)) {
    throw new Error('Release tag or draft points to a different commit');
  }
  if (release && !release.draft) {
    if (!tagCommit) throw new Error('Published release has no commit tag');
    // Never replace assets that users have already downloaded.
    return release.html_url;
  }

  const latest = await lookup('/releases/latest');
  const latestBuild = buildTag.exec(latest?.tag_name ?? '');
  // The publish job is serialized. Older builds/reruns cannot move Latest back.
  const makeLatest = !latestBuild || BigInt(plan.runNumber) > BigInt(latestBuild[1]);
  if (!release) {
    const download = `https://github.com/${plan.repository}/releases/download/${plan.tag}`;
    release = await api('POST', base + '/releases', {
      tag_name: plan.tag,
      target_commitish: plan.sha,
      name: `FMini SMK-37 ${plan.version} — build ${plan.runNumber}`,
      draft: true,
      prerelease: false,
      body: [
        `**[Download Windows x64 / Télécharger pour Windows x64](${download}/FMini-SMK-37-Windows-x64.zip)**`,
        '',
        'Automatically built, tested and published from main. Extract the ZIP and copy the entire `VST3/FMini SMK-37.vst3` bundle into your VST3 folder.',
        '',
        'Compilation, tests et publication automatiques depuis main. Extraire le ZIP et copier le dossier complet `VST3/FMini SMK-37.vst3` dans votre dossier VST3.',
        '',
        'Complete corresponding sources and GPL-3.0-only license included. SAVIHost is downloaded separately.',
        'Sources correspondantes complètes et licence GPL-3.0-only incluses. SAVIHost se télécharge séparément.',
        '',
        `Commit: [${plan.sha}](https://github.com/${plan.repository}/commit/${plan.sha})`,
      ].join('\n'),
    });
  }

  const names = assets.map(asset => asset.name);
  if (release.assets.some(asset => !names.includes(asset.name))) throw new Error('Unexpected draft asset');
  for (const asset of release.assets) await api('DELETE', `${base}/releases/assets/${asset.id}`);
  for (const asset of assets) {
    await api('POST', `https://uploads.github.com${base}/releases/${release.id}/assets?name=${encodeURIComponent(asset.name)}`, asset.data);
  }
  const uploaded = await api('GET', `${base}/releases/${release.id}`);
  if (uploaded.assets.length !== assets.length) throw new Error('Incomplete uploaded release');
  for (const asset of assets) {
    const remote = uploaded.assets.find(item => item.name === asset.name);
    if (!remote || remote.state !== 'uploaded' || remote.size !== asset.data.length ||
        (remote.digest && remote.digest !== `sha256:${hash(asset.data)}`)) {
      throw new Error(`Uploaded asset verification failed: ${asset.name}`);
    }
  }
  const published = await api('PATCH', `${base}/releases/${release.id}`, {
    draft: false, prerelease: false, make_latest: String(makeLatest),
  });
  if (published.draft) throw new Error('Release is still a draft');
  return published.html_url;
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) {
  const plan = makePlan({ repository: process.env.GITHUB_REPOSITORY, ref: process.env.GITHUB_REF,
    sha: process.env.GITHUB_SHA, version: process.env.RELEASE_VERSION, runNumber: process.env.GITHUB_RUN_NUMBER });
  const assets = loadAssets(process.argv[2] ?? 'release-assets', plan.version);
  const url = await publishRelease(plan, assets, createGitHubApi(process.env.GITHUB_TOKEN));
  console.log(`Published release: ${url}`);
  if (process.env.GITHUB_STEP_SUMMARY) {
    fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY,
      `Published [${plan.tag}](${url}) from commit \`${plan.sha}\`.\n\n` +
      `[Download Windows x64](https://github.com/${plan.repository}/releases/latest/download/FMini-SMK-37-Windows-x64.zip)\n`);
  }
}
