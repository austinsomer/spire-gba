# Battle UI asset prompts — 3 batches

Generate each batch as its own image. Paste the ENTIRE batch block (style
header + item list) into Nano Banana each time — the repeated style header
keeps the sheets visually consistent with the card set. Save each result
and give Claude the file paths; slicing/import is automated from there.

Lesson from the card sheets: the model sometimes adds duplicate or extra
cells. The counts and "exactly" language below fight that — if a sheet
comes back with extras, just hand it over anyway; Claude picks the best
variant of any duplicate during slicing.

---

## BATCH 1 of 3 — Battle backdrop

Create a single high-resolution wide landscape image, 3:1 aspect ratio.

STYLE: 16-bit pixel art, dark-fantasy dungeon-crawler aesthetic inspired
by Slay the Spire, matching a retro pixel-art card game. Chunky pixels,
dark outlines, very limited palette (10 colors maximum), no gradients, no
dithering noise.

THE SCENE: an empty dungeon combat arena viewed from the side, like a
theater stage. A large flat oval of mossy green-gray stone floor fills the
lower half — this is the ground the fighters stand on. Behind it, a dark
dungeon: heavy shadowed stone columns or tree-like pillars fading into
near-black darkness at the top and sides. The middle of the floor oval is
subtly lighter, as if lit from above. NO characters, NO creatures, NO
objects on the floor — the arena must be completely empty. Keep the top
quarter and the far left/right edges very dark and low-detail, since UI
text is drawn over them.

Mood: ominous, quiet, torch-lit gloom. High contrast between the lit
floor oval and the dark walls.

---

## BATCH 2 of 3 — Relic icons (8)

Create a single high-resolution sprite sheet: a 4-column by 2-row grid of
exactly 8 small fantasy trinket icons, on a plain solid dark gray
background with wide uniform spacing between icons. Exactly 8 icons — no
duplicates, no extra cells, nothing between the cells.

STYLE (applies to every icon): 16-bit pixel art, dark-fantasy
dungeon-crawler aesthetic inspired by Slay the Spire. Each icon is a
single centered object with a thick dark outline, chunky pixels, bold
silhouette, high contrast, no background scene behind it (plain gray
shows around each icon). The whole set SHARES one small palette of about
12 colors — golds, bronzes, deep reds, warm browns, one green accent.
Icons must stay readable when shrunk to 16x16 pixels: one clear shape
each, no fine detail, no text.

THE 8 ICONS, in exact grid order (left to right, top to bottom):

Row 1: BURNING BLOOD (a fat drop of blood wreathed in a small orange
flame) • VAJRA (a golden double-ended thunderbolt scepter) • BRONZE
SCALES (a cluster of three overlapping spiked bronze scales) • ANCHOR (a
heavy iron ship anchor)

Row 2: LANTERN (a brass lantern glowing warm yellow) • STRAWBERRY (a
bright red strawberry with a green leaf cap) • BAG OF PREPARATION (a
bulging drawstring satchel with two card corners poking out the top) •
BLOOD VIAL (a corked glass vial of glowing red liquid)

Same lighting, same pixel density, same rendering style across all 8
icons. Every icon fully inside its grid cell.

---

## BATCH 3 of 3 — Battle HUD elements (10)

Create a single high-resolution sprite sheet: a 5-column by 2-row grid of
exactly 10 game interface elements for a retro pixel-art card battler, on
a plain solid dark gray background with wide uniform spacing between
elements. Exactly 10 elements — no duplicates, no extra cells, nothing
between the cells.

STYLE (applies to every element): 16-bit pixel art, dark-fantasy
dungeon-crawler aesthetic inspired by Slay the Spire. Thick dark
outlines, chunky pixels, bold readable silhouettes, high contrast. The
whole set SHARES one small palette of about 12 colors — deep red, warm
gold/yellow, steel gray, dark leather brown, one cool blue accent. These
are UI elements drawn over a black interface bar, so make them pop:
bright rims, dark cores. Each element must stay readable when shrunk very
small: no fine detail, no tiny text except where a label is explicitly
asked for.

THE 10 ELEMENTS, in exact grid order (left to right, top to bottom):

Row 1: DRAW PILE (the back of a playing card, portrait orientation, dark
red leather back with a gold swirling flame emblem in the center) •
DISCARD PILE (the same card back but tilted slightly and rendered in
desaturated cool grays, looking spent) • ENERGY ORB (a round glossy
energy sphere split into red, yellow and blue segments, thick gold rim —
like a cracked elemental core) • END TURN BUTTON (a rounded rectangular
stone button with a beveled steel border, the words "END TURN" carved in
bold blocky pixel capitals, faint blue glow) • TARGETING ARROW (a bold
red downward-pointing arrowhead / chevron, slightly glossy)

Row 2: GOLD COIN (a thick gold coin, face-on, with a simple stamped
emblem) • ATTACK INTENT (two crossed steel swords) • DEFEND INTENT (a
sturdy kite shield, face-on) • BUFF INTENT (a thick upward-pointing
golden arrow with a small glow) • DEBUFF INTENT (a thick downward-
pointing sickly purple arrow, slightly jagged)

Same lighting, same pixel density, same rendering style across all 10
elements. Every element fully inside its grid cell.

---

## Slicing reference (Claude uses this on import)

Batch 1 → backdrop, cover-cropped to 240x80, auto-quantized to 11 colors
Batch 2 → `burningblood vajra bronzescales anchor / lantern strawberry
bagofprep bloodvial` — each 16x16, whole set shares ≤12 colors
Batch 3 → `drawpile discardpile energy endturn target / coin atk def
buff debuff` — targets: drawpile+discardpile 16x24, energy 24x24,
endturn 32x24, target 16x16, coin 16x16, intents 16x16; whole set
shares ≤12 colors

Provide the three file paths to Claude when generated. Claude slices,
downscales, quantizes, and restructures the battle screen (top relic/gold
bar, arena y16-95, black hand zone with arched overlapping cards, pile
counters, energy orb, end-turn button).
