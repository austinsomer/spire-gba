/* BG2 scene composers — dungeon backdrops per screen */
#include "game.h"
#include "bgtiles.h"

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

void scene_battle(void)
{
    bg2_clear();
    wall(1, 10);
    ground(11, 13);
    /* rows 0 and 14-19 stay backdrop-dark for HUD + hand */
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
