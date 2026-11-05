// Packs the web app into dist/ as <path>.gz files for the device's LittleFS image.
// No bundler: the page uses ES modules, so src/*.js is shipped as-is.
import { gzipSync } from 'node:zlib';
import { mkdirSync, readFileSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';

const pdfjsBuild = 'node_modules/pdfjs-dist/legacy/build'; // legacy: older iOS Safari
const files = {
  'index.html': 'index.html',
  'app.js': 'app.js',
  'style.css': 'style.css',
  'vendor/pdf.mjs': `${pdfjsBuild}/pdf.min.mjs`,
  'vendor/pdf.worker.mjs': `${pdfjsBuild}/pdf.worker.min.mjs`,
  ...Object.fromEntries(readdirSync('src').map((f) => [`src/${f}`, `src/${f}`])),
};

rmSync('dist', { recursive: true, force: true });
let total = 0;
for (const [published, source] of Object.entries(files)) {
  const gz = gzipSync(readFileSync(source), { level: 9 });
  const out = join('dist', `${published}.gz`);
  mkdirSync(dirname(out), { recursive: true });
  writeFileSync(out, gz);
  total += gz.length;
  console.log(`${String(gz.length).padStart(8)}  ${out}`);
}
console.log(`${String(total).padStart(8)}  total (gzip bytes)`);
