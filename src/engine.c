#include "game.h"
#include "font8x8.h"

/* BG0 text: charblock 0, screenblock 31, prio 0
   BG1 ui:   charblock 0, screenblock 30, prio 1 */
#define SB_TXT 31
#define SB_UI  30
#define SB_BG  28   /* 32x64: uses screenblocks 28+29 */

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
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 |
                  DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG0CNT = BG_CBB(0) | BG_SBB(SB_TXT) | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(SB_UI)  | BG_PRIO(1);
    REG_BG2CNT = BG_CBB(1) | BG_SBB(SB_BG) | BG_PRIO(2) | (2 << 14); /* 32x64 */
    REG_BG0HOFS = 0; REG_BG0VOFS = 0;
    REG_BG1HOFS = 0; REG_BG1VOFS = 0;
    REG_BG2HOFS = 0; REG_BG2VOFS = 0;

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
    static const u8 orb[8]    = {60,126,255,255,255,255,126,60};
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
    load_tile_rows(T_ORB, orb, 1, 0);   /* filled disc, transparent corners */

    txt_clear();
    ui_clear();

    /* hide all sprites */
    obj_hide_all();

    /* vblank irq for vsync */
    ISR_VECTOR = (u32)0; /* set below via extern */
    extern void irq_stub(void);
    ISR_VECTOR = (u32)irq_stub;
    REG_DISPSTAT = 0x0008;      /* vblank irq enable */
    REG_IE = IRQ_VBLANK;
    REG_IME = 1;
}

/* OAM shadow: hardware OAM only writable in vblank, so obj_* edit this
   and vsync() commits it right after the vblank wait */
static ObjAttr oam_shadow[128];

void vsync(void)
{
    __asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3");
    {
        volatile u32 *dst = (volatile u32 *)MEM_OAM;
        const u32 *src = (const u32 *)oam_shadow;
        for (int i = 0; i < 128 * 2; i++) dst[i] = src[i];
    }
    frame_count++;
    music_tick();
    pcm_tick();
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
        case 8: keys_cur |= (frame_count & 16) ? KEY_START : KEY_SELECT; break;
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

/* ---- obj sprites ---- */
#define SPRITES_DATA
#include "sprites.h"

void sprites_load(void)
{
    /* affine param group 0 = 0.5 scale matrix -> 2x magnification */
    oam_shadow[0].pad = 0x0080; oam_shadow[1].pad = 0;
    oam_shadow[2].pad = 0;      oam_shadow[3].pad = 0x0080;
    vu16 *dst = (vu16 *)0x06010000;
    const u16 *src16 = (const u16 *)sprite_tiles;
    for (u32 i = 0; i < sizeof(sprite_tiles) / 2; i++) dst[i] = src16[i];
    for (int s = 0; s < N_SPRITE_BANKS; s++) {
        MEM_PAL_OBJ[s * 16] = 0;
        for (int c = 0; c < 15; c++)
            MEM_PAL_OBJ[s * 16 + 1 + c] = sprite_pals[s][c];
    }
}

void obj_show(int i, int sprite, int x, int y)
{
    oam_shadow[i].attr0 = (u16)(ATTR0_Y(y) | ATTR0_SQUARE);
    oam_shadow[i].attr1 = (u16)(ATTR1_X(x) | ATTR1_SIZE(2));
    oam_shadow[i].attr2 = (u16)(ATTR2_TILE(sprite_tile_ofs[sprite]) |
                                ATTR2_PAL(sprite_pal_bank[sprite]) |
                                ATTR2_PRIO(2));
}

void obj_show_big(int i, int sprite, int x, int y)
{
    /* 32x32 source magnified 2x via affine group 0 (double-size box 64x64) */
    oam_shadow[i].attr0 = (u16)(ATTR0_Y(y) | ATTR0_SQUARE | 0x0100 | 0x0200);
    oam_shadow[i].attr1 = (u16)(ATTR1_X(x) | ATTR1_SIZE(2));   /* param 0 */
    oam_shadow[i].attr2 = (u16)(ATTR2_TILE(sprite_tile_ofs[sprite]) |
                                ATTR2_PAL(sprite_pal_bank[sprite]) |
                                ATTR2_PRIO(2));
}

void obj_hide(int i) { oam_shadow[i].attr0 = ATTR0_HIDE; }

void obj_hide_all(void)
{
    for (int i = 0; i < 128; i++) oam_shadow[i].attr0 = ATTR0_HIDE;
}

/* ---- bg2 scenery + icons ---- */
#define BGTILES_DATA
#include "bgtiles.h"

void bg2_load(void)
{
    /* tile 0 must be blank (mode-4 title clobbers charblock 1) */
    for (int i = 0; i < 16; i++) CHARBLOCK(1)[i] = 0;
    /* scenery tiles -> charblock 1 */
    for (int t = 1; t < N_BGTILES; t++) {
        vu16 *dst = CHARBLOCK(1) + t * 16;
        const u32 *src = &bgtile_px[(t - 1) * 8];
        for (int y = 0; y < 8; y++) {
            dst[y * 2]     = src[y] & 0xFFFF;
            dst[y * 2 + 1] = src[y] >> 16;
        }
    }
    /* icon tiles -> charblock 0 at ICON_BASE */
    for (int t = 0; t < N_ICONS; t++) {
        vu16 *dst = CHARBLOCK(0) + (ICON_BASE + t) * 16;
        const u32 *src = &icon_px[t * 8];
        for (int y = 0; y < 8; y++) {
            dst[y * 2]     = src[y] & 0xFFFF;
            dst[y * 2 + 1] = src[y] >> 16;
        }
    }
    /* palette banks 10..15 */
    for (int b = 0; b < 6; b++)
        for (int c = 0; c < 15; c++)
            MEM_PAL_BG[(10 + b) * 16 + 1 + c] = bgbank_pal[b][c];
}

static vu16 *bg2_cell(int x, int y)
{
    if (x < 0 || x >= 32 || y < 0 || y >= 64) return 0;
    return &SCREENBLOCK(SB_BG + (y >> 5))[(y & 31) * 32 + x];
}

void bg2_tile(int x, int y, int tile)
{
    vu16 *c = bg2_cell(x, y);
    if (c) *c = (u16)(tile | (bg_tile_bank[tile] << 12));
}

void bg2_stamp(int x, int y, int tile, int bank, int hflip)
{
    vu16 *c = bg2_cell(x, y);
    if (c) *c = (u16)(tile | (hflip ? 0x0400 : 0) | (bank << 12));
}

void bg2_scroll(int y) { REG_BG2VOFS = (u16)y; }

void bg2_fill(int x, int y, int w, int h, int tile)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            bg2_tile(x + i, y + j, tile);
}

void bg2_clear(void)
{
    vu16 *m = SCREENBLOCK(SB_BG);
    for (int i = 0; i < 32 * 32 * 2; i++) m[i] = 0;  /* both screenblocks */
    REG_BG2VOFS = 0;
}

/* ---- full-card faces (battle hand) ----
   tiles streamed into the charblock-1 gap (63..319), palettes into text
   banks 0..3 entries 4..15 (text itself only uses entries 1..3) */
#include "cardimg.h"
#define CARDFACE_TILE0 176   /* 5 slots x 35 tiles = 176..350 */

/* slot -> BG palette bank: banks 0-3 + 6 (4/5 = HUD/relics, 9 = battle bg) */
static const u8 face_bank[5] = { 0, 1, 2, 3, 6 };

void card_face_load(int slot, int card_id)
{
    vu16 *dst = CHARBLOCK(1) + (CARDFACE_TILE0 + slot * CARDIMG_TILES) * 16;
    const u32 *src = cardimg_tiles[card_id];
    for (int i = 0; i < CARDIMG_TILES * 8; i++) {
        dst[i * 2]     = src[i] & 0xFFFF;
        dst[i * 2 + 1] = src[i] >> 16;
    }
    for (int c = 0; c < 12; c++)
        MEM_PAL_BG[face_bank[slot] * 16 + 4 + c] = cardimg_pal[card_id][c];
}

void card_face_stamp(int slot, int tx, int ty)
{
    int t = CARDFACE_TILE0 + slot * CARDIMG_TILES;
    for (int j = 0; j < CARDIMG_TH; j++)
        for (int i = 0; i < CARDIMG_TW; i++)
            bg2_stamp(tx + i, ty + j, t++, face_bank[slot], 0);
}

/* ---- battle HUD elements + relic icons (hand zone / top bar) ----
   tiles in the charblock-1 gap below the card faces, banks 4 + 5 */
#define HUDIMG_DATA
#include "hudimg.h"
#define HUD_TILE0   63
#define RELIC_TILE0 (HUD_TILE0 + HUDIMG_NTILES)
#define BANK_HUD    4
#define BANK_RELIC  5

void hud_load(void)
{
    vu16 *dst = CHARBLOCK(1) + HUD_TILE0 * 16;
    for (int i = 0; i < HUDIMG_NTILES * 8; i++) {
        dst[i * 2]     = hudimg_tiles[i] & 0xFFFF;
        dst[i * 2 + 1] = hudimg_tiles[i] >> 16;
    }
    dst = CHARBLOCK(1) + RELIC_TILE0 * 16;
    for (int i = 0; i < RELICIMG_NTILES * 8; i++) {
        dst[i * 2]     = relicimg_tiles[i] & 0xFFFF;
        dst[i * 2 + 1] = relicimg_tiles[i] >> 16;
    }
    for (int c = 0; c < 12; c++) {
        MEM_PAL_BG[BANK_HUD * 16 + 4 + c]   = hudimg_pal[c];
        MEM_PAL_BG[BANK_RELIC * 16 + 4 + c] = relicimg_pal[c];
    }
}

void hud_stamp(int elem, int tx, int ty)
{
    int t = HUD_TILE0 + hudimg_elem[elem].ofs;
    for (int j = 0; j < hudimg_elem[elem].th; j++)
        for (int i = 0; i < hudimg_elem[elem].tw; i++)
            bg2_stamp(tx + i, ty + j, t++, BANK_HUD, 0);
}

void relic_stamp(int relic, int tx, int ty)
{
    int t = RELIC_TILE0 + relic * 4;
    bg2_stamp(tx,     ty,     t,     BANK_RELIC, 0);
    bg2_stamp(tx + 1, ty,     t + 1, BANK_RELIC, 0);
    bg2_stamp(tx,     ty + 1, t + 2, BANK_RELIC, 0);
    bg2_stamp(tx + 1, ty + 1, t + 3, BANK_RELIC, 0);
}

void ui_icon(int x, int y, int icon)   /* icon = TI_* absolute tile index */
{
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    SCREENBLOCK(SB_UI)[y * 32 + x] =
        (u16)(icon | (icon_bank[icon - ICON_BASE] << 12));
}

/* ---- 2x text: synthesize scaled glyphs into cb0 tiles 200+ ----
   each 2x glyph = 4 tiles; ~77 slots free above icons */
#define BIG_BASE 200
static u8 big_cached[128];
static int big_next = 1;   /* slot 0 reserved so cache 0 = miss */

void txt2x_reset(void)     /* call after anything clobbers charblock 0 */
{
    for (int i = 0; i < 128; i++) big_cached[i] = 0;
    big_next = 1;
}

static int big_glyph(char c)
{
    u8 uc = (u8)c & 0x7F;
    if (big_cached[uc]) return big_cached[uc];
    int slot = big_next++;
    /* expand font glyph 2x into 4 tiles (TL TR BL BR) */
    for (int q = 0; q < 4; q++) {
        vu16 *dst = CHARBLOCK(0) + (BIG_BASE + slot * 4 + q) * 16;
        int oy = (q >> 1) * 4, ox = (q & 1) * 4;
        for (int y = 0; y < 8; y++) {
            u8 row = font8x8_basic[uc][oy + y / 2];
            u32 line = 0;
            for (int x = 0; x < 8; x++)
                if (row & (1 << (ox + x / 2))) line |= 1u << (x * 4);
            dst[y * 2]     = line & 0xFFFF;
            dst[y * 2 + 1] = line >> 16;
        }
    }
    big_cached[uc] = (u8)slot;
    return slot;
}

static void txt_raw(int x, int y, int tile, int clr)
{
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    SCREENBLOCK(SB_TXT)[y * 32 + x] = (u16)(tile | (clr << 12));
}

void txt_put2x(int x, int y, const char *s, int clr)
{
    for (; *s; s++, x += 2) {
        if (*s == ' ') continue;
        int base = BIG_BASE + big_glyph(*s) * 4;
        txt_raw(x,     y,     base,     clr);
        txt_raw(x + 1, y,     base + 1, clr);
        txt_raw(x,     y + 1, base + 2, clr);
        txt_raw(x + 1, y + 1, base + 3, clr);
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
u32 rng_state_get(void) { return rng_state; }

/* ---- sfx: psg square ch1 (hardware sweep) + noise ch4 ---- */
u8 opt_music = 1, opt_sfx = 1;   /* settings menu toggles */

void sfx_init(void)
{
    REG_SOUNDCNT_X = 0x0080;   /* master on */
    REG_SOUNDCNT_L = 0xFF77;   /* full vol, all channels L+R */
    /* psg 100% + DS A 100% L+R timer0 (music) + DS B 100% L+R timer1 (sfx) */
    REG_SOUNDCNT_H = 0x0002 | 0x0004 | 0x0300 | 0x0008 | 0x3000 | 0x4000;
}

/* ch1 pulse. sweep = raw SOUND1CNT_L: (time<<4)|(down<<3)|shift —
   shift n steps freq by f/2^n every time*7.8ms; 0 = no sweep.
   duty 0-3 = 12.5/25/50/75%. len in 1/256s units (1..63). */
static void ch1(int vol, int envstep, int duty, int len, int sweep, int freq)
{
    REG_SOUND1CNT_L = (u16)sweep;
    REG_SOUND1CNT_H = (u16)((vol << 12) | (envstep << 8) | (duty << 6) |
                            (64 - len));
    REG_SOUND1CNT_X = (u16)(0x8000 | 0x4000 | (2048 - 131072 / freq));
}

void sfx_blip(void)  { /* navigation is silent (nav beep removed) */ }
/* confirm: rising chirp */
void sfx_ok(void)    { if (!opt_sfx) return; ch1(11, 2, 1, 22, 0x23, 660); }
/* cancel/error: falling womp */
void sfx_bad(void)   { if (!opt_sfx) return; ch1(12, 3, 2, 30, 0x3B, 330); }
/* sampled families route to PCM samples on FIFO B (see tools/mksfx.py);
   hit alternates thud/slash for variety */
void sfx_heal(void)  { if (!opt_sfx) return; sfxpcm_play(SFXP_HEAL); }
void sfx_card(void)  { if (!opt_sfx) return; sfxpcm_play(SFXP_CARD); }
void sfx_block(void) { if (!opt_sfx) return; sfxpcm_play(SFXP_CLANG); }
void sfx_hit(void)
{
    static u8 alt;
    if (!opt_sfx) return;
    sfxpcm_play((alt ^= 1) ? SFXP_HIT : SFXP_SLASH);
}
void sfx_coin(void)  { if (!opt_sfx) return; sfxpcm_play(SFXP_COIN); }

/* ---- screen transition: hardware fade-to-black + mosaic "zoom" ----
   fx_out() darkens+pixelates the current screen (map) as it leaves; the
   destination composes under black and calls fx_reveal() to fade back in.
   Both use BLDY (brightness-down on every layer) + BG mosaic — content-
   agnostic, so one routine works from any screen. fx_pending gates reveal
   so screens NOT reached via a faded exit don't fade. */
#define FX_FRAMES     10
#define FX_MOSAIC_MAX 6
static u8 fx_pending;

static void fx_step(int y, int m)
{
    REG_BLDY = (u16)y;
    REG_MOSAIC = (u16)((m << 4) | m);   /* BG mosaic H|V (low byte) */
}

static void fx_blend_on(void)
{
    REG_BG0CNT |= BG_MOSAIC;
    REG_BG1CNT |= BG_MOSAIC;
    REG_BG2CNT |= BG_MOSAIC;
    REG_BLDCNT = 0x3F | (3 << 6);       /* all layers 1st target, brightness-down (to black) */
}

void fx_out(void)
{
    fx_blend_on();
    for (int f = 1; f <= FX_FRAMES; f++) {
        fx_step(f * 16 / FX_FRAMES, f * FX_MOSAIC_MAX / FX_FRAMES);
        vsync();
    }
    fx_step(16, FX_MOSAIC_MAX);         /* hold full black */
    fx_pending = 1;
}

void fx_reveal(void)
{
    if (!fx_pending) return;
    fx_pending = 0;
    fx_blend_on();
    for (int f = FX_FRAMES; f >= 0; f--) {
        fx_step(f * 16 / FX_FRAMES, f * FX_MOSAIC_MAX / FX_FRAMES);
        vsync();
    }
    REG_BLDCNT = 0;
    REG_MOSAIC = 0;
    REG_BG0CNT &= ~BG_MOSAIC;
    REG_BG1CNT &= ~BG_MOSAIC;
    REG_BG2CNT &= ~BG_MOSAIC;
}
