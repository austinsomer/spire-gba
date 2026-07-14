/* Cold-boot intro video player.

   Plays the mode-4 frame stream in src/introdata.s (built by tools/mkvideo.py):
   240x160 8bpp frames at 15fps, double-buffered across the two mode-4 pages.
   After the last frame it holds the (text-free) final scene and flashes our own
   "PRESS START" (baked in the title font) until START/A, then fades to black —
   title_screen fades up from black on the far side for a seamless hand-off.
   START/A during playback skips straight to the PRESS START hold. */
#include "gba.h"
#include "game.h"
#include "introimg.h"
#include "slidesimg.h"

#define PAGE_SZ 0xA000
static u8 *page_ptr(int p) { return (u8 *)(0x06000000 + p * PAGE_SZ); }

/* ROM -> mode-4 VRAM page. VRAM is not 8-bit writable, so copy 32-bit; the
   frame layout (38912-byte frames, 512-byte palette prefix) keeps both ends
   4-byte aligned. */
static void copy_page(u8 *dst, const u8 *src)
{
    volatile u32 *d = (volatile u32 *)dst;
    const u32 *s = (const u32 *)src;
    for (int i = 0; i < INTRO_PIX_BYTES / 4; i++) d[i] = s[i];
}

/* blit the PRESS START patch (palette indices 254/255, 0 = transparent) into a
   page via read-modify-write per 16-bit word (byte-per-pixel, 16-bit writable). */
static void blit_ps(u8 *page)
{
    volatile u16 *v = (volatile u16 *)page;
    for (int py = 0; py < INTRO_PS_H; py++) {
        int y = INTRO_PS_Y + py;
        for (int px = 0; px < INTRO_PS_W; px++) {
            u8 b = intro_ps_patch[py * INTRO_PS_W + px];
            if (!b) continue;
            int x = INTRO_PS_X + px;
            int wi = (y * INTRO_W + x) >> 1;
            u16 w = v[wi];
            if (x & 1) w = (w & 0x00ff) | ((u16)b << 8);
            else       w = (w & 0xff00) | b;
            v[wi] = w;
        }
    }
}

static void load_pal(const u16 *pal)
{
    for (int c = 0; c < 256; c++) MEM_PAL_BG[c] = pal[c];
}

/* pre-intro splash slides: after the GBA bios, before the video. each slide
   fades up from black, holds ~2s, fades to black. A/START skips a slide. */
void slides_play(void)
{
    obj_hide_all();
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;           /* page 0 shown */
    REG_BLDCNT = 0x3F | (3 << 6);
    REG_BLDY = 16;                                 /* start on black */
    for (int s = 0; s < SLIDES_N; s++) {
        const u8 *fr = slides_data + s * SLIDE_FRAME_BYTES;
        const u16 *pal = (const u16 *)fr;
        const u8 *px = fr + SLIDE_PAL_BYTES;
        copy_page(page_ptr(0), px);                /* load under black */
        load_pal(pal);
        fade_in();                                 /* 16 -> 0 */
        int skip = 0;
        for (int f = 0; f < 120 && !skip; f++) {   /* ~2s at 60fps */
            vsync(); key_poll();
            if (key_hit(KEY_START | KEY_A)) skip = 1;
        }
        fade_out();                                /* 0 -> 16 (black) */
    }
    /* screen left on black; intro_play() takes over */
}

void intro_play(void)
{
    obj_hide_all();
    REG_BLDCNT = 0; REG_BLDY = 0;
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;            /* page 0 shown */
    pcm_play(PCM_INTRO);                            /* one-shot; plays over video + hold */
    int page = 0, skip = 0;

    for (int i = 0; i < INTRO_NFRAMES && !skip; i++) {
        const u8 *fr = intro_data + i * INTRO_FRAME_BYTES;
        const u16 *pal = (const u16 *)fr;
        const u8 *px = fr + INTRO_PAL_BYTES;
        int hidden = page ^ 1;
        copy_page(page_ptr(hidden), px);           /* draw the hidden page */
        for (int v = 0; v < 4; v++) {              /* 15fps: hold 4 vblanks */
            vsync(); key_poll();
            if (v == 0) {                          /* flip + palette at vblank */
                load_pal(pal);
                REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | (hidden ? 0x10 : 0);
                page = hidden;
            }
            if (key_hit(KEY_START | KEY_A)) { skip = 1; break; }
        }
    }

    /* hold the final (text-free) scene on page 0, same + PRESS START on page 1,
       then flash by flipping pages until START/A. */
    const u8 *fr = intro_data + (INTRO_NFRAMES - 1) * INTRO_FRAME_BYTES;
    const u16 *pal = (const u16 *)fr;
    const u8 *px = fr + INTRO_PAL_BYTES;
    load_pal(pal);
    copy_page(page_ptr(0), px);
    copy_page(page_ptr(1), px);
    blit_ps(page_ptr(1));
    vsync();
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;           /* clean page shown */

    int show = 0, blink = 0;
    for (;;) {
        vsync(); key_poll();
        if (++blink >= 24) {                       /* toggle ~0.4s */
            blink = 0; show ^= 1;
            REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | (show ? 0x10 : 0);
        }
        if (key_hit(KEY_START | KEY_A)) break;
    }
    pcm_stop();                                    /* stop intro music on start press */
    sfx_ok();
    vsync();
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;           /* fade the clean scene */
    fade_to_black();                               /* title_screen fades up */
}
