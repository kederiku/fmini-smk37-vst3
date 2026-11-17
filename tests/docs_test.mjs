// Check repository-owned Markdown links without requiring network access.
// Ignore generated/downloaded sources and private build notes deliberately.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const root = path.resolve(import.meta.dirname, '..');
const read = relative => fs.readFileSync(path.join(root, relative), 'utf8');
const files = fs.readdirSync(root).filter(name => name.endsWith('.md'));
for (const folder of ['docs/en', 'docs/fr']) {
  for (const name of fs.readdirSync(path.join(root, folder))) if (name.endsWith('.md')) files.push(`${folder}/${name}`);
}
files.push('docs/verification.md', 'docs/verification.en.md');
for (const file of files) {
  const markdown = read(file).replace(/```[\s\S]*?```/g, '');
  for (const match of markdown.matchAll(/\[[^\]]*\]\(([^)]+)\)/g)) {
    const target = match[1].replace(/^<|>$/g, '');
    if (/^(https?:|mailto:|#)/i.test(target)) continue;
    const pathname = decodeURIComponent(target.split('#')[0]);
    assert(fs.existsSync(path.resolve(root, path.dirname(file), pathname)), `${file}: broken link ${target}`);
  }
  assert(!/[A-Z]:[\\/]Users[\\/]/.test(markdown), `${file}: personal absolute path`);
}
for (const file of ['.github/workflows/ci.yml', '.github/workflows/release.yml']) {
  for (const match of read(file).matchAll(/uses:\s+([^\s#]+)/g)) {
    if (match[1].startsWith('./')) continue;
    assert(/^[\w-]+\/[\w-]+@[a-f0-9]{40}$/.test(match[1]), `Action must use a full SHA: ${match[1]}`);
  }
}
assert(/^\d+\.\d+\.\d+$/.test(read('VERSION').trim()), 'Invalid VERSION');
console.log(`PASS ${files.length} documentation files: local links, portable paths, action pins and version`);
