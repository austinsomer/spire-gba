#include "game.h"
#include "cards.h"
#include "sprites.h"

Run run;
GState gstate;
int combat_won;

int run_has_relic(u8 r)
{
    for (int i = 0; i < run.nrelics; i++)
        if (run.relics[i] == r) return 1;
    return 0;
}

void deck_add(u8 card_id)
{
    if (run.ndeck < MAX_DECK) {
        run.deck[run.ndeck].id = card_id;
        run.deck[run.ndeck].up = 0;
        run.ndeck++;
    }
}

#include "title_img.h"

void title_screen(void)
{
    /* full-screen mode-4 bitmap title (charblocks 0-2 clobbered) */
    obj_hide_all();
    vsync();
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
    for (int i = 0; i < 256; i++) MEM_PAL_BG[i] = title_pal[i];
    {
        vu16 *v = MEM_VRAM;
        for (int i = 0; i < 240 * 160 / 2; i++) v[i] = title_px[i];
    }

    for (int blink = 0;; blink++) {
        vsync(); key_poll();
        MEM_PAL_BG[TITLE_ARROW_PALIDX] =
            (blink & 32) ? TITLE_ARROW_OFF : TITLE_ARROW_ON;
        if (key_hit(KEY_START | KEY_A)) break;
    }
    rng_seed(frame_count * 2654435761u + 12345);
    sfx_ok();

    /* restore tiled video state */
    video_init();
    sprites_load();
    bg2_load();
    txt2x_reset();
    run_new();
    gstate = ST_MAP;
}

int main(void)
{
    video_init();
    sprites_load();
    bg2_load();
    sfx_init();
    gstate = ST_TITLE;

    for (;;) {
        switch (gstate) {
        case ST_TITLE:    title_screen();    break;
        case ST_MAP:      map_screen();      break;
        case ST_COMBAT:   combat_screen(map_pending_encounter); break;
        case ST_REWARD:   reward_screen(reward_elite); break;
        case ST_REST:     rest_screen();     break;
        case ST_SHOP:     shop_screen();     break;
        case ST_EVENT:    event_screen();    break;
        case ST_TREASURE: treasure_screen(); break;
        case ST_GAMEOVER: gameover_screen(); break;
        case ST_VICTORY:  victory_screen();  break;
        }
    }
}
