// Exercise publication failures and retries without contacting GitHub.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { makePlan, loadAssets, publishRelease, createGitHubApi } from '../scripts/publish-release.mjs';

const sha = 'a'.repeat(40);
const plan = (runNumber = '12') => makePlan({
  repository: 'owner/synth', ref: 'refs/heads/main', sha, version: '0.1.0', runNumber,
});
const digest = data => `sha256:${createHash('sha256').update(data).digest('hex')}`;
const assets = ['FMini-SMK-37-Windows-x64.zip', 'FMini-SMK-37-sources.zip', 'SHA256SUMS.txt']
  .map(name => ({ name, data: Buffer.from(name) }));

function fixture(t) {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'fmini-release-'));
  t.after(() => fs.rmSync(dir, { recursive: true, force: true }));
  const names = ['FMini-SMK-37-0.1.0-Windows-x64.zip', 'FMini-SMK-37-0.1.0-sources.zip'];
  for (const name of names) fs.writeFileSync(path.join(dir, name), name);
  fs.writeFileSync(path.join(dir, 'FMini-SMK-37-0.1.0-SHA256SUMS.txt'),
    '\uFEFF' + names.map(name => `${digest(Buffer.from(name)).slice(7).toUpperCase()}  ${name}`).join('\r\n') + '\r\n');
  return { dir, names };
}

// The fake models remote draft state, including an upload that fails halfway.
function github({ latest = null, release = null, tagSha = null, failUpload = false, pagesBeforeDraft = 0 } = {}) {
  const state = { latest, release, tagSha, failUpload, pagesBeforeDraft, calls: [] };
  let nextId = 10;
  state.api = async (method, route, body) => {
    state.calls.push({ method, route, body });
    if (method === 'GET' && route.endsWith('/releases/latest')) return state.latest;
    if (method === 'GET' && route.includes('/releases/tags/')) return state.release?.draft ? null : state.release;
    if (method === 'GET' && route.includes('/releases?')) {
      const page = Number(new URL(route, 'https://api.github.com').searchParams.get('page'));
      if (page <= state.pagesBeforeDraft) return Array.from({ length: 100 }, (_, i) => ({ tag_name: `other-${page}-${i}`, draft: false }));
      return state.release ? [state.release] : [];
    }
    if (method === 'GET' && route.includes('/git/ref/tags/')) return state.tagSha ? { ref: route } : null;
    if (method === 'GET' && route.includes('/commits/')) {
      if (!state.tagSha) throw new Error('GitHub API: HTTP 422 (unknown commit)');
      return { sha: state.tagSha };
    }
    if (method === 'POST' && route.endsWith('/releases')) {
      if (state.release) throw new Error('Duplicate release creation');
      state.release = { ...body, id: 1, assets: [], html_url: 'https://github.com/owner/synth/releases/tag/' + body.tag_name };
      return state.release;
    }
    if (method === 'DELETE' && route.includes('/releases/assets/')) {
      state.release.assets = state.release.assets.filter(a => a.id !== Number(route.split('/').at(-1)));
      return null;
    }
    if (method === 'POST' && route.includes('uploads.github.com/')) {
      if (state.failUpload && state.release.assets.length === 1) throw new Error('Upload failed');
      const asset = { id: nextId++, name: new URL(route).searchParams.get('name'),
        size: body.length, state: 'uploaded', digest: digest(body) };
      state.release.assets.push(asset);
      return asset;
    }
    if (method === 'GET' && route.endsWith('/releases/1')) return state.release;
    if (method === 'PATCH' && route.endsWith('/releases/1')) {
      Object.assign(state.release, body);
      if (!body.draft) state.tagSha = state.release.target_commitish;
      return state.release;
    }
    throw new Error(`Unexpected request: ${method} ${route}`);
  };
  return state;
}

test('each main run has its own tag; retries retain that tag', () => {
  assert.equal(plan().tag, 'v0.1.0-build.12');
  assert.equal(plan().tag, plan().tag);
  assert.notEqual(plan('13').tag, plan().tag);
  assert.throws(() => makePlan({ ...plan(), ref: 'refs/heads/feature' }), /main/);
});

test('packaging verifies both archives and creates stable download names and checksums', t => {
  const { dir } = fixture(t);
  const result = loadAssets(dir, '0.1.0');
  assert.deepEqual(result.map(a => a.name), assets.map(a => a.name));
  const manifest = result[2].data.toString();
  for (const asset of result.slice(0, 2)) assert(manifest.includes(`${digest(asset.data).slice(7)}  ${asset.name}`));
});

test('corrupted binary, missing source and private/extra assets are rejected', t => {
  const { dir, names } = fixture(t);
  fs.writeFileSync(path.join(dir, names[0]), 'corrupt');
  assert.throws(() => loadAssets(dir, '0.1.0'), /checksum/i);
  fs.writeFileSync(path.join(dir, names[0]), names[0]);
  fs.unlinkSync(path.join(dir, names[1]));
  assert.throws(() => loadAssets(dir, '0.1.0'), /files/i);
  fs.writeFileSync(path.join(dir, names[1]), names[1]);
  fs.writeFileSync(path.join(dir, 'SAVIHost.exe'), 'private');
  assert.throws(() => loadAssets(dir, '0.1.0'), /files/i);
});

test('incomplete or duplicate checksum manifests are rejected', t => {
  const { dir, names } = fixture(t);
  const line = `${digest(Buffer.from(names[0])).slice(7)}  ${names[0]}\n`;
  const manifest = path.join(dir, 'FMini-SMK-37-0.1.0-SHA256SUMS.txt');
  fs.writeFileSync(manifest, line);
  assert.throws(() => loadAssets(dir, '0.1.0'), /manifest/i);
  fs.writeFileSync(manifest, line + line);
  assert.throws(() => loadAssets(dir, '0.1.0'), /manifest/i);
});

test('a release targets the tested SHA and becomes public only after all uploads', async () => {
  const remote = github();
  await publishRelease(plan(), assets, remote.api);
  assert.equal(remote.release.target_commitish, sha);
  assert.equal(remote.release.draft, false);
  assert.equal(remote.release.prerelease, false);
  assert.equal(remote.release.make_latest, 'true');
  assert.deepEqual(remote.release.assets.map(a => a.name), assets.map(a => a.name));
  const writes = remote.calls.filter(c => c.method !== 'GET');
  assert.equal(writes[0].method, 'POST');
  assert.equal(writes.at(-1).method, 'PATCH');
});

test('an interrupted upload stays private and the next attempt completes it', async () => {
  const remote = github({ failUpload: true });
  await assert.rejects(publishRelease(plan(), assets, remote.api), /Upload failed/);
  assert.equal(remote.release.draft, true);
  assert.equal(remote.release.assets.length, 1);
  remote.failUpload = false;
  await publishRelease(plan(), assets, remote.api);
  assert.equal(remote.release.draft, false);
  assert.equal(remote.release.assets.length, 3);
});

test('rerunning a published release performs no writes', async () => {
  const remote = github();
  await publishRelease(plan(), assets, remote.api);
  remote.calls = [];
  await publishRelease(plan(), assets, remote.api);
  assert(remote.calls.every(c => c.method === 'GET'));
});

test('a pending draft can be resumed even beyond the first release-list page', async () => {
  const remote = github({ failUpload: true });
  await assert.rejects(publishRelease(plan(), assets, remote.api), /Upload failed/);
  remote.failUpload = false;
  remote.pagesBeforeDraft = 1;
  await publishRelease(plan(), assets, remote.api);
  assert.equal(remote.release.draft, false);
  assert.equal(remote.calls.filter(c => c.method === 'POST' && c.route.endsWith('/releases')).length, 1);
  assert(remote.calls.some(c => c.route.endsWith('per_page=100&page=2')));
});

test('a conflicting tag or draft commit is rejected before uploads', async () => {
  const wrongTag = github({ tagSha: 'b'.repeat(40) });
  await assert.rejects(publishRelease(plan(), assets, wrongTag.api), /commit/i);
  const wrongDraft = github({ release: { tag_name: plan().tag, draft: true, target_commitish: 'b'.repeat(40) } });
  await assert.rejects(publishRelease(plan(), assets, wrongDraft.api), /commit/i);
  assert([...wrongTag.calls, ...wrongDraft.calls].every(c => c.method === 'GET'));
});

test('an older completed build does not replace a newer Latest release', async () => {
  const remote = github({ latest: { tag_name: 'v0.1.0-build.13' } });
  await publishRelease(plan('12'), assets, remote.api);
  assert.equal(remote.release.draft, false);
  assert.equal(remote.release.make_latest, 'false');
});

test('a new base version can become Latest; build numbers remain monotonic', async () => {
  const remote = github({ latest: { tag_name: 'v0.1.0-build.11' } });
  await publishRelease(makePlan({ ...plan(), version: '0.2.0' }), assets, remote.api);
  assert.equal(remote.release.make_latest, 'true');
});

test('HTTP 404 is optional only for explicit lookups; server errors stop publishing', async () => {
  const missing = createGitHubApi('test-token', async () => new Response('{}', { status: 404 }));
  assert.equal(await missing('GET', '/missing', undefined, { allowMissing: true }), null);
  await assert.rejects(missing('POST', '/missing', {}), /404/);
  const unavailable = createGitHubApi('test-token', async () => new Response('{}', { status: 503 }));
  await assert.rejects(unavailable('GET', '/releases/latest', undefined, { allowMissing: true }), /503/);
});
