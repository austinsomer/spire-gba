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

/* menu lines: 0 NEW GAME, 1 CONTINUE (skipped without a save), 2 SETTINGS */
void title_screen(void)
{
    int sel = 0;
    for (;;) {
        int have = save_run_valid();
        /* full-screen mode-4 bitmap title (charblocks 0-2 clobbered) */
        obj_hide_all();
        vsync();
        REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
        for (int i = 0; i < 256; i++) MEM_PAL_BG[i] = title_pal[i];
        {
            vu16 *v = MEM_VRAM;
            for (int i = 0; i < 240 * 160 / 2; i++) v[i] = title_px[i];
        }
        if (sel == 1 && !have) sel = 0;

        for (int blink = 0;; blink++) {
            vsync(); key_poll();
            int on = !(blink & 32);
            for (int li = 0; li < TITLE_NLINES; li++)
                MEM_PAL_BG[TITLE_ARROW_PALIDX + li] =
                    (sel == li && on) ? TITLE_ARROW_ON : TITLE_ARROW_OFF;
            if (key_hit(KEY_UP) && sel > 0) {
                sel--;
                if (sel == 1 && !have) sel = 0;
                sfx_blip();
            }
            if (key_hit(KEY_DOWN) && sel < TITLE_NLINES - 1) {
                sel++;
                if (sel == 1 && !have) sel = 2;
                sfx_blip();
            }
            if (key_hit(KEY_START | KEY_A)) break;
        }
        sfx_ok();

        /* restore tiled video state */
        video_init();
        sprites_load();
        bg2_load();
        txt2x_reset();

        if (sel == 0) {
            rng_seed(frame_count * 2654435761u + 12345);
            run_new();
            gstate = ST_MAP;
            return;
        }
        if (sel == 1) {
            if (save_run_load()) { gstate = ST_MAP; return; }
            continue;            /* corrupt save: back to menu */
        }
        settings_screen();   /* returns here; loop reloads the bitmap */
    }
}

int main(void)
{
    video_init();
    sprites_load();
    bg2_load();
    sfx_init();
    music_init();          /* PSG tracker kept but idle; music is PCM now */
    save_init();           /* waitstates + persisted settings (pre-pcm) */
#ifdef SETTINGSTEST
    settings_screen(); gstate = ST_TITLE;      /* settings screen peek */
#elif defined(MAPTEST)
    rng_seed(7); run_new(); gstate = ST_MAP;   /* skip title for map peek */
#elif defined(BATTLETEST)
    rng_seed(7); run_new(); map_pending_encounter = 1; gstate = ST_COMBAT;
#else
    gstate = ST_TITLE;
#endif

    GState prev = ST_TITLE;
    for (;;) {
        pcm_play(gstate == ST_TITLE ? PCM_TITLE : PCM_RUN);
        /* room resolved -> back on the map: snapshot the run */
        if (gstate == ST_MAP && prev != ST_MAP && prev != ST_TITLE)
            save_run();
        prev = gstate;
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
