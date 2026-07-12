# Slay the Spire GBA Demake — Progress

Goal: playable .gba rom, Slay the Spire act 1, Ironclad, direct clone names (personal use). Emulator: mGBA.
Repo: https://github.com/austinsomer/spire-gba (build/ untracked — `make` regenerates build/spire.gba)

## Status (2026-07-11)
- [x] Toolchain: arm-none-eabi-gcc 16.1.0 (brew), mGBA.app (brew cask)
- [x] Build scaffold: Makefile, src/gba.ld, src/crt0.s, tools/gbafix.py (logo+checksum). `make` → build/spire.gba, `make run` opens mGBA
- [x] Engine (src/engine.c, src/gba.h, src/game.h): mode 0, BG0 text layer (font8x8 → 4bpp, 10 color banks), BG1 UI layer (boxes, HP bar tiles, icons), vsync via vblank IRQ + swi 0x05, input w/ key repeat, xorshift rng, PSG sfx blips
- [x] Title screen boots in mGBA, VERIFIED via screenshot (60fps, renders correct)
- [x] Task 2: cards (src/cards.h/.c): 32 Ironclad cards + 4 statuses, upgrades, piles (draw/hand/discard/exhaust), deck browser UI. VERIFIED in mGBA.
- [x] Task 3: combat (src/combat.c): full act 1 roster + elites Nob/Lagavulin/Sentries + bosses SlimeBoss(split)/Guardian/Hexaghost, intents w/ live damage numbers, weak/vuln/str/thorns/metallicize/barricade/demonform, targeting UI, autoplay bot (-DAUTOPLAY) ran all 13 encounters clean: normals+Nob+Sentries won, Lagavulin+bosses killed starter deck (expected)
- [x] Task 4: map (src/map.c) 7x15 4-walk gen, room routing; screens (src/screens.c): reward (gold/card-of-3/elite relic), shop (5 cards/relic/remove), rest (heal/smith), 4 events (Big Fish, Golden Idol, Cleric, Living Wall), treasure, gameover, victory; 8 relics w/ combat hooks. stubs.c deleted.
- [x] Enemy sprites: 15 32x32 OBJ sprites (tools/art.txt ASCII → tools/mksprites.py → src/sprites.h). OAM shadow committed in vsync (OAM ignores writes outside vblank). VERIFIED in mGBA (slimes w/ faces render).
- [x] FIXED: memset self-recursion — gcc pattern-matched freestanding memset body into a memset call → infinite recursion, SP underflow, wild exec. mem.o builds w/ -fno-builtin -fno-tree-loop-distribute-patterns. Found via mGBA gdb stub (-g) + raw RSP python client (no arm-gdb on box; lldb batch dies on SIGINT). Single-step SP trace nailed it.
- [x] Makefile: header deps now tracked ($(HDRS)) — stale engine.o w/ old sprites.h cost a debug cycle.
- [x] Task 5: 6+ min monkey soak clean (deep runs, shop/relics cycling, sprites across species), ship build boots to title. GAME COMPLETE.
- [x] UI overhaul (user mockups): BG2 scenery layer (charblock 1, sbb 29, banks 10-15, tools/mkbg.py + tools/bgart.txt), scene composers (src/scene.c). Title: 2x gold logo (txt_put2x synthesizes scaled glyphs into cb0 200+), wall+torches, ironclad+cultist sprites. Map: icon nodes (skull/elite/fire/chest/coin/?/boss) + legend. Battle: stage layout — ironclad left (oam5), enemies right at 120+i*40, intents above, hp bars below, card-strip hand (5 slots, type-colored frames), L/R select. Shop: keeper sprite (looter), panel list, desc on bottom line. All VERIFIED via mGBA window screenshots.
- Art authored by subagent: 7 bg tiles + 7 icons (tools/bgart.txt), @ironclad sprite (art.txt).
- [x] Map overhaul: BG2 = 32x64 scrolling layer (SB 28+29, vofs camera). 16x16 sage-backfilled icons (mkbg @big: + !opaque), runtime dashed/solid path patterns + dim bank 14 + line/cursor bank 15 (src/map.c). UP/DOWN pans, camera centers current floor. MAPTEST define = pre-walked map for screenshots. Pattern tiles rebuilt on every map_screen entry (mode-4 title clobbers cb1).
- [x] Bitmap title: user key art via tools/mkimage.py (mode 4, 254-color quantize, font8x8 1.5x "NEW GAME" bake at y~120, arrow on reserved pal slot 255 → blinked by palette cycling in title loop). Source png kept at tools/title_src.png. Mode-4 exit MUST: video_init + sprites_load + bg2_load + txt2x_reset (bitmap clobbers charblocks 0-2; bg2_load re-blanks cb1 tile 0).

- [x] Battle UX overhaul (user mock): image backdrop via tools/mkbattlebg.py (deduped tiles at index 320+, bank 9 entries 4-14, battle_bg_load per combat entry), framed hp bars top, status lines w/ icons, intent icon+dmg, shadow sprites, 4-slot big card row (cost orb/name banner/16x16 art inset via BG2 window in BG1 fill/dmg). Sprites prio 2 (HUD renders over). FIXED: sprite palettes >16 banks wrapped into BG bank 0 (mksprites now packs shared banks); gcc strlen emission (mem.c). BATTLETEST define boots straight into jaw worm fight.
- [x] PNG asset pipeline: tools/assets.py export/import + ASSETS.md — user edits PNGs in assets/, importer splices back into art sources w/ palette-budget validation. Round-trip verified.

- [x] Full-card art (2026-07-12): user's 3 Gemini sheets (asset-grids/) sliced → assets/cards/*.png (36 cards, 40x56, alpha corners). tools/mkcards.py → src/cardimg.h (per-card 12-color k-means palettes, 4bpp 5x7 tiles, 40KB). Render: engine card_face_load/stamp — tiles streamed into cb1 gap 176-315 (4 slots x 35), palettes into TEXT banks 0-3 entries 4-15 (text uses only 1-3; OBJ banks were full: 14/16 used). combat draw_hand rewritten: 4 slots at cols 4/10/16/22 rows 13-19, yellow ui_box sel (interior cleared — ui_box fills!), live cost orb/dmg/blk overlays; STAGE_Y 80→64 so sprites+shadows end at card top (y104); enemy mini-bars row 13→12, enemy name row 12→1, hints → row 0 center. face_loaded[] cache reset at combat entry (cb1 reload). Dup variants on sheets resolved (defend/flex/pommel/carnage/entrench/flamebarrier); extra "STATUS" card on sheet A unused. VERIFIED: BATTLETEST screenshots + AUTOPLAY soak.

## IN FLIGHT — battle screen redesign (started 2026-07-12, waiting on user art)

User is generating 3 asset sheets w/ Nano Banana from BATTLE_UI_PROMPTS.md
(backdrop / 8 relic icons / 10 HUD elements — slicing reference + target
sizes + palette budgets at bottom of that file). When user provides the
sheet paths, slice + import + rebuild the battle screen to this layout:

- **Top bar y0-15**: owned relic icons 16x16 left-to-right from (0,0); coin icon + gold count right-aligned. Black/dark panel strip.
- **Arena y16-95**: new backdrop 240x80 (replaces 240x160 one; rework tools/mkbattlebg.py — still bank 9, 11 colors, entries 4-14). Player sprite left, enemies right, hp bars UNDER each sprite in-arena (framed bar + hp/max overlaid, like mockup), intent icon+dmg above enemy, target arrow (h_target) over targeted enemy instead of '>' char.
- **Row 12 (y96-103)**: selected card name + desc text, centered.
- **Hand zone y104-159, black** (backdrop reg color / bg fill): cards arched + overlapped. BG2 = 8px granularity → overlap = stamp left-to-right w/ 8px column overlap, selected card stamped LAST (fully visible) + maybe 8px raise. Frees width: 5 visible slots feasible (5x40 - 4x8 = 168px).
- **Hand-zone HUD**: left = h_drawpile 16x24 + draw count, h_energy 24x24 + "3/3" overlay; right = h_discardpile + discard count, h_endturn 32x24 + R hint. Replaces the old col-0 text stack and R:END.

Implementation notes (decided this session, reuse):
- Palette plan: relic set + HUD set each get their own streamed 12-color set in FREE text-bank upper entries — banks 0-3 entries 4-15 hold the 4 card slots; banks 4-8 entries 4-15 are still free (text/UI only ever touch entries 1-3; verified engine.c). Suggest bank 4 = HUD, bank 5 = relics.
- Charblock 1 free gap after card faces: tiles 63..175 (cards live 176..315, battlebg 320..916). HUD/relic tiles fit there, or charblock 0 (icons region ~171+, 2x-text 200+ — check collisions w/ BIG_BASE 200).
- Sheet slicing: tools/slicegrid.py (this session) dumps cell crops from a gray-bg grid sheet; handles touching cells via min-mask seam split. Then per-set: resize to target, k-means quantize (like tools/mkcards.py), joint palette for shared sets.
- Gemini sheets may contain duplicate/extra cells — slice all, eyeball, pick best variant (card sheets needed this: defend/flex/pommel/carnage/entrench/flamebarrier dups).
- Gold display exists in run state (run.gold); relics in run.relics[]/nrelics, ids match relic_names order = prompt order (burningblood vajra bronzescales anchor lantern strawberry bagofprep bloodvial).
- After layout change re-verify: BATTLETEST screenshot, AUTOPLAY soak, ship build boots.

## Design decisions
- Player: Ironclad 80 HP, 3 energy, draw 5. Statuses: Wound/Burn/Slimed/Dazed.
- Screens run own loops, set `gstate` to advance (see main.c state machine).
- Verify visually: `make run`, then capture mGBA WINDOW directly (works even occluded): pyobjc Quartz CGWindowListCopyWindowInfo → find owner 'mGBA' w/ name → `screencapture -x -l<windowid>`. NOTE: mGBA auto-pauses when fully occluded — identical frames ≠ game hang. osascript keystrokes BLOCKED (no accessibility perm) → use `-DAUTOPLAY` builds: combat bot + monkey input in key_poll.
- `make EXTRA=-DAUTOPLAY` = self-playing build for soak tests. Ship build: plain `make` (do `make clean` when switching!).
- Text grid 30x20, txt_* on BG0, ui_* on BG1. Colors = palette banks (CLR_* enum).
- No libc: src/mem.c provides memcpy/memmove/memset (gcc emits calls for struct copies).
- Map: only current-floor nodes selectable, edges from 4 random walks (union at shared nodes, no dead ends).
