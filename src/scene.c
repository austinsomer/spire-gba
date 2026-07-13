/* BG2 scene composers — dungeon backdrops per screen */
#include "game.h"
#include "bgtiles.h"
#include "battlebg.h"

/* battle bg image: tiles live at BATTLEBG_BASE (spans cb1 free space into
   cb2). mode-4 title clobbers both, so reload per combat entry. */
void battle_bg_load(void)
{
    vu16 *dst = CHARBLOCK(1) + BATTLEBG_BASE * 16;
    const u16 *src = (const u16 *)battlebg_tiles;
    for (u32 i = 0; i < sizeof(battlebg_tiles) / 2; i++) dst[i] = src[i];
    for (int c = 0; c < 11; c++)
        MEM_PAL_BG[BATTLEBG_BANK * 16 + 4 + c] = battlebg_pal[c];
    hud_load();
}

/* brick wall with pillar/torch rhythm + arch windows */
static void wall(int y0, int y1)
{
    for (int y = y0; y <= y1; y++)
        for (int x = 0; x < 30; x++)
            bg2_tile(x, y, (x < 3 || x >= 27) ? TB_BRICKDARK : TB_BRICK);
    for (int x = 5; x < 30; x += 9)
        for (int y = y0 + 1; y <= y0 + 2 && y <= y1; y++)
            bg2_tile(x, y, TB_ARCH);
    if (y1 - y0 >= 2) {
        bg2_tile(2, y0 + 2, TB_TORCH);
        bg2_tile(27, y0 + 2, TB_TORCH);
    }
}

static void ground(int y0, int y1)
{
    for (int y = y0; y <= y1; y++)
        for (int x = 0; x < 30; x++)
            bg2_tile(x, y, ((x * 7 + y * 13) & 15) == 3 ? TB_FLOORDARK
                                                        : TB_FLOOR);
}

/* per-redraw restore: restamp the arena band (rows 2-11, erases stale
   intent/target stamps) and blank the card rows (12-19: 12 catches the
   card-play pop animation). No bg2_clear — clearing all 2048 cells every
   redraw blanked the screen for a beat (flicker). Top bar cells (rows
   0-1) are only ever relic/coin stamps, redrawn identically by draw_all. */
void scene_battle_refresh(void)
{
    for (int y = 0; y < BATTLEBG_ROWS; y++)
        for (int x = 0; x < 30; x++)
            bg2_stamp(x, y + 2, BATTLEBG_BASE + battlebg_map[y * 30 + x],
                      BATTLEBG_BANK, 0);
    bg2_fill(0, 12, 30, 8, 0);      /* hand zone -> near-black */
}

void scene_battle(void)   /* combat entry: full reset, then compose */
{
    bg2_clear();
    scene_battle_refresh();
}

/* big-hit impact flash: brighten the backdrop palette for a couple of
   frames (on=1), then restore (on=0) */
void battle_flash(int on)
{
    for (int c = 0; c < 11; c++) {
        u16 v = battlebg_pal[c];
        if (on) {
            int r = (v & 31) + 9, g = ((v >> 5) & 31) + 9,
                b = ((v >> 10) & 31) + 9;
            if (r > 31) r = 31;
            if (g > 31) g = 31;
            if (b > 31) b = 31;
            v = (u16)(r | (g << 5) | (b << 10));
        }
        MEM_PAL_BG[BATTLEBG_BANK * 16 + 4 + c] = v;
    }
}

void scene_title(void)
{
    bg2_clear();
    wall(0, 15);
    ground(16, 19);
}

void scene_map(void)
{
    bg2_clear();
    for (int y = 1; y < 18; y++)
        for (int x = 0; x < 30; x++)
            bg2_tile(x, y, TB_BRICKDARK);
}

void scene_shop(void)
{
    bg2_clear();
    wall(1, 3);
    ground(17, 19);
}

void scene_none(void)
{
    bg2_clear();
}
