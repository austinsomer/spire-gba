#include "game.h"
#include "font8x8.h"

/* BG0 text: charblock 0, screenblock 31, prio 0
   BG1 ui:   charblock 0, screenblock 30, prio 1 */
#define SB_TXT 31
#define SB_UI  30

u32 frame_count;
static u16 keys_cur, keys_prev;
static u16 rep_key; static int rep_timer;

/* ---- palettes ---- */
static const u16 bank_colors[10][3] = {
    /* color1, color2, color3 per bank */
    { RGB15(31,31,31), RGB15(6,6,8),    RGB15(12,12,14) },  /* white / panel */
    { RGB15(16,16,18), RGB15(6,6,8),    RGB15(10,10,12) },  /* gray */
    { RGB15(31,8,8),   RGB15(10,2,2),   RGB15(20,4,4)   },  /* red */
    { RGB15(8,28,8),   RGB15(2,9,2),    RGB15(5,18,5)   },  /* green */
    { RGB15(10,16,31), RGB15(3,5,10),   RGB15(6,10,20)  },  /* blue */
    { RGB15(31,29,8),  RGB15(10,9,2),   RGB15(20,18,4)  },  /* yellow */
    { RGB15(31,18,4),  RGB15(10,6,1),   RGB15(20,12,2)  },  /* orange */
    { RGB15(8,28,28),  RGB15(2,9,9),    RGB15(5,18,18)  },  /* cyan */
    { RGB15(22,10,31), RGB15(7,3,10),   RGB15(14,6,20)  },  /* purple */
    { RGB15(18,2,2),   RGB15(6,1,1),    RGB15(12,2,2)   },  /* dark red */
};

static void load_tile1bpp(int index, const u8 *rows, int fg)
{
    /* expand 8 row-bytes (LSB = left pixel) to 4bpp tile */
    vu16 *dst = CHARBLOCK(0) + index * 16;
    for (int y = 0; y < 8; y++) {
        u8 r = rows[y];
        u32 line = 0;
        for (int x = 0; x < 8; x++)
            if (r & (1 << x)) line |= (u32)(fg & 0xF) << (x * 4);
        dst[y * 2]     = line & 0xFFFF;
        dst[y * 2 + 1] = line >> 16;
    }
}

static void load_tile_rows(int index, const u8 *rows, int fg, int bg)
{
    vu16 *dst = CHARBLOCK(0) + index * 16;
    for (int y = 0; y < 8; y++) {
        u8 r = rows[y];
        u32 line = 0;
        for (int x = 0; x < 8; x++)
            line |= (u32)((r & (1 << x)) ? fg : bg) << (x * 4);
        dst[y * 2]     = line & 0xFFFF;
        dst[y * 2 + 1] = line >> 16;
    }
}

void video_init(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG0CNT = BG_CBB(0) | BG_SBB(SB_TXT) | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(SB_UI)  | BG_PRIO(1);
    REG_BG0HOFS = 0; REG_BG0VOFS = 0;
    REG_BG1HOFS = 0; REG_BG1VOFS = 0;

    /* palettes: 10 banks, colors 1..3 */
    MEM_PAL_BG[0] = RGB15(2, 2, 4);  /* backdrop: near-black blue */
    for (int b = 0; b < 10; b++) {
        MEM_PAL_BG[b * 16 + 1] = bank_colors[b][0];
        MEM_PAL_BG[b * 16 + 2] = bank_colors[b][1];
        MEM_PAL_BG[b * 16 + 3] = bank_colors[b][2];
    }

    /* font tiles 0..127 */
    for (int c = 0; c < 128; c++)
        load_tile1bpp(c, font8x8_basic[c], 1);

    /* ui tiles */
    static const u8 solid[8]  = {255,255,255,255,255,255,255,255};
    load_tile1bpp(T_SOLID, solid, 1);
    load_tile1bpp(T_PANEL, solid, 2);
    for (int n = 0; n <= 8; n++) {           /* bar tiles */
        u8 rows[8];
        u8 fill = (u8)((1 << n) - 1);
        for (int y = 0; y < 8; y++) rows[y] = (y == 0 || y == 7) ? 0 : fill;
        vu16 *dst = CHARBLOCK(0) + (T_BAR0 + n) * 16;
        for (int y = 0; y < 8; y++) {
            u32 line = 0;
            for (int x = 0; x < 8; x++) {
                int px = (y == 0 || y == 7) ? 2 : ((rows[y] >> x) & 1 ? 1 : 2);
                line |= (u32)px << (x * 4);
            }
            dst[y * 2] = line & 0xFFFF; dst[y * 2 + 1] = line >> 16;
        }
    }
    static const u8 edge_t[8] = {0,255,0,0,0,0,0,0};
    static const u8 edge_b[8] = {0,0,0,0,0,0,255,0};
    static const u8 edge_l[8] = {2,2,2,2,2,2,2,2};
    static const u8 edge_r[8] = {64,64,64,64,64,64,64,64};
    static const u8 corn_tl[8]= {0,252,4,2,2,2,2,2};
    static const u8 corn_tr[8]= {0,63,32,64,64,64,64,64};
    static const u8 corn_bl[8]= {2,2,2,2,2,4,252,0};
    static const u8 corn_br[8]= {64,64,64,64,64,32,63,0};
    static const u8 arrow[8]  = {8,24,56,120,120,56,24,8};
    static const u8 heart[8]  = {0,102,255,255,255,126,60,24};
    static const u8 gold[8]   = {0,60,126,219,219,126,60,0};
    static const u8 energy[8] = {24,28,62,127,126,124,56,24};
    static const u8 shield[8] = {255,255,129,129,66,66,36,24};
    static const u8 dot[8]    = {0,0,24,60,60,24,0,0};
    load_tile1bpp(T_EDGE_T, edge_t, 1); load_tile1bpp(T_EDGE_B, edge_b, 1);
    load_tile1bpp(T_EDGE_L, edge_l, 1); load_tile1bpp(T_EDGE_R, edge_r, 1);
    load_tile1bpp(T_CORN_TL, corn_tl, 1); load_tile1bpp(T_CORN_TR, corn_tr, 1);
    load_tile1bpp(T_CORN_BL, corn_bl, 1); load_tile1bpp(T_CORN_BR, corn_br, 1);
    load_tile1bpp(T_ARROW, arrow, 1);
    load_tile_rows(T_HEART, heart, 1, 0);
    load_tile_rows(T_GOLDPT, gold, 1, 0);
    load_tile_rows(T_ENERGY, energy, 1, 0);
    load_tile_rows(T_SHIELD, shield, 1, 0);
    load_tile1bpp(T_DOT, dot, 1);

    txt_clear();
    ui_clear();

    /* hide all sprites */
    for (int i = 0; i < 128; i++) OAM_OBJS[i].attr0 = ATTR0_HIDE;

    /* vblank irq for vsync */
    ISR_VECTOR = (u32)0; /* set below via extern */
    extern void irq_stub(void);
    ISR_VECTOR = (u32)irq_stub;
    REG_DISPSTAT = 0x0008;      /* vblank irq enable */
    REG_IE = IRQ_VBLANK;
    REG_IME = 1;
}

void vsync(void)
{
    __asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3");
    frame_count++;
#ifdef AUTOPLAY
    /* heartbeat: spinner top-right proves CPU alive + in vsync loop */
    {
        extern GState gstate;
        static const char spin[4] = {'-', '/', '|', '+'};
        txt_putc(29, 0, spin[(frame_count >> 4) & 3], CLR_PURPLE);
        txt_putc(28, 0, '0' + (int)gstate, CLR_PURPLE);
    }
#endif
}

void key_poll(void)
{
    keys_prev = keys_cur;
    keys_cur = ~REG_KEYINPUT & KEY_ANY;
#ifdef AUTOPLAY
    /* monkey input: press random key every 8 frames */
    if ((frame_count & 7) == 0) {
        u32 r = frame_count * 2654435761u; r ^= r >> 13;
        switch (r % 10) {
        case 0: case 1: case 2: case 3: keys_cur |= KEY_A; break;
        case 4: case 5: keys_cur |= KEY_DOWN; break;
        case 6: keys_cur |= KEY_UP; break;
        case 7: keys_cur |= KEY_RIGHT; break;
        case 8: keys_cur |= KEY_START; break;
        default: break; /* rest */
        }
    }
#endif
}

u16 key_hit(u16 mask)  { return (keys_cur & ~keys_prev) & mask; }
u16 key_held(u16 mask) { return keys_cur & mask; }

u16 key_repeat(u16 mask)
{
    u16 hit = (keys_cur & ~keys_prev) & mask;
    if (hit) { rep_key = hit; rep_timer = 18; return hit; }
    if ((keys_cur & rep_key & mask)) {
        if (--rep_timer <= 0) { rep_timer = 6; return rep_key & mask; }
    } else rep_key &= ~ (u16)(~keys_cur);
    return 0;
}

/* ---- text ---- */
void txt_clear(void)
{
    vu16 *m = SCREENBLOCK(SB_TXT);
    for (int i = 0; i < 32 * 32; i++) m[i] = 0; /* tile 0 = blank in font */
}

void txt_putc(int x, int y, char c, int clr)
{
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    SCREENBLOCK(SB_TXT)[y * 32 + x] = (u16)((u8)c | (clr << 12));
}

void txt_put(int x, int y, const char *s, int clr)
{
    while (*s) txt_putc(x++, y, *s++, clr);
}

int txt_int_at(int x, int y, int v, int clr)
{
    char buf[12]; int i = 0, neg = v < 0;
    if (neg) v = -v;
    do { buf[i++] = '0' + v % 10; v /= 10; } while (v);
    if (neg) buf[i++] = '-';
    while (i) txt_putc(x++, y, buf[--i], clr);
    return x;
}

void txt_int(int x, int y, int v, int clr) { txt_int_at(x, y, v, clr); }

/* ---- ui layer ---- */
void ui_clear(void)
{
    vu16 *m = SCREENBLOCK(SB_UI);
    for (int i = 0; i < 32 * 32; i++) m[i] = 0;
}

void ui_tile(int x, int y, int tile, int clr)
{
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    SCREENBLOCK(SB_UI)[y * 32 + x] = (u16)(tile | (clr << 12));
}

void ui_fill(int x, int y, int w, int h, int tile, int clr)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            ui_tile(x + i, y + j, tile, clr);
}

void ui_box(int x, int y, int w, int h, int clr)
{
    ui_fill(x + 1, y + 1, w - 2, h - 2, T_PANEL, 0);
    ui_tile(x, y, T_CORN_TL, clr);
    ui_tile(x + w - 1, y, T_CORN_TR, clr);
    ui_tile(x, y + h - 1, T_CORN_BL, clr);
    ui_tile(x + w - 1, y + h - 1, T_CORN_BR, clr);
    for (int i = 1; i < w - 1; i++) {
        ui_tile(x + i, y, T_EDGE_T, clr);
        ui_tile(x + i, y + h - 1, T_EDGE_B, clr);
    }
    for (int j = 1; j < h - 1; j++) {
        ui_tile(x, y + j, T_EDGE_L, clr);
        ui_tile(x + w - 1, y + j, T_EDGE_R, clr);
    }
}

void ui_bar(int x, int y, int wtiles, int val, int max, int clr)
{
    if (val < 0) val = 0;
    if (max < 1) max = 1;
    int px = val * (wtiles * 8) / max;
    if (val > 0 && px == 0) px = 1;
    for (int i = 0; i < wtiles; i++) {
        int p = px - i * 8;
        if (p < 0) p = 0;
        if (p > 8) p = 8;
        ui_tile(x + i, y, T_BAR0 + p, clr);
    }
}

/* ---- rng ---- */
static u32 rng_state = 0xC0FFEE01;
void rng_seed(u32 s) { rng_state = s ? s : 0xC0FFEE01; }
u32 rng(void)
{
    u32 x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rng_state = x;
}
int rng_range(int n) { return n > 0 ? (int)(rng() % (u32)n) : 0; }

/* ---- sfx: psg square ch1 + noise ch4 ---- */
void sfx_init(void)
{
    REG_SOUNDCNT_X = 0x0080;   /* master on */
    REG_SOUNDCNT_L = 0xFF77;   /* full vol, all channels L+R */
    REG_SOUNDCNT_H = 0x0002;   /* psg 100% */
}

static void square(int freq, int env, int len)
{
    REG_SOUND1CNT_L = 0x0008;                 /* no sweep */
    REG_SOUND1CNT_H = (u16)(0x4000 | (env << 12) | 0x0100 | (63 - len)); /* duty 50%, env dec */
    REG_SOUND1CNT_X = (u16)(0x8000 | 0x4000 | (2048 - 131072 / freq));
}

void sfx_blip(void)  { square(880, 8, 8); }
void sfx_ok(void)    { square(1320, 10, 12); }
void sfx_bad(void)   { square(220, 10, 14); }
void sfx_heal(void)  { square(660, 10, 20); }
void sfx_block(void) { square(440, 9, 10); }

void sfx_hit(void)
{
    REG_SOUND4CNT_L = (u16)(0xA000 | (63 - 20)); /* env */
    REG_SOUND4CNT_H = 0x8034;                    /* trigger, noise params */
}
