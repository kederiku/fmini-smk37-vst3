// Reference runs the unmodified upstream WASM in Node's WebAssembly engine.
import fs from 'node:fs';
import path from 'node:path';
const root = path.resolve(import.meta.dirname, '..');
const wasm = fs.readFileSync(path.join(root, 'third_party/fmini-1-46ff9987680b01d12522779448e8b7f07625093f/docs/emu/smk/felucca.wasm'));
const { instance } = await WebAssembly.instantiate(wasm, {});
const e = instance.exports;
console.log('Imports:', WebAssembly.Module.imports(new WebAssembly.Module(wasm)));
e._initialize(); e.web_nor_erase(); e.web_smk_wheel(0, 535); e.web_smk_wheel(1, 615);
for (let i = 0; i < 4; i++) e.web_smk_fader(i, 0);
e.web_boot();
function render(n, dst, start = 0) {
  for (let p = 0; p < n; p += 128) {
    const count = Math.min(128, n - p);
    e.web_render(count);
    if (!dst) continue;
    const l = new Float32Array(e.memory.buffer, e.web_out_l(), count);
    const r = new Float32Array(e.memory.buffer, e.web_out_r(), count);
    for (let i = 0; i < count; i++) { dst[2*(start+p+i)] = l[i]; dst[2*(start+p+i)+1] = r[i]; }
  }
}
render(136710);
const out = new Float32Array(2*88200);
e.web_midi(0x90, 60, 100); render(44100, out);
e.web_midi(0x80, 60, 0); render(44100, out, 44100);
fs.mkdirSync(path.join(root, 'build'), {recursive:true});
fs.writeFileSync(path.join(root, 'build/reference.f32'), Buffer.from(out.buffer));
const peak = out.reduce((a,v) => Math.max(a,Math.abs(v)),0);
if (!Number.isFinite(peak) || peak < 0.001) throw new Error('Reference is not audible');
console.log(`Reference: ${out.length/2} stereo frames; peak=${peak}; engine=${e.web_sample_rate()} Hz`);
