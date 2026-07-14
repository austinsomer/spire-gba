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

/* Blit a menu-line text patch into mode-4 VRAM at (x0,y0).  VRAM is
   byte-per-pixel but only 16-bit-writable, so replace the target byte with a
   read-modify-write on the u16 word, selecting the byte by x parity.  Patch
   bytes are palette indices (already "+1" adjusted); 0 = transparent (skip,
   leaving the key art underneath). */
static void title_blit_patch(int li, int x0, int y0)
{
    const u8 *p = title_patch[li];
    int pw = title_patch_w[li], ph = title_patch_h[li];
    for (int py = 0; py < ph; py++) {
        int y = y0 + py;
        if (y < 0 || y >= 160) continue;
        for (int px = 0; px < pw; px++) {
            u8 b = p[py * pw + px];
            if (!b) continue;                 /* transparent */
            int x = x0 + px;
            if (x < 0 || x >= 240) continue;
            int wi = (y * 240 + x) >> 1;       /* u16 word index */
            u16 w = MEM_VRAM[wi];
            if (x & 1) w = (w & 0x00ff) | ((u16)b << 8);
            else       w = (w & 0xff00) | b;
            MEM_VRAM[wi] = w;
        }
    }
}

/* menu lines: 0 NEW GAME, 1 CONTINUE (hidden without a save), 2 SETTINGS */
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

        /* reflow: visible menu lines (skip CONTINUE when there is no save),
           vertically centered as a block under the logo, each patch centered
           horizontally, then blitted onto the clean key art. */
        int vis[TITLE_NLINES], nvis = 0;
        for (int li = 0; li < TITLE_NLINES; li++)
            if (li != 1 || have) vis[nvis++] = li;
        int block = nvis * TITLE_LINE_H + (nvis - 1) * TITLE_LINE_GAP;
        int ytop = TITLE_MENU_TOP + (TITLE_MENU_BOT - TITLE_MENU_TOP - block) / 2;
        for (int k = 0; k < nvis; k++) {
            int li = vis[k];
            int y0 = ytop + k * (TITLE_LINE_H + TITLE_LINE_GAP);
            int x0 = (240 - title_patch_w[li]) / 2;
            title_blit_patch(li, x0, y0);
        }

        fade_from_black();      /* seamless fade up from the intro (no-op otherwise) */

        if (sel == 1 && !have) sel = 0;

        for (;;) {
            vsync(); key_poll();
            /* steady highlight: selected line fill = gold, others = cream */
            for (int li = 0; li < TITLE_NLINES; li++)
                MEM_PAL_BG[TITLE_FILL_PALIDX + li] =
                    (sel == li) ? TITLE_FILL_ON : TITLE_FILL_OFF;
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
            neow_screen();
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
#elif defined(NEOWTEST)
    rng_seed(7); run_new(); neow_screen(); gstate = ST_MAP;  /* neow peek */
#elif defined(TITLETEST)
    gstate = ST_TITLE;
#else
    slides_play();              /* pre-intro splash slides (bios -> slides -> video) */
    intro_play();               /* cold-boot intro video, ends faded to black */
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
