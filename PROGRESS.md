# Slay the Spire GBA Demake — Progress

Goal: playable .gba rom, Slay the Spire act 1, Ironclad, direct clone names (personal use). Emulator: mGBA.

## Status (2026-07-11)
- [x] Toolchain: arm-none-eabi-gcc 16.1.0 (brew), mGBA.app (brew cask)
- [x] Build scaffold: Makefile, src/gba.ld, src/crt0.s, tools/gbafix.py (logo+checksum). `make` → build/spire.gba, `make run` opens mGBA
- [x] Engine (src/engine.c, src/gba.h, src/game.h): mode 0, BG0 text layer (font8x8 → 4bpp, 10 color banks), BG1 UI layer (boxes, HP bar tiles, icons), vsync via vblank IRQ + swi 0x05, input w/ key repeat, xorshift rng, PSG sfx blips
- [x] Title screen boots in mGBA, VERIFIED via screenshot (60fps, renders correct)
- [ ] src/stubs.c holds temp stubs for map/combat/etc — replace as real files land, then delete
- [ ] Task 2: cards (32 Ironclad cards planned, piles, upgrades)
- [ ] Task 3: combat (act 1 roster, elites Nob/Lagavulin/Sentries, bosses Slime Boss/Guardian/Hexaghost, intents, statuses)
- [ ] Task 4: map gen (7x15 StS-style), rewards, shop, rest, 4 events, relics
- [ ] Task 5: enemy sprite art (ASCII-art → 4bpp via tools script), polish, playtest

## Design decisions
- Player: Ironclad 80 HP, 3 energy, draw 5. Statuses: Wound/Burn/Slimed/Dazed.
- Screens run own loops, set `gstate` to advance (see main.c state machine).
- Verify visually: `make run`, then `screencapture -x <scratchpad>/shot.png` + Read image (mGBA has no headless mode; full-screen capture works).
- Text grid 30x20, txt_* on BG0, ui_* on BG1. Colors = palette banks (CLR_* enum).
