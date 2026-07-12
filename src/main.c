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

void title_screen(void)
{
    txt_clear(); ui_clear();
    scene_title();

    /* logo panel over the wall */
    ui_fill(3, 2, 24, 7, T_PANEL, CLR_GRAY);
    ui_box(2, 1, 26, 9, CLR_DKRED);
    txt_put2x(7, 2, "SLAY THE", CLR_YELLOW);
    txt_put2x(10, 4, "SPIRE", CLR_YELLOW);
    txt_put(10, 8, "GBA DEMAKE", CLR_GRAY);

    /* heroes flanking the menu, standing on the floor */
    obj_show(0, SPR_IRONCLAD, 32, 104);
    obj_show(1, SPR_CULTIST, 176, 104);

    txt_put(10, 13, "> NEW GAME", CLR_WHITE);
    txt_put(8, 18, "IRONCLAD - ACT I", CLR_ORANGE);

    int blink = 0;
    for (;;) {
        vsync(); key_poll();
        blink++;
        if ((blink & 31) == 0)
            txt_put(10, 13, (blink & 32) ? "  NEW GAME" : "> NEW GAME", CLR_WHITE);
        if (key_hit(KEY_START | KEY_A)) {
            rng_seed(frame_count * 2654435761u + 12345);
            sfx_ok();
            obj_hide_all();
            run_new();
            gstate = ST_MAP;
            return;
        }
    }
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
