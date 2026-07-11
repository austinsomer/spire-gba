#include "cards.h"

/* 7 cols x 15 floors, StS act 1 style */
#define MCOLS 7
#define MFLOORS 15

typedef struct { u8 room; u8 edges; } MapNode;
/* edges: bit0 = up-left, bit1 = up, bit2 = up-right (to floor+1) */

static MapNode mnode[MFLOORS][MCOLS];
s8 map_pending_encounter;
int reward_elite;

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

void map_generate(void)
{
    for (int f = 0; f < MFLOORS; f++)
        for (int c = 0; c < MCOLS; c++)
            mnode[f][c] = (MapNode){ROOM_NONE, 0};

    /* 4 random walks bottom to top */
    for (int w = 0; w < 4; w++) {
        int c = 1 + rng_range(MCOLS - 2);
        for (int f = 0; f < MFLOORS; f++) {
            if (mnode[f][c].room == ROOM_NONE)
                mnode[f][c].room = roll_room(f);
            if (f == MFLOORS - 1) break;
            int d = (int)rng_range(3) - 1;          /* -1,0,1 */
            int nc = c + d;
            if (nc < 0) { nc = 0; d = c ? -1 : 0; }
            if (nc >= MCOLS) { nc = MCOLS - 1; d = 0; }
            d = nc - c;
            mnode[f][c].edges |= 1 << (d + 1);
            c = nc;
        }
    }
}

int map_current_room(void)
{
    return mnode[run.floor][run.mapcol].room;
}

/* encounter picks (ids match combat.c enum) */
static int pick_monster(void)
{
    /* early floors easy pool */
    if (run.floor < 3) return rng_range(4);        /* cultist/jaw/louses/slimes */
    return rng_range(7);
}
static int pick_elite(void) { return 7 + rng_range(3); }

static const char room_ch[] = {' ', 'M', 'E', '?', 'R', '$', 'T', 'B'};
static const int  room_clr[] = {CLR_GRAY, CLR_WHITE, CLR_RED, CLR_CYAN,
                                CLR_ORANGE, CLR_YELLOW, CLR_GREEN, CLR_DKRED};

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

/* reachable on given floor from (run.floor-1, run.mapcol)? */
static int reachable(int f, int c)
{
    if (mnode[f][c].room == ROOM_NONE) return 0;
    if (f == 0) return 1;
    if ((int)run.floor != f) return 0;   /* only current row selectable */
    if (run.floor == 0) return 1;
    /* check edge from previous node */
    int pc = run.mapcol, d = c - pc;
    if (d < -1 || d > 1) return 0;
    return (mnode[f - 1][pc].edges >> (d + 1)) & 1;
}

void map_screen(void)
{
    /* boss floor */
    if (run.floor >= MFLOORS) {
        txt_clear(); ui_clear();
        ui_box(4, 6, 22, 6, CLR_DKRED);
        txt_put(9, 8, "BOSS AHEAD", CLR_RED);
        txt_put(7, 9, "PRESS A TO FIGHT", CLR_WHITE);
        for (;;) {
            vsync(); key_poll();
            if (key_hit(KEY_A)) { sfx_ok(); route_room(ROOM_BOSS); return; }
            if (key_hit(KEY_SELECT)) deck_browse("DECK", 0);
        }
    }

    int sel = -1;
    for (int c = 0; c < MCOLS && sel < 0; c++)
        if (reachable(run.floor, c)) sel = c;
    if (sel < 0) { /* dead path shouldn't happen; fail safe to monster */
        mnode[run.floor][run.mapcol].room = ROOM_MONSTER;
        sel = run.mapcol;
    }

    for (;;) {
        txt_clear(); ui_clear();
        /* header */
        ui_tile(0, 0, T_HEART, CLR_RED);
        int xx = txt_int_at(2, 0, run.hp, CLR_WHITE);
        txt_putc(xx, 0, '/', CLR_GRAY); txt_int(xx + 1, 0, run.maxhp, CLR_GRAY);
        ui_tile(12, 0, T_GOLDPT, CLR_YELLOW);
        txt_int(14, 0, run.gold, CLR_YELLOW);
        txt_put(20, 0, "FLR", CLR_GRAY); txt_int(24, 0, run.floor + 1, CLR_WHITE);

        txt_put(12, 1, "BOSS", CLR_DKRED);
        /* nodes: floor 14 top y=2 .. floor 0 y=16 */
        for (int f = 0; f < MFLOORS; f++) {
            int y = 16 - f;
            for (int c = 0; c < MCOLS; c++) {
                if (mnode[f][c].room == ROOM_NONE) continue;
                int x = 1 + c * 4;
                int done = (f < run.floor);
                int here = (f == run.floor && reachable(f, c));
                int clr = done ? CLR_GRAY : room_clr[mnode[f][c].room];
                if (here && c == sel) {
                    ui_fill(x - 1, y, 3, 1, T_PANEL, CLR_BLUE);
                    txt_putc(x - 1, y, '>', CLR_YELLOW);
                }
                txt_putc(x, y, room_ch[mnode[f][c].room],
                         (here && c == sel) ? CLR_YELLOW : clr);
            }
        }
        txt_put(0, 18, "M:FOE E:ELITE ?:EVT R:REST", CLR_GRAY);
        txt_put(0, 19, "$:SHOP T:CHEST  SEL:DECK", CLR_GRAY);

        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_LEFT)) {
                for (int c = sel - 1; c >= 0; c--)
                    if (reachable(run.floor, c)) { sel = c; sfx_blip(); break; }
                break;
            }
            if (key_repeat(KEY_RIGHT)) {
                for (int c = sel + 1; c < MCOLS; c++)
                    if (reachable(run.floor, c)) { sel = c; sfx_blip(); break; }
                break;
            }
            if (key_hit(KEY_SELECT)) { deck_browse("DECK", 0); break; }
            if (key_hit(KEY_A)) {
                sfx_ok();
                run.mapcol = sel;
                int room = mnode[run.floor][sel].room;
                run.floor++;
                route_room(room);
                return;
            }
        }
    }
}
