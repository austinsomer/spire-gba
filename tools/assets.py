#!/usr/bin/env python3
"""Two-way PNG asset pipeline for the spire-gba art.

  python3 tools/assets.py export   # dump current art as PNGs into assets/
  python3 tools/assets.py import   # pull edited PNGs back + regenerate

Groups (see ASSETS.md for full requirements):
  assets/sprites/<name>.png    32x32  characters/monsters -> SPR_<NAME>
  assets/cardart/art_<n>.png   16x16  card illustrations  (bank 13)
  assets/mapicons/m_<n>.png    16x16  map node icons      (bank 13)
  assets/icons/<n>.png          8x8   UI icons            (bank 11)
  assets/tiles/<n>.png          8x8   scenery tiles       (their bank)
  assets/bg/battle.png        240x160 battle backdrop (auto-quantized)
  assets/bg/title.png         240x160 title art       (auto-quantized)

Transparency: alpha < 128. Colors are snapped to GBA 5-bit (multiples
of 8 in 0..248). Pixel groups are exact-color: >15 unique colors in one
sprite (or overflowing a shared bank) is an error, not a quantize.
"""
import sys, os, re
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, 'tools', 'art.txt')
BGART = os.path.join(ROOT, 'tools', 'bgart.txt')
ASSETS = os.path.join(ROOT, 'assets')

CHARSET = "abcdefghijmnpqstuvwzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

def snap(c):  # 8-bit -> GBA 5-bit -> back to display 8-bit
    return (c >> 3) * 8

def rgb31(c): return c >> 3

# ---------- text-file parsing ----------

def parse_blocks(path):
    """returns (lines, blocks) where blocks = {name: (start, end, header)}
    for @-blocks; bank palettes = {bank: [(char, r, g, b)]} (0..31 scale)."""
    lines = open(path).read().split('\n')
    blocks, banks = {}, {}
    cur_bank = None
    i = 0
    start = None; name = None
    for i, l in enumerate(lines):
        if l.startswith('!bank'):
            cur_bank = int(l.split()[1]); banks[cur_bank] = []
        elif l.startswith('=') and cur_bank is not None and name is None:
            m = re.match(r'=(\S)\s+(\d+)\s+(\d+)\s+(\d+)', l)
            banks[cur_bank].append((m.group(1), int(m.group(2)),
                                    int(m.group(3)), int(m.group(4))))
        elif l.startswith('@'):
            if name: blocks[name] = (start, i, lines[start])
            start = i; name = l[1:].split()[0]
        elif not l.strip() and name:
            blocks[name] = (start, i, lines[start])
            name = None
    if name: blocks[name] = (start, len(lines), lines[start])
    return lines, blocks, banks

def block_rows(lines, se):
    rows = []
    for l in lines[se[0] + 1:se[1]]:
        if l.startswith('=') or not l.strip() or l.lstrip().startswith('#'):
            continue
        rows.append(l)
    return rows

def block_palette(lines, se):
    pal = []
    for l in lines[se[0] + 1:se[1]]:
        m = re.match(r'=(\S)\s+(\d+)\s+(\d+)\s+(\d+)', l)
        if m: pal.append((m.group(1), int(m.group(2)), int(m.group(3)),
                          int(m.group(4))))
    return pal

# ---------- export ----------

def rows_to_png(rows, cmap, path, size):
    im = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    px = im.load()
    for y, r in enumerate(rows):
        for x, ch in enumerate(r.ljust(size, '.')):
            if ch != '.' and ch in cmap:
                cr, cg, cb = cmap[ch]
                px[x, y] = (cr * 8 + cr // 4, cg * 8 + cg // 4,
                            cb * 8 + cb // 4, 255)
    im.save(path)

def do_export():
    for d in ('sprites', 'cardart', 'mapicons', 'icons', 'tiles', 'bg'):
        os.makedirs(os.path.join(ASSETS, d), exist_ok=True)
    # sprites (art.txt: per-block palettes)
    lines, blocks, _ = parse_blocks(ART)
    for name, se in blocks.items():
        pal = {ch: (r, g, b) for ch, r, g, b in block_palette(lines, se)}
        rows_to_png(block_rows(lines, se), pal,
                    os.path.join(ASSETS, 'sprites', f'{name}.png'), 32)
    # bgart groups (bank palettes)
    lines, blocks, banks = parse_blocks(BGART)
    for name, se in blocks.items():
        head = lines[se[0]]
        bank = int(head.split()[1])
        pal = {ch: (r, g, b) for ch, r, g, b in banks[bank]}
        rows = block_rows(lines, se)
        if name.startswith('icon:'):
            rows_to_png(rows, pal,
                        os.path.join(ASSETS, 'icons', f'{name[5:]}.png'), 8)
        elif name.startswith('big:'):
            n = name[4:]
            grp = 'cardart' if n.startswith('art_') else 'mapicons'
            rows_to_png(rows, pal, os.path.join(ASSETS, grp, f'{n}.png'), 16)
        else:
            rows_to_png(rows, pal,
                        os.path.join(ASSETS, 'tiles', f'{name}.png'), 8)
    # bg sources
    for src, dst in (('battlebg_src.png', 'bg/battle.png'),
                     ('title_src.png', 'bg/title.png')):
        p = os.path.join(ROOT, 'tools', src)
        if os.path.exists(p):
            Image.open(p).save(os.path.join(ASSETS, dst))
    print('exported to', ASSETS)

# ---------- import ----------

def png_grid(path, size):
    im = Image.open(path).convert('RGBA')
    if im.size != (size, size):
        sys.exit(f'{path}: must be {size}x{size}, got {im.size[0]}x{im.size[1]}')
    px = im.load()
    grid, colors = [], []
    for y in range(size):
        row = []
        for x in range(size):
            r, g, b, a = px[x, y]
            if a < 128:
                row.append(None)
            else:
                c = (rgb31(r), rgb31(g), rgb31(b))
                if c not in colors: colors.append(c)
                row.append(c)
        grid.append(row)
    return grid, colors

def grid_to_rows(grid, cmap):
    return [''.join('.' if c is None else cmap[c] for c in row)
            for row in grid]

def splice(lines, blocks, name, newlines):
    if name in blocks:
        s, e, _ = blocks[name]
        return lines[:s] + newlines + lines[e:]
    return lines + [''] + newlines

def import_sprites():
    d = os.path.join(ASSETS, 'sprites')
    if not os.path.isdir(d): return
    for f in sorted(os.listdir(d)):
        if not f.endswith('.png'): continue
        name = f[:-4]
        grid, colors = png_grid(os.path.join(d, f), 32)
        if len(colors) > 15:
            sys.exit(f'{f}: {len(colors)} colors (max 15)')
        cmap = {c: CHARSET[i] for i, c in enumerate(colors)}
        lines, blocks, _ = parse_blocks(ART)
        nl = [f'@{name}'] + [f'={cmap[c]} {c[0]} {c[1]} {c[2]}' for c in colors]
        nl += grid_to_rows(grid, cmap)
        open(ART, 'w').write('\n'.join(splice(lines, blocks, name, nl)))
        print(f'sprite {name}: {len(colors)} colors')

def import_bank_group(dirname, size, bank, prefix):
    d = os.path.join(ASSETS, dirname)
    if not os.path.isdir(d): return
    for f in sorted(os.listdir(d)):
        if not f.endswith('.png'): continue
        name = f[:-4]
        grid, colors = png_grid(os.path.join(d, f), size)
        lines, blocks, banks = parse_blocks(BGART)
        pal = banks[bank]
        by_rgb = {(r, g, b): ch for ch, r, g, b in pal}
        used = {ch for ch, *_ in pal}
        # place new colors into free bank slots
        add = []
        for c in colors:
            if c not in by_rgb:
                if len(pal) + len(add) >= 15:
                    sys.exit(f'{f}: bank {bank} palette overflow '
                             f'({len(pal)} existing + new colors > 15). '
                             f'Reuse the documented palette (ASSETS.md).')
                ch = next(x for x in CHARSET if x not in used)
                used.add(ch); add.append((ch, c))
                by_rgb[c] = ch
        # append new palette lines right after the bank's last = line
        if add:
            bank_line = next(i for i, l in enumerate(lines)
                             if l.startswith(f'!bank {bank}'))
            ins = bank_line + 1
            while ins < len(lines) and lines[ins].startswith('='):
                ins += 1
            for ch, c in add:
                lines.insert(ins, f'={ch} {c[0]} {c[1]} {c[2]}')
                ins += 1
            lines2, blocks, banks = parse_blocks_from(lines)
        else:
            blocks = parse_blocks_lines(lines)
        cmap = {c: by_rgb[c] for c in colors}
        blkname = prefix + name
        nl = [f'@{blkname} {bank}'] + grid_to_rows(grid, cmap)
        lines = splice(lines, blocks, blkname, nl)
        open(BGART, 'w').write('\n'.join(lines))
        print(f'{dirname}/{name}: ok (bank {bank})')

def parse_blocks_lines(lines):
    """blocks only, from a lines list"""
    blocks = {}
    start = None; name = None
    for i, l in enumerate(lines):
        if l.startswith('@'):
            if name: blocks[name] = (start, i, lines[start])
            start = i; name = l[1:].split()[0]
        elif not l.strip() and name:
            blocks[name] = (start, i, lines[start])
            name = None
    if name: blocks[name] = (start, len(lines), lines[start])
    return blocks

def parse_blocks_from(lines):
    return lines, parse_blocks_lines(lines), None

def do_import():
    import_sprites()
    import_bank_group('icons', 8, 11, 'icon:')
    import_bank_group('cardart', 16, 13, 'big:')
    import_bank_group('mapicons', 16, 13, 'big:')
    # scenery tiles: keep each tile in its existing bank
    d = os.path.join(ASSETS, 'tiles')
    if os.path.isdir(d):
        for f in sorted(os.listdir(d)):
            if not f.endswith('.png'): continue
            name = f[:-4]
            lines, blocks, banks = parse_blocks(BGART)
            bank = int(blocks[name][2].split()[1]) if name in blocks else 12
            import_bank_group_single(f, name, bank)
    # backgrounds
    bb = os.path.join(ASSETS, 'bg', 'battle.png')
    if os.path.exists(bb):
        os.system(f'cp "{bb}" "{ROOT}/tools/battlebg_src.png" && '
                  f'python3 "{ROOT}/tools/mkbattlebg.py" '
                  f'"{ROOT}/tools/battlebg_src.png" "{ROOT}/src/battlebg.h"')
    tt = os.path.join(ASSETS, 'bg', 'title.png')
    if os.path.exists(tt):
        os.system(f'cp "{tt}" "{ROOT}/tools/title_src.png" && '
                  f'python3 "{ROOT}/tools/mkimage.py" '
                  f'"{ROOT}/tools/title_src.png" "{ROOT}/src/title_img.h" "NEW GAME"')
    # regenerate + validate
    r1 = os.system(f'python3 "{ROOT}/tools/mksprites.py" "{ART}" "{ROOT}/src/sprites.h"')
    r2 = os.system(f'python3 "{ROOT}/tools/mkbg.py" "{BGART}" "{ROOT}/src/bgtiles.h"')
    if r1 or r2: sys.exit('generator failed — see above')
    print('import complete. Run: make clean && make')

def import_bank_group_single(f, name, bank):
    # single scenery tile via the same path as groups
    d = os.path.join(ASSETS, 'tiles')
    grid, colors = png_grid(os.path.join(d, f), 8)
    lines, blocks, banks = parse_blocks(BGART)
    pal = banks[bank]
    by_rgb = {(r, g, b): ch for ch, r, g, b in pal}
    for c in colors:
        if c not in by_rgb:
            sys.exit(f'{f}: color {c} not in bank {bank} palette — scenery '
                     f'tiles must reuse their bank colors (ASSETS.md)')
    cmap = {c: by_rgb[c] for c in colors}
    nl = [f'@{name} {bank}'] + grid_to_rows(grid, cmap)
    lines = splice(lines, blocks, name, nl)
    open(BGART, 'w').write('\n'.join(lines))
    print(f'tiles/{name}: ok (bank {bank})')

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('export', 'import'):
        sys.exit(__doc__)
    (do_export if sys.argv[1] == 'export' else do_import)()
