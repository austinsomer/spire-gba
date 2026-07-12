#!/usr/bin/env python3
"""Generate GBA-legal sprites with Gemini image gen ("nano banana 2").

  export GEMINI_API_KEY=...        # from aistudio.google.com/apikey
  python3 tools/gensprite.py ironclad "battle-worn barbarian, red cape, iron armor, facing right"
  python3 tools/gensprite.py cultist  "hooded cultist, dark robe" --colors 12
  python3 tools/gensprite.py ironclad --reprocess     # re-run post-proc on saved raw, no API call
  python3 tools/gensprite.py ironclad "..." --raw-only # just fetch art, skip GBA conversion

Flow: prompt -> Gemini image -> key out flat bg -> crop to subject ->
downscale -> quantize to <=COLORS -> snap to GBA 5-bit -> enforce budget ->
write assets/sprites/<name>.png (32x32, alpha, <=15 colors). Then:
  python3 tools/assets.py import && make clean && make && make run

Other groups: --kind cardart|mapicons|icons|tiles set the target size.
Raw model output is kept in assets/_raw/<name>.png for re-tuning.
"""
import sys, os, json, base64, argparse, ssl, urllib.request, urllib.error
from PIL import Image

try:
    import certifi
    SSLCTX = ssl.create_default_context(cafile=certifi.where())
except ImportError:
    SSLCTX = ssl.create_default_context()

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, 'assets')
RAW = os.path.join(ASSETS, '_raw')

# nano banana 2 (Nano Banana Pro). Override with --model or GEMINI_IMAGE_MODEL.
# Cheaper fallbacks in cost order. ALL image models require a *billed* project.
DEFAULT_MODEL = os.environ.get('GEMINI_IMAGE_MODEL', 'gemini-3-pro-image')
FALLBACK_MODELS = ['gemini-3.1-flash-image', 'gemini-3.1-flash-lite-image',
                   'gemini-2.5-flash-image']
API = 'https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent'

# flat key color the model is told to paint the background — easy to remove.
KEY = (255, 0, 255)

KINDS = {  # name -> (dir, size, default color budget)
    'sprites':  ('sprites',  32, 15),
    'cardart':  ('cardart',  16, 14),
    'mapicons': ('mapicons', 16, 14),
    'icons':    ('icons',     8, 14),
    'tiles':    ('tiles',     8, 12),
}

def snap(c):            # 8-bit -> GBA 5-bit -> display 8-bit (multiple of 8)
    return (c >> 3) * 8

# ---------- generation ----------

def build_prompt(user_prompt, size):
    return (
        f"A single video-game sprite: {user_prompt}. "
        f"Pixel-art style, bold clean shapes, high contrast, dark outline around "
        f"the whole figure, readable when tiny (~{size}px). One centered character, "
        f"full body in frame, no cropping, no text, no shadow on the ground. "
        f"Fill the ENTIRE background with pure solid magenta rgb(255,0,255) and "
        f"nothing else — no gradient, no scenery, no border."
    )

def gen_image(prompt, model, api_key):
    body = json.dumps({
        "contents": [{"parts": [{"text": prompt}]}],
        "generationConfig": {"responseModalities": ["IMAGE"]},
    }).encode()
    url = API.format(model=model)
    req = urllib.request.Request(
        url, data=body,
        headers={"Content-Type": "application/json", "x-goog-api-key": api_key})
    with urllib.request.urlopen(req, timeout=120, context=SSLCTX) as r:
        data = json.load(r)
    for cand in data.get("candidates", []):
        for part in cand.get("content", {}).get("parts", []):
            inl = part.get("inlineData") or part.get("inline_data")
            if inl and inl.get("data"):
                return base64.b64decode(inl["data"])
    raise RuntimeError("no image in response: " + json.dumps(data)[:500])

def fetch(prompt, size, model, api_key):
    full = build_prompt(prompt, size)
    tried = [model] + [m for m in FALLBACK_MODELS if m != model]
    last = None
    for m in tried:
        try:
            print(f"  gen via {m} ...")
            return gen_image(full, m, api_key)
        except urllib.error.HTTPError as e:
            msg = e.read().decode()[:300]
            print(f"  {m} failed: HTTP {e.code} {msg}")
            last = e
        except Exception as e:
            print(f"  {m} failed: {e}")
            last = e
    raise SystemExit(f"all models failed. last: {last}")

# ---------- post-processing ----------

def key_out_bg(im):
    """Magenta -> transparent; also flood transparent from edges for stray bg."""
    im = im.convert('RGBA')
    px = im.load()
    w, h = im.size
    def is_key(r, g, b):
        return r > 180 and b > 180 and g < 90
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if is_key(r, g, b):
                px[x, y] = (0, 0, 0, 0)
    return im

def crop_square(im):
    bbox = im.getbbox()          # bbox of non-zero-alpha
    if bbox:
        im = im.crop(bbox)
    w, h = im.size
    s = max(w, h)
    pad = int(s * 0.10)          # small margin so outline isn't flush to edge
    s += 2 * pad
    canvas = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    canvas.paste(im, ((s - w) // 2, (s - h) // 2), im)
    return canvas

def downscale(im, size):
    return im.resize((size, size), Image.LANCZOS)

def harden_alpha(im):
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            px[x, y] = (r, g, b, 255 if a >= 128 else 0)
    return im

def quantize_snap(im, max_colors):
    """Quantize opaque pixels to <=max_colors GBA-snapped colors.
    Retries with fewer target colors until snapped unique count fits."""
    im = harden_alpha(im)
    px = im.load()
    w, h = im.size
    target = max_colors
    while target >= 2:
        rgb = Image.new('RGB', (w, h), KEY)
        rp = rgb.load()
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a:
                    rp[x, y] = (r, g, b)
        q = rgb.quantize(colors=target, method=Image.MEDIANCUT, dither=Image.NONE)
        q = q.convert('RGB')
        qp = q.load()
        out = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        op = out.load()
        seen = set()
        for y in range(h):
            for x in range(w):
                if px[x, y][3]:
                    r, g, b = qp[x, y]
                    c = (snap(r), snap(g), snap(b))
                    seen.add(c)
                    op[x, y] = (c[0], c[1], c[2], 255)
        if len(seen) <= max_colors:
            return out, len(seen)
        target -= 1
    raise SystemExit("could not fit color budget — try --colors lower or simpler art")

def process(raw_bytes_or_im, size, max_colors):
    im = (raw_bytes_or_im if isinstance(raw_bytes_or_im, Image.Image)
          else Image.open(_io(raw_bytes_or_im)))
    im = key_out_bg(im)
    im = crop_square(im)
    im = downscale(im, size)
    return quantize_snap(im, max_colors)

def _io(b):
    import io
    return io.BytesIO(b)

# ---------- main ----------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name')
    ap.add_argument('prompt', nargs='?', default=None)
    ap.add_argument('--kind', choices=KINDS, default='sprites')
    ap.add_argument('--colors', type=int, default=None)
    ap.add_argument('--model', default=DEFAULT_MODEL)
    ap.add_argument('--reprocess', action='store_true',
                    help='skip API, re-run post-proc on saved assets/_raw/<name>.png')
    ap.add_argument('--raw-only', action='store_true',
                    help='fetch art only, skip GBA conversion')
    args = ap.parse_args()

    subdir, size, defcolors = KINDS[args.kind]
    colors = args.colors or defcolors
    os.makedirs(RAW, exist_ok=True)
    os.makedirs(os.path.join(ASSETS, subdir), exist_ok=True)
    raw_path = os.path.join(RAW, f'{args.name}.png')
    out_path = os.path.join(ASSETS, subdir, f'{args.name}.png')

    if args.reprocess:
        if not os.path.exists(raw_path):
            raise SystemExit(f'no raw at {raw_path} — generate first')
        im = Image.open(raw_path)
    else:
        if not args.prompt:
            raise SystemExit('prompt required (or use --reprocess)')
        key = os.environ.get('GEMINI_API_KEY')
        if not key:
            raise SystemExit('set GEMINI_API_KEY (aistudio.google.com/apikey)')
        raw = fetch(args.prompt, size, args.model, key)
        with open(raw_path, 'wb') as f:
            f.write(raw)
        print(f'  raw -> {raw_path}')
        if args.raw_only:
            return
        im = Image.open(raw_path)

    out, n = process(im, size, colors)
    out.save(out_path)
    print(f'{args.name}: {size}x{size}, {n} colors -> {out_path}')
    print(f'  next: python3 tools/assets.py import && make clean && make && make run')

if __name__ == '__main__':
    main()
