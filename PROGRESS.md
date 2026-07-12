# Slay the Spire GBA Demake — Progress

Goal: playable .gba rom, Slay the Spire act 1, Ironclad, direct clone names (personal use). Emulator: mGBA.

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

## Design decisions
- Player: Ironclad 80 HP, 3 energy, draw 5. Statuses: Wound/Burn/Slimed/Dazed.
- Screens run own loops, set `gstate` to advance (see main.c state machine).
- Verify visually: `make run`, then capture mGBA WINDOW directly (works even occluded): pyobjc Quartz CGWindowListCopyWindowInfo → find owner 'mGBA' w/ name → `screencapture -x -l<windowid>`. NOTE: mGBA auto-pauses when fully occluded — identical frames ≠ game hang. osascript keystrokes BLOCKED (no accessibility perm) → use `-DAUTOPLAY` builds: combat bot + monkey input in key_poll.
- `make EXTRA=-DAUTOPLAY` = self-playing build for soak tests. Ship build: plain `make` (do `make clean` when switching!).
- Text grid 30x20, txt_* on BG0, ui_* on BG1. Colors = palette banks (CLR_* enum).
- No libc: src/mem.c provides memcpy/memmove/memset (gcc emits calls for struct copies).
- Map: only current-floor nodes selectable, edges from 4 random walks (union at shared nodes, no dead ends).
