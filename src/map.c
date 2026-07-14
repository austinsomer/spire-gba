#include "cards.h"
#include "bgtiles.h"

/* 7 cols x 15 floors, StS act 1 style.
   Rendered as a 512px-tall scrolling BG2 map: sage stone backdrop,
   16x16 bone icons, dashed path lines (solid + dim once traveled). */
#define MCOLS 7
#define MFLOORS 15

typedef struct { u8 room; u8 edges; } MapNode;
/* edges: bit0 = up-left, bit1 = up, bit2 = up-right (to floor+1) */

static MapNode mnode[MFLOORS][MCOLS];
static u8 path_taken[MFLOORS];      /* col chosen per completed floor */
s8 map_pending_encounter;
int reward_elite;

/* node centers on the 256x512 bg (icon = 16x16 around center) */
#define NPX(c) (24 + (c) * 32)
#define NPY(f) (480 - (f) * 32)
#define BOSS_Y 12
#define CAM_MAX 352                  /* 512 - 160 */

static u8 roll_room(int floor)
{
    if (floor == 0) return ROOM_MONSTER;
    if (floor == 8) return ROOM_TREASURE;
    if (floor == 14) return ROOM_REST;
    u32 r = rng() % 100;
    if (floor >= 5 && r < 14) return ROOM_ELITE;
    if (floor >= 5 && r < 24) return ROOM_REST;
    if (r < 29) return ROOM_SHOP;
    if (r < 51) return ROOM_EVENT;
    return ROOM_MONSTER;
}

/* ---- map balance: StS-style pacing constraints ----
   The random walks assign types with no spacing rules, which clumps (back-to-
   back shops, adjacent elites, no rest before the boss).  balance_map() runs
   after the walks: it locks the fixed floors, rerolls nodes that violate the
   pacing rules until clean or a retry cap, then enforces per-act caps.
   MONSTER is exempt from every rule and is the universal fallback, so the
   pass always terminates on a valid, playable map. */
#define MAX_ELITES      2
#define MAX_SHOPS       3
#define MIN_ELITE_FLOOR 5
#define MIN_REST_FLOOR  5

static int node_exists(int f, int c)
{
    return f >= 0 && f < MFLOORS && c >= 0 && c < MCOLS &&
           mnode[f][c].room != ROOM_NONE;
}

/* is there an upward edge from (f,c) to (f+1, nc)? */
static int has_up_edge(int f, int c, int nc)
{
    int d = nc - c;
    if (f < 0 || f >= MFLOORS - 1 || c < 0 || c >= MCOLS) return 0;
    if (d < -1 || d > 1) return 0;
    return (mnode[f][c].edges >> (d + 1)) & 1;
}

/* is (f-1, pc) a parent (path predecessor) of (f, c)? */
static int is_parent(int f, int c, int pc)
{
    return node_exists(f - 1, pc) && has_up_edge(f - 1, pc, c);
}

/* does any parent of (f,c) have room type R? */
static int parent_has(int f, int c, u8 room)
{
    for (int pc = c - 1; pc <= c + 1; pc++)
        if (is_parent(f, c, pc) && mnode[f - 1][pc].room == room) return 1;
    return 0;
}

/* is there a same-type run of 3 ending at (f,c)? (a parent of R that itself
   has a parent of R) */
static int triple_run(int f, int c)
{
    u8 R = mnode[f][c].room;
    for (int pc = c - 1; pc <= c + 1; pc++) {
        if (!is_parent(f, c, pc) || mnode[f - 1][pc].room != R) continue;
        if (parent_has(f - 1, pc, R)) return 1;
    }
    return 0;
}

static int floor_locked(int f)   /* fixed floors are never rerolled */
{
    return f == 0 || f == 1 || f == 8 || f == MFLOORS - 1;
}

/* does the node break a rerollable pacing rule, given its neighbours? */
static int node_bad(int f, int c)
{
    u8 R = mnode[f][c].room;
    if (R == ROOM_SHOP  && parent_has(f, c, ROOM_SHOP))  return 1; /* back-to-back shop */
    if (R == ROOM_ELITE && parent_has(f, c, ROOM_ELITE)) return 1; /* adjacent elites */
    if (R == ROOM_ELITE && f < MIN_ELITE_FLOOR) return 1;
    if (R == ROOM_REST  && f < MIN_REST_FLOOR)  return 1;          /* rest too early */
    /* floor 14 is a locked (forced) rest, so a rest run into it isn't caught by
       triple_run (that node is never rerolled); break it a step earlier. */
    if (R == ROOM_REST && f == MFLOORS - 2 && parent_has(f, c, ROOM_REST)) return 1;
    if (R != ROOM_MONSTER && triple_run(f, c))  return 1;          /* no type 3x on a path */
    return 0;
}

static void balance_map(void)
{
    /* StS opener: floors 0 and 1 are always plain combat. */
    for (int c = 0; c < MCOLS; c++) {
        if (node_exists(0, c)) mnode[0][c].room = ROOM_MONSTER;
        if (node_exists(1, c)) mnode[1][c].room = ROOM_MONSTER;
    }

    /* reroll local violators, preferring a fresh valid roll, else MONSTER. */
    for (int pass = 0; pass < 80; pass++) {
        int changed = 0;
        for (int f = 2; f < MFLOORS - 1; f++) {
            if (floor_locked(f)) continue;
            for (int c = 0; c < MCOLS; c++) {
                if (!node_exists(f, c) || !node_bad(f, c)) continue;
                u8 pick = ROOM_MONSTER;
                for (int t = 0; t < 6; t++) {
                    mnode[f][c].room = roll_room(f);
                    if (!node_bad(f, c)) { pick = mnode[f][c].room; break; }
                }
                mnode[f][c].room = pick;
                changed = 1;
            }
        }
        if (!changed) break;
    }

    /* guarantee: force any residual violator to plain combat.  Ascending so a
       demoted node can never leave a child violating (MONSTER parents break
       every chain and every back-to-back). */
    for (int f = 2; f < MFLOORS - 1; f++) {
        if (floor_locked(f)) continue;
        for (int c = 0; c < MCOLS; c++)
            if (node_exists(f, c) && node_bad(f, c)) mnode[f][c].room = ROOM_MONSTER;
    }

    /* per-act caps: demote excess elites / shops to plain combat (bottom-up,
       so the earliest of each survive).  Demotion to MONSTER cannot introduce
       a new violation. */
    int elites = 0, shops = 0;
    for (int f = 2; f < MFLOORS - 1; f++) {
        if (floor_locked(f)) continue;
        for (int c = 0; c < MCOLS; c++) {
            if (!node_exists(f, c)) continue;
            if (mnode[f][c].room == ROOM_ELITE) {
                if (++elites > MAX_ELITES) mnode[f][c].room = ROOM_MONSTER;
            } else if (mnode[f][c].room == ROOM_SHOP) {
                if (++shops > MAX_SHOPS) mnode[f][c].room = ROOM_MONSTER;
            }
        }
    }
}

void map_generate(void)
{
    for (int f = 0; f < MFLOORS; f++)
        for (int c = 0; c < MCOLS; c++)
            mnode[f][c] = (MapNode){ROOM_NONE, 0};
    for (int f = 0; f < MFLOORS; f++) path_taken[f] = 0xFF;

    for (int w = 0; w < 4; w++) {
        int c = 1 + rng_range(MCOLS - 2);
        for (int f = 0; f < MFLOORS; f++) {
            if (mnode[f][c].room == ROOM_NONE)
                mnode[f][c].room = roll_room(f);
            if (f == MFLOORS - 1) break;
            int d = (int)rng_range(3) - 1;
            int nc = c + d;
            if (nc < 0) { nc = 0; d = c ? -1 : 0; }
            if (nc >= MCOLS) { nc = MCOLS - 1; d = 0; }
            d = nc - c;
            mnode[f][c].edges |= 1 << (d + 1);
            c = nc;
        }
    }
    balance_map();
}

int map_current_room(void)
{
    return mnode[run.floor][run.mapcol].room;
}

/* save blob: raw node bytes + path_taken (see MAP_BLOB_SIZE) */
int map_export(u8 *dst)
{
    int n = 0;
    for (int f = 0; f < MFLOORS; f++)
        for (int c = 0; c < MCOLS; c++) {
            dst[n++] = mnode[f][c].room;
            dst[n++] = mnode[f][c].edges;
        }
    for (int f = 0; f < MFLOORS; f++) dst[n++] = path_taken[f];
    return n;
}

void map_import(const u8 *src)
{
    int n = 0;
    for (int f = 0; f < MFLOORS; f++)
        for (int c = 0; c < MCOLS; c++) {
            mnode[f][c].room  = src[n++];
            mnode[f][c].edges = src[n++];
        }
    for (int f = 0; f < MFLOORS; f++) path_taken[f] = src[n++];
}

static int pick_monster(void)
{
    if (run.floor < 3) return rng_range(4);
    return rng_range(7);
}
static int pick_elite(void) { return 7 + rng_range(3); }

static u16 room_icon16(int room)   /* TL tile of 2x2 icon */
{
    switch (room) {
    case ROOM_MONSTER:  return TB_M_SKULL;
    case ROOM_ELITE:    return TB_M_ELITE;
    case ROOM_EVENT:    return TB_M_QUESTION;
    case ROOM_REST:     return TB_M_FIRE;
    case ROOM_SHOP:     return TB_M_COIN;
    case ROOM_TREASURE: return TB_M_CHEST;
    default:            return TB_M_BOSS;
    }
}

static void route_room(int room)
{
    switch (room) {
    case ROOM_MONSTER:
        map_pending_encounter = pick_monster();
        reward_elite = 0; gstate = ST_COMBAT; break;
    case ROOM_ELITE:
        map_pending_encounter = pick_elite();
        reward_elite = 1; gstate = ST_COMBAT; break;
    case ROOM_BOSS:
        map_pending_encounter = 10 + run.act_boss;
        reward_elite = 1; gstate = ST_COMBAT; break;
    case ROOM_EVENT:    gstate = ST_EVENT; break;
    case ROOM_REST:     gstate = ST_REST; break;
    case ROOM_SHOP:     gstate = ST_SHOP; break;
    case ROOM_TREASURE: gstate = ST_TREASURE; break;
    }
}

static int reachable(int f, int c)
{
    if (mnode[f][c].room == ROOM_NONE) return 0;
    if (f == 0) return 1;
    if ((int)run.floor != f) return 0;
    int pc = run.mapcol, d = c - pc;
    if (d < -1 || d > 1) return 0;
    return (mnode[f - 1][pc].edges >> (d + 1)) & 1;
}

/* ---------- runtime path-line + cursor tiles (charblock 1) ----------
   bank 14 = dimmed copy of bank 13 (built here); bank 15 = line colors:
   1 dark-olive line, 2 cream (cursor). Patterns: 6x6-tile scratch,
   line between node centers local (8,40) -> (8+dx,8), margins 11px. */

#define PT_BASE   N_BGTILES              /* pattern tiles start here */
#define PT_TILES  36                     /* 6x6 per pattern */
enum { PAT_V_DASH, PAT_D_DASH, PAT_V_SOLID, PAT_D_SOLID, N_PATS };
#define CURSOR_TILE (PT_BASE + N_PATS * PT_TILES)
#define DOT_TILE    (CURSOR_TILE + 1)    /* reachable-node marker */

static u8 pat_used[N_PATS][6];           /* col bitmask per row */
static int map_tiles_ready;

static void px_set(u32 *scratch, int x, int y)
{
    if (x < 0 || x >= 48 || y < 0 || y >= 48) return;
    /* scratch = 36 tiles x 8 words */
    int tile = (y >> 3) * 6 + (x >> 3);
    scratch[tile * 8 + (y & 7)] |= 1u << ((x & 7) * 4);
}

static void gen_pattern(int pat, int dx, int solid)
{
    static u32 scratch[PT_TILES * 8];
    static u8 hit[PT_TILES];
    for (int i = 0; i < PT_TILES * 8; i++) scratch[i] = 0;
    for (int i = 0; i < PT_TILES; i++) hit[i] = 0;

    /* full center-to-center line: lower node (8,40) -> upper (8+dx,8),
       clipped where within the 16x16 icon boxes; dashes 4on/4off */
    int x0 = 8, y0 = 40, x1 = 8 + dx, y1 = 8;
    int steps = 48;
    for (int s = 0; s <= steps; s++) {
        if (!solid && ((s / 6) & 1)) continue;
        int x = x0 + (x1 - x0) * s / steps;
        int y = y0 + (y1 - y0) * s / steps;
        int in0 = (x - x0 >= -9 && x - x0 <= 9 && y0 - y <= 9 && y0 - y >= -9);
        int in1 = (x - x1 >= -9 && x - x1 <= 9 && y - y1 <= 9 && y - y1 >= -9);
        if (in0 || in1) continue;
        px_set(scratch, x, y);     px_set(scratch, x + 1, y);
        px_set(scratch, x, y + 1); px_set(scratch, x + 1, y + 1);
        if (x >= 0 && x < 48 && y >= 0 && y < 48)
            hit[(y >> 3) * 6 + (x >> 3)] = 1;
        if (x + 1 < 48 && y + 1 < 48)
            hit[((y + 1) >> 3) * 6 + ((x + 1) >> 3)] = 1;
    }
    /* upload touched tiles (backfilled w/ sage, color idx 3), record mask */
    for (int r = 0; r < 6; r++) pat_used[pat][r] = 0;
    for (int t = 0; t < PT_TILES; t++) {
        if (!hit[t]) continue;
        pat_used[pat][t / 6] |= 1 << (t % 6);
        vu16 *dst = CHARBLOCK(1) + (PT_BASE + pat * PT_TILES + t) * 16;
        for (int i = 0; i < 8; i++) {
            u32 w = scratch[t * 8 + i];
            u32 filled = 0;
            for (int x = 0; x < 8; x++) {
                u32 nib = (w >> (x * 4)) & 0xF;
                filled |= (nib ? nib : 3u) << (x * 4);
            }
            dst[i * 2]     = filled & 0xFFFF;
            dst[i * 2 + 1] = filled >> 16;
        }
    }
}

static void gen_map_tiles(void)
{
    /* bank 15: line + cursor colors over sage backfill */
    MEM_PAL_BG[15 * 16 + 1] = RGB15(7, 8, 5);     /* dark olive line */
    MEM_PAL_BG[15 * 16 + 2] = RGB15(29, 29, 25);  /* cream cursor */
    MEM_PAL_BG[15 * 16 + 3] = RGB15(17, 19, 15);  /* sage base backfill */
    /* bank 14: dimmed copy of bank 13 (blend toward sage) */
    for (int c = 0; c < 16; c++) {
        u16 v = MEM_PAL_BG[13 * 16 + c];
        int r = (v & 31), g = (v >> 5) & 31, b = (v >> 10) & 31;
        r = (r + 14) / 2; g = (g + 16) / 2; b = (b + 12) / 2;
        MEM_PAL_BG[14 * 16 + c] = (u16)(r | (g << 5) | (b << 10));
    }
    gen_pattern(PAT_V_DASH, 0, 0);
    gen_pattern(PAT_D_DASH, 32, 0);
    gen_pattern(PAT_V_SOLID, 0, 1);
    gen_pattern(PAT_D_SOLID, 32, 1);
    /* cursor: right-pointing arrow, cream (color 2) */
    {
        static const u8 arrow[8] = {0x08,0x18,0x38,0x78,0x78,0x38,0x18,0x08};
        vu16 *dst = CHARBLOCK(1) + CURSOR_TILE * 16;
        for (int y = 0; y < 8; y++) {
            u32 line = 0;
            for (int x = 0; x < 8; x++)
                line |= (u32)((arrow[y] & (1 << x)) ? 2 : 3) << (x * 4);
            dst[y * 2] = line & 0xFFFF; dst[y * 2 + 1] = line >> 16;
        }
    }
    /* reachable-node marker: small cream square w/ dark outline */
    {
        vu16 *dst = CHARBLOCK(1) + DOT_TILE * 16;
        for (int y = 0; y < 8; y++) {
            u32 line = 0;
            for (int x = 0; x < 8; x++) {
                int px = 3;
                if (x >= 2 && x <= 5 && y >= 2 && y <= 5)
                    px = (x == 2 || x == 5 || y == 2 || y == 5) ? 1 : 2;
                line |= (u32)px << (x * 4);
            }
            dst[y * 2] = line & 0xFFFF; dst[y * 2 + 1] = line >> 16;
        }
    }
    map_tiles_ready = 1;
}

/* stamp a pattern anchored at world tile (ax,ay) */
static void stamp_pattern(int pat, int ax, int ay, int hflip)
{
    for (int r = 0; r < 6; r++)
        for (int cc = 0; cc < 6; cc++)
            if (pat_used[pat][r] & (1 << cc))
                bg2_stamp(ax + (hflip ? 5 - cc : cc), ay + r,
                          PT_BASE + pat * PT_TILES + r * 6 + cc, 15, hflip);
}

/* edge (c,f) -> (c+d,f+1): anchor = tile of local(0,0):
   world px anchor = (NPX(c)-8 [minus 32 if d<0], NPY(f)-40) */
static void draw_edge(int c, int f, int d, int solid)
{
    int pat = (d == 0) ? (solid ? PAT_V_SOLID : PAT_V_DASH)
                       : (solid ? PAT_D_SOLID : PAT_D_DASH);
    int ax_px = NPX(c) - 8 + (d < 0 ? -32 : 0);
    int ay_px = NPY(f) - 40;
    stamp_pattern(pat, ax_px >> 3, ay_px >> 3, d < 0);
}

static void icon16(int cx_px, int cy_px, int tl, int bank)
{
    int tx = (cx_px - 8) >> 3, ty = (cy_px - 8) >> 3;
    bg2_stamp(tx,     ty,     tl,     bank, 0);
    bg2_stamp(tx + 1, ty,     tl + 1, bank, 0);
    bg2_stamp(tx,     ty + 1, tl + 2, bank, 0);
    bg2_stamp(tx + 1, ty + 1, tl + 3, bank, 0);
}

/* mottled sage backdrop tile for cell (x,y) — also used to restore the
   cell under the cursor without recomposing the whole map */
static void sage_cell(int x, int y)
{
    u32 h = (u32)(x * 2654435761u + y * 40503u);
    h ^= h >> 13;
    int t = TB_SAGE;
    if ((h & 15) == 0) t = TB_SAGELIGHT;
    else if ((h & 15) == 1) t = TB_SAGEDARK;
    bg2_tile(x, y, t);
}

static void compose_map(void)
{
    if (!map_tiles_ready) gen_map_tiles();
    bg2_clear();
    /* sage backdrop with mottled patches, full 64 rows */
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 32; x++)
            sage_cell(x, y);
    /* edges */
    for (int f = 0; f < MFLOORS - 1; f++)
        for (int c = 0; c < MCOLS; c++) {
            if (mnode[f][c].room == ROOM_NONE) continue;
            for (int d = -1; d <= 1; d++) {
                if (!((mnode[f][c].edges >> (d + 1)) & 1)) continue;
                /* solid once traveled: both endpoints on the recorded path */
                int taken = (path_taken[f] == c &&
                             path_taken[f + 1] != 0xFF &&
                             path_taken[f + 1] == (u8)(c + d));
                draw_edge(c, f, d, taken);
            }
        }
    /* top floor -> boss (vertical dashes from each floor-14 node) */
    for (int c = 0; c < MCOLS; c++)
        if (mnode[MFLOORS - 1][c].room != ROOM_NONE) {
            /* fake edge toward boss column 3 as dashes going up */
            int d = (c < 3) ? 1 : (c > 3) ? -1 : 0;
            int solid = ((int)run.floor >= MFLOORS &&
                         path_taken[MFLOORS - 1] == c);
            if (d == 0) draw_edge(c, MFLOORS - 1, 0, solid);
            else        draw_edge(c, MFLOORS - 1, d, solid);
        }
    /* nodes */
    for (int f = 0; f < MFLOORS; f++)
        for (int c = 0; c < MCOLS; c++) {
            if (mnode[f][c].room == ROOM_NONE) continue;
            int visited = (f < (int)run.floor);
            icon16(NPX(c), NPY(f), room_icon16(mnode[f][c].room),
                   visited ? 14 : 13);
        }
    /* boss icon at top center */
    icon16(NPX(3), BOSS_Y + 4, TB_M_BOSS,
           ((int)run.floor >= MFLOORS) ? 13 : 13);
}

static void banner(void)
{
    txt_clear(); ui_clear();
    ui_fill(0, 0, 30, 1, T_PANEL, CLR_GRAY);
    txt_put(1, 0, "FLOOR", CLR_WHITE);
    txt_int(7, 0, run.floor + 1, CLR_WHITE);
    txt_put(10, 0, "ACT 1: THE EXORDIUM", CLR_GRAY);
    /* bottom strip: hp / gold / hint */
    ui_fill(0, 19, 30, 1, T_PANEL, CLR_GRAY);
    ui_tile(0, 19, T_HEART, CLR_RED);
    int xx = txt_int_at(2, 19, run.hp, CLR_WHITE);
    txt_putc(xx, 19, '/', CLR_GRAY); txt_int(xx + 1, 19, run.maxhp, CLR_GRAY);
    ui_tile(11, 19, T_GOLDPT, CLR_YELLOW);
    txt_int(13, 19, run.gold, CLR_YELLOW);
    txt_put(19, 19, "SELECT:DECK", CLR_GRAY);
}

void map_screen(void)
{
    /* mode-4 title clobbers charblock 1 — always rebuild pattern tiles */
    map_tiles_ready = 0;
#ifdef MAPTEST
    /* pre-walk 6 floors so visited/solid-path rendering is inspectable */
    if (run.floor == 0)
        for (int f = 0; f < 6; f++)
            for (int c = 0; c < MCOLS; c++)
                if (reachable(f, c)) {
                    run.mapcol = c; path_taken[f] = c; run.floor++;
                    break;
                }
#endif
    /* boss floor: everything traveled, prompt at the top of the map */
    int boss_mode = (run.floor >= MFLOORS);

    int sel = -1;
    if (!boss_mode) {
        for (int c = 0; c < MCOLS && sel < 0; c++)
            if (reachable(run.floor, c)) sel = c;
        if (sel < 0) {
            mnode[run.floor][run.mapcol].room = ROOM_MONSTER;
            sel = run.mapcol;
        }
    }

    for (;;) {
        compose_map();
        banner();
        if (boss_mode) txt_put(9, 1, "THE BOSS AWAITS", CLR_DKRED);
        /* steady markers beside every reachable node this floor */
        if (!boss_mode)
            for (int c = 0; c < MCOLS; c++)
                if (reachable(run.floor, c))
                    bg2_stamp((NPX(c) - 16) >> 3, (NPY(run.floor) - 4) >> 3,
                              DOT_TILE, 15, 0);

        /* camera centered on current row (or boss) */
        int target_y = boss_mode ? BOSS_Y + 4 : NPY(run.floor);
        int cam = target_y - 80;
        if (cam < 0) cam = 0;
        if (cam > CAM_MAX) cam = CAM_MAX;
        int pan = cam;

        int blink = 0;
        for (;;) {
            vsync(); key_poll();
            blink++;
            bg2_scroll(pan);
            /* blinking cursor left of selected node (bg tile stamp) */
            int ctx = ((boss_mode ? NPX(3) : NPX(sel)) - 16) >> 3;
            int cty = ((boss_mode ? BOSS_Y + 4 : NPY(run.floor)) - 4) >> 3;
            if (blink & 16)      bg2_stamp(ctx, cty, CURSOR_TILE, 15, 0);
            else if (boss_mode)  sage_cell(ctx, cty);
            else                 bg2_stamp(ctx, cty, DOT_TILE, 15, 0);
            if (key_repeat(KEY_UP) && pan > 0)        { pan -= 8; continue; }
            if (key_repeat(KEY_DOWN) && pan < CAM_MAX){ pan += 8; continue; }
            /* L/R move the cursor in place — recomposing the whole map
               here blanked BG2 for a frame and flickered */
            if (!boss_mode && key_repeat(KEY_LEFT)) {
                for (int c = sel - 1; c >= 0; c--)
                    if (reachable(run.floor, c)) {
                        bg2_stamp(ctx, cty, DOT_TILE, 15, 0);  /* old sel */
                        sel = c; blink = 16; sfx_blip(); break;
                    }
                continue;
            }
            if (!boss_mode && key_repeat(KEY_RIGHT)) {
                for (int c = sel + 1; c < MCOLS; c++)
                    if (reachable(run.floor, c)) {
                        bg2_stamp(ctx, cty, DOT_TILE, 15, 0);  /* old sel */
                        sel = c; blink = 16; sfx_blip(); break;
                    }
                continue;
            }
            if (key_hit(KEY_SELECT)) {
                obj_hide_all();
                deck_browse("DECK", 0);
                break;   /* recompose */
            }
            if (key_hit(KEY_START)) {
                obj_hide_all();
                if (pause_menu()) { bg2_scroll(0); return; }  /* SAVE & QUIT */
                map_tiles_ready = 0;   /* relic_view's hud_load clobbered cb1 63..151 */
                break;   /* recompose rebuilds pattern tiles */
            }
            if (key_hit(KEY_A)) {
                sfx_ok();
                fx_out();               /* encounter transition: fade current map out */
                bg2_scroll(0);
                if (boss_mode) { route_room(ROOM_BOSS); return; }
                run.mapcol = sel;
                path_taken[run.floor] = sel;
                int room = mnode[run.floor][sel].room;
                run.floor++;
                route_room(room);
                return;
            }
        }
    }
}
