# Art asset pipeline

You own the art. Edit PNGs in `assets/`, then run:

```
python3 tools/assets.py import
make clean && make && make run
```

To (re)dump the current in-game art as editable PNG starting points:

```
python3 tools/assets.py export
```

## Global rules

- **Transparent pixels**: alpha 0 (anything under 50% alpha counts as transparent). Transparent = the layer behind shows through.
- **Colors are exact** — no quantizing for pixel art. Use RGB values that are multiples of 8 (GBA is 5-bit/channel; the importer snaps, so 250 and 248 become the same color).
- **File names are lowercase**, they become code identifiers (`ironclad.png` → `SPR_IRONCLAD`).
- Keep a **dark outline** on sprites/icons — everything sits on dark or textured backgrounds.
- After `import`, the tool validates sizes/colors and regenerates; it errors with the file name if something doesn't fit.

## The assets

### 1. Character sprites — `assets/sprites/<name>.png`
- **32x32**, transparent background, **max 15 unique colors each** (palettes are packed automatically — no shared-palette worries).
- Existing set you can repaint: `ironclad` (player), `cultist`, `jawworm`, `louse_r`, `louse_g`, `acidslime`, `spikeslime`, `looter`, `fungi`, `slaver`, `nob`, `lagavulin`, `sentry`, `slimeboss`, `guardian`, `hexaghost`, `shadow` (the ground ellipse — keep it in the bottom 8 rows).
- New enemy = new file + telling me to wire it to a species.
- Characters face **right** if they stand on the left (ironclad), face **left/neutral** for enemies.

### 2. Card art — `assets/cardart/art_<type>.png`
- **16x16**, shown inset on card faces in battle.
- Files: `art_strike`, `art_defend`, `art_power`, `art_skill`, `art_status`.
- **Shared palette budget**: card art + map icons together get **15 colors total** (they share GBA palette bank 13). Current palette: near-black outline (16,24,16), creams (232,232,200 / 176,176,144 / 120,120,96), orange (224,120,24), yellow (248,200,64), gold (224,176,48), browns (96,56,24 / 144,88,40), red (176,32,32), sage backfill (136,152,120). Reusing these = free; each brand-new color eats one of the remaining slots. The importer errors if the bank overflows.
- Fully transparent pixels render as **sage** (the inset background) — design on a sage swatch.

### 2b. Full-card faces — `assets/cards/<card>.png`
- **40x56**, full card face (frame + art + banner) shown in the battle hand.
- One file per card, named after the card id (`strike.png`, `flamebarrier.png`, ... see `tools/mkcards.py` ORDER).
- Auto-quantized to **12 colors** per card (k-means, 5-bit snapped) — palettes are
  streamed per hand slot into text banks 0-3 entries 4-15, so no shared budget.
- Alpha 0 = transparent (rounded corners show the battle backdrop).
- Regenerate with `python3 tools/mkcards.py` → `src/cardimg.h`, then rebuild.
  (Not part of `tools/assets.py import` — run it directly.)

### 3. Map node icons — `assets/mapicons/m_<name>.png`
- **16x16**, same shared bank-13 palette budget as card art.
- Files: `m_skull`, `m_elite`, `m_fire`, `m_chest`, `m_coin`, `m_question`, `m_boss`, `m_dot`.
- Transparent renders as sage (map background). Dimmed/visited versions are generated automatically.

### 4. Small UI icons — `assets/icons/<name>.png`
- **8x8**, shared **bank-11** palette (15-color budget across all of them). Current: outline, bone creams, gold/yellow, orange, red, browns.
- Files: `skull`, `elite`, `fire`, `chest`, `coin`, `question`, `boss` (legacy/small map set), `atk`, `def`, `buff`, `debuff` (battle intent + status icons).

### 5. Scenery tiles — `assets/tiles/<name>.png`
- **8x8**, must tile seamlessly. **Strict**: colors must come from the tile's existing bank palette (these interlock with other tiles; the importer errors on new colors — ask me if you want a bank's palette changed).
- Dungeon set (bank 10): `brick`, `brickdark`, `floor`, `floordark`, `arch`, `pillar`, `torch`. Map set (bank 12): `sage`, `sagelight`, `sagedark`.

### 6. Full-screen backgrounds — `assets/bg/`
- `battle.png` — battle backdrop. **Any size 3:2-ish** (auto cover-cropped to 240x160), auto-quantized to **11 colors** — keep it low-contrast/dark so HUD text reads over it. Avoid detail in the top 16px (HP bars) and bottom 40px (cards).
- `title.png` — title art. Any size (cover-cropped to 240x160), auto-quantized to 254 colors — anything goes. "NEW GAME" text is overlaid automatically.

## What happens on import

PNGs are converted back into the text art sources (`tools/art.txt`, `tools/bgart.txt`) or image headers, generators run with full validation, and you just rebuild. Everything is committed to git, so a bad import is always revertible.
