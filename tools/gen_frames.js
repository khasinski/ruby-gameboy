#!/usr/bin/env node
// Generate pre-rendered gem frames as C tile data for Game Boy Color

const R1 = 24, R2 = 40;
const tableY = -18, girdleY = 2, culetY = 38;
const angleX = 0.0;
const CX = 80, CY = 72;
const W = 160, H = 144;
const FRAMES = 7;

// Build vertices
const verts = [];
for (let i = 0; i < 8; i++) {
  const a = i * Math.PI / 4;
  verts.push([R1 * Math.cos(a), tableY, R1 * Math.sin(a)]);
}
for (let i = 0; i < 8; i++) {
  const a = i * Math.PI / 4;
  verts.push([R2 * Math.cos(a), girdleY, R2 * Math.sin(a)]);
}
verts.push([0, culetY, 0]);

// Build faces
const faces = [];
const faceType = []; // 0=table, 1=crown, 2=pavilion
for (let i = 1; i < 7; i++) { faces.push([0, i, i + 1]); faceType.push(0); }
for (let i = 0; i < 8; i++) {
  const t1 = i, t2 = (i + 1) % 8, g1 = 8 + i, g2 = 8 + (i + 1) % 8;
  faces.push([t1, g1, t2]); faceType.push(1);
  faces.push([t2, g1, g2]); faceType.push(1);
}
for (let i = 0; i < 8; i++) {
  const g1 = 8 + i, g2 = 8 + (i + 1) % 8;
  faces.push([g1, 16, g2]); faceType.push(2);
}

// Rasterize a triangle into the pixel buffer
function fillTri(pixels, pts, color) {
  let [x0,y0] = pts[0], [x1,y1] = pts[1], [x2,y2] = pts[2];
  // Sort by Y
  if (y0 > y1) { [x0,y0,x1,y1] = [x1,y1,x0,y0]; }
  if (y0 > y2) { [x0,y0,x2,y2] = [x2,y2,x0,y0]; }
  if (y1 > y2) { [x1,y1,x2,y2] = [x2,y2,x1,y1]; }

  if (y0 === y2) return;

  for (let py = Math.max(0, Math.floor(y0)); py <= Math.min(H-1, Math.floor(y2)); py++) {
    const dy02 = y2 - y0, dy01 = y1 - y0, dy12 = y2 - y1;
    const longX = x0 + (x2 - x0) * (py - y0) / dy02;
    let shortX;
    if (py < y1) {
      shortX = dy01 === 0 ? x0 : x0 + (x1 - x0) * (py - y0) / dy01;
    } else {
      shortX = dy12 === 0 ? x1 : x1 + (x2 - x1) * (py - y1) / dy12;
    }
    let lx = Math.floor(Math.min(longX, shortX));
    let rx = Math.floor(Math.max(longX, shortX));
    lx = Math.max(0, lx);
    rx = Math.min(W - 1, rx);
    for (let px = lx; px <= rx; px++) {
      pixels[py * W + px] = color;
    }
  }
}

function renderFrame(angleY) {
  const pixels = new Uint8Array(W * H); // 0=bg, 1=bright, 2=medium, 3=dark
  const cy = Math.cos(angleY), sy = Math.sin(angleY);
  const cx = Math.cos(angleX), sx = Math.sin(angleX);

  const projected = verts.map(([x, y, z]) => {
    const rx = x * cy - z * sy;
    const rz = x * sy + z * cy;
    const ry = y * cx - rz * sx;
    const rz2 = y * sx + rz * cx;
    const scale = 200 / (200 + rz2);
    return [CX + rx * scale * 1.5, CY + ry * scale * 1.5, rz2];
  });

  // Sort faces back-to-front
  const sorted = faces.map((f, i) => ({ f, i })).sort((a, b) => {
    const az = a.f.reduce((s, v) => s + projected[v][2], 0);
    const bz = b.f.reduce((s, v) => s + projected[v][2], 0);
    return bz - az;
  });

  for (const { f, i } of sorted) {
    const pts = f.map(v => projected[v]);
    const [ax, ay] = pts[0], [bx, by] = pts[1], [cx2, cy2] = pts[2];
    const nz = (bx - ax) * (cy2 - ay) - (by - ay) * (cx2 - ax);
    if (nz >= 0) continue;

    const avgX = pts.reduce((s, p) => s + p[0], 0) / 3;
    const isTable = i < 6;
    // Bake shading into pixel color (3 shades)
    let color;
    if (isTable) color = 2;
    else if (avgX > 90) color = 1;      // bright
    else if (avgX > 70) color = 2;      // medium
    else color = 3;                     // dark

    fillTri(pixels, pts.map(p => [Math.round(p[0]), Math.round(p[1])]), color);
  }
  return pixels;
}

// Convert pixel buffer to tiles
function pixelsToTiles(pixels) {
  const tiles = [];
  const tileMap = [];

  for (let ty = 0; ty < 18; ty++) {
    for (let tx = 0; tx < 20; tx++) {
      const tileData = [];
      for (let row = 0; row < 8; row++) {
        let lo = 0, hi = 0;
        for (let col = 0; col < 8; col++) {
          const px = pixels[(ty * 8 + row) * W + tx * 8 + col];
          const bit = 0x80 >> col;
          if (px & 1) lo |= bit;
          if (px & 2) hi |= bit;
        }
        tileData.push(lo, hi);
      }
      tiles.push(tileData);
      tileMap.push(tiles.length - 1);

    }
  }
  return { tiles, tileMap };
}

// Deduplicate tiles across all frames
function dedup(allFrames) {
  const uniqueTiles = [];
  const tileIndex = new Map();
  const frameMaps = [];
  for (const frame of allFrames) {
    const map = [];
    for (let i = 0; i < frame.tiles.length; i++) {
      const key = frame.tiles[i].join(',');
      if (!tileIndex.has(key)) {
        tileIndex.set(key, uniqueTiles.length);
        uniqueTiles.push(frame.tiles[i]);
      }
      map.push(tileIndex.get(key));
    }
    frameMaps.push(map);
  }
  return { uniqueTiles, frameMaps };
}

// Generate frames
const allFrames = [];
for (let f = 0; f < FRAMES; f++) {
  const angle = f * (Math.PI / 4) / FRAMES;
  const pixels = renderFrame(angle);
  allFrames.push(pixelsToTiles(pixels));
}

const { uniqueTiles, frameMaps } = dedup(allFrames);

console.error(`Unique tiles: ${uniqueTiles.length}`);
console.error(`Frames: ${FRAMES}, tiles per frame: 360`);
console.error(`Tile data: ${uniqueTiles.length * 16} bytes`);
console.error(`Map data: ${FRAMES * 360} bytes`);
console.error(`Total: ${uniqueTiles.length * 16 + FRAMES * 360} bytes`);

if (uniqueTiles.length > 256) {
  console.error('WARNING: Too many unique tiles (>256)!');
}

// Output C header
let out = '// Auto-generated gem frame data\n';
out += `#define GEM_FRAME_COUNT ${FRAMES}\n`;
out += `#define GEM_TILE_COUNT ${uniqueTiles.length}\n\n`;

out += 'static const uint8_t gem_tile_data[] = {\n';
for (let i = 0; i < uniqueTiles.length; i++) {
  out += '    ' + uniqueTiles[i].map(b => '0x' + b.toString(16).padStart(2, '0')).join(',') + ',\n';
}
out += '};\n\n';

for (let f = 0; f < FRAMES; f++) {
  out += `static const uint8_t gem_frame_${f}[] = {\n`;
  for (let row = 0; row < 18; row++) {
    out += '    ' + frameMaps[f].slice(row * 20, row * 20 + 20).join(',') + ',\n';
  }
  out += '};\n\n';
}

out += 'static const uint8_t* const gem_frames[] = {\n';
for (let f = 0; f < FRAMES; f++) {
  out += `    gem_frame_${f},\n`;
}
out += '};\n\n';


process.stdout.write(out);

// Optional: generate PNG preview (requires node-canvas)
try { const { createCanvas } = require('canvas');
const pngCanvas = createCanvas(W * FRAMES, H);
const pctx = pngCanvas.getContext('2d');
const colors = ['#ddd8cc', '#ee4444', '#cc2222', '#882020'];

for (let f = 0; f < FRAMES; f++) {
  const angle = f * (Math.PI / 4) / FRAMES;
  const pixels = renderFrame(angle);
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      pctx.fillStyle = colors[pixels[y * W + x]];
      pctx.fillRect(f * W + x, y, 1, 1);
    }
  }
}

const fs2 = require('fs');
const buf = pngCanvas.toBuffer('image/png');
fs2.writeFileSync('tools/gem_frames_preview.png', buf);
console.error('Wrote tools/gem_frames_preview.png');
} catch(e) {}
