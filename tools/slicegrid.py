#!/usr/bin/env python3
"""Slice a generated sprite-sheet grid (items on a flat gray/solid bg) into
tight-cropped cell PNGs.

    python3 tools/slicegrid.py <sheet.png> <outdir> [--cols N]

Auto-detects row bands and column runs from the background mask. If cells
touch horizontally (no bg gap), pass --cols N to split each row band into N
cells at minimum-content seams instead. Crops are named r<row>c<col>.png.

Used for the card sheets (asset-grids/) and the battle UI sheets. Sheets
often contain duplicate/extra cells — slice everything, eyeball the crops,
pick variants by hand. Downscale/quantize is per-asset-type (see
tools/mkcards.py for the k-means approach).
"""
import os, sys
import numpy as np
from PIL import Image


def runs(flags, min_len):
    out, s = [], None
    for i, v in enumerate(flags):
        if v and s is None:
            s = i
        if not v and s is not None:
            if i - s > min_len:
                out.append((s, i))
            s = None
    if s is not None:
        out.append((s, len(flags)))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    cols = None
    for a in sys.argv[1:]:
        if a.startswith('--cols'):
            cols = int(a.split('=')[1] if '=' in a else sys.argv[sys.argv.index(a) + 1])
    sheet, outdir = args[0], args[1]
    os.makedirs(outdir, exist_ok=True)

    im = Image.open(sheet).convert('RGB')
    a = np.asarray(im).astype(int)
    bg = a[2, 2]
    mask = np.abs(a - bg).sum(axis=2) > 60

    for bi, (y0, y1) in enumerate(runs(mask.sum(axis=1) > mask.shape[1] * 0.02, 50)):
        m = mask[y0:y1]
        colsum = m.sum(axis=0)
        if cols:  # fixed count, split at min-content seams (touching cells)
            xs_any = np.where(m.any(axis=0))[0]
            x0, x1 = int(xs_any[0]), int(xs_any[-1] + 1)
            step = (x1 - x0) / cols
            cuts = [x0]
            for k in range(1, cols):
                nom = int(x0 + step * k)
                lo = max(nom - 40, 0)
                cuts.append(lo + int(np.argmin(colsum[lo:nom + 40])))
            cuts.append(x1)
            spans = list(zip(cuts, cuts[1:]))
        else:
            spans = runs(colsum > (y1 - y0) * 0.02, 50)
        for ci, (cx0, cx1) in enumerate(spans):
            sub = m[:, cx0:cx1]
            ys = np.where(sub.any(axis=1))[0]
            xs = np.where(sub.any(axis=0))[0]
            crop = im.crop((cx0 + int(xs[0]), y0 + int(ys[0]),
                            cx0 + int(xs[-1]) + 1, y0 + int(ys[-1]) + 1))
            fn = f'{outdir}/r{bi}c{ci}.png'
            crop.save(fn)
            print(f'{fn}: {crop.size}')


if __name__ == '__main__':
    main()
