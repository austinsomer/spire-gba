#ifndef GAME_H
#define GAME_H

#include "gba.h"

/* ---------- engine ---------- */

/* text colors = BG palette banks */
enum {
    CLR_WHITE, CLR_GRAY, CLR_RED, CLR_GREEN, CLR_BLUE,
    CLR_YELLOW, CLR_ORANGE, CLR_CYAN, CLR_PURPLE, CLR_DKRED
};

/* UI tiles in charblock 0 (font occupies 0-127) */
enum {
    T_SOLID = 128,     /* all color1 */
    T_PANEL = 129,     /* all color2 */
    T_BAR0  = 130,     /* 130..138: 0..8 px filled color1 over color2 */
    T_EDGE_T = 140, T_EDGE_B, T_EDGE_L, T_EDGE_R,
    T_CORN_TL, T_CORN_TR, T_CORN_BL, T_CORN_BR,
    T_ARROW = 148,     /* right arrow */
    T_HEART = 149, T_GOLDPT = 150, T_ENERGY = 151, T_SHIELD = 152,
    T_DOT = 153,
};

void video_init(void);
void vsync(void);
void key_poll(void);
u16  key_hit(u16 mask);      /* pressed this frame */
u16  key_held(u16 mask);
u16  key_repeat(u16 mask);   /* hit or held-repeat, for menus */

/* text layer BG0 (30x20). ui layer BG1 behind it */
void txt_clear(void);
void txt_put(int x, int y, const char *s, int clr);
void txt_putc(int x, int y, char c, int clr);
void txt_int(int x, int y, int v, int clr);
int  txt_int_at(int x, int y, int v, int clr); /* returns x after number */
void ui_clear(void);
void ui_tile(int x, int y, int tile, int clr);
void ui_box(int x, int y, int w, int h, int clr);   /* frame on BG1 */
void ui_fill(int x, int y, int w, int h, int tile, int clr);
void ui_bar(int x, int y, int wtiles, int val, int max, int clr);

u32  rng(void);              /* xorshift */
int  rng_range(int n);       /* 0..n-1 */
void rng_seed(u32 s);
extern u32 frame_count;

/* BG2 scenery layer (charblock 1, behind BG0/BG1). tiles in bgtiles.h */
void bg2_load(void);                       /* tiles + palette banks 10-15 */
void bg2_tile(int x, int y, int tile);     /* bank auto from tile */
void bg2_fill(int x, int y, int w, int h, int tile);
void bg2_clear(void);
void ui_icon(int x, int y, int icon);      /* icon tile on BG1, own bank */

/* 2x-scaled text on BG0 (each glyph = 2x2 tiles, synthesized) */
void txt_put2x(int x, int y, const char *s, int clr);

/* scene composers (src/scene.c) */
void scene_battle(void);
void scene_title(void);
void scene_map(void);
void scene_shop(void);
void scene_none(void);

/* obj sprites (32x32, ids in sprites.h enum) */
void sprites_load(void);
void obj_show(int oam_i, int sprite, int x, int y);
void obj_hide(int oam_i);
void obj_hide_all(void);

void sfx_init(void);
void sfx_blip(void);         /* menu move */
void sfx_ok(void);           /* confirm */
void sfx_hit(void);          /* damage (noise) */
void sfx_block(void);
void sfx_bad(void);          /* error/cancel */
void sfx_heal(void);

/* ---------- game ---------- */

#define MAX_DECK   64
#define MAX_HAND   10
#define MAX_ENEMY  5

typedef struct { u8 id; u8 up; } CardInst;

typedef enum {
    ST_TITLE, ST_MAP, ST_COMBAT, ST_REWARD, ST_REST,
    ST_SHOP, ST_EVENT, ST_TREASURE, ST_GAMEOVER, ST_VICTORY
} GState;

/* run state */
typedef struct {
    s16 hp, maxhp;
    s16 gold;
    u8  floor;              /* 0..15 */
    u8  mapcol;             /* current column on map */
    CardInst deck[MAX_DECK];
    u8  ndeck;
    u8  relics[16];
    u8  nrelics;
    u8  act_boss;           /* which boss this run */
    u8  potion;             /* 0 = none, else potion id */
} Run;

extern Run run;
extern GState gstate;

void run_new(void);
int  run_has_relic(u8 r);
void deck_add(u8 card_id);
void deck_remove(int idx);

/* relics */
enum { RLC_BURNINGBLOOD, RLC_VAJRA, RLC_BRONZESCALES, RLC_ANCHOR,
       RLC_LANTERN, RLC_STRAWBERRY, RLC_BAGPREP, RLC_BLOODVIAL, N_RELICS };
extern const char relic_names[N_RELICS][14];
void relic_add(u8 r);
u8   relic_random(void);       /* unowned relic, or 0xFF if all owned */

extern int reward_elite;       /* set before ST_REWARD */

/* screens — each runs its own loop, returns/advances gstate */
void title_screen(void);
void map_screen(void);
void combat_screen(int encounter);  /* sets combat result */
void reward_screen(int elite);
void rest_screen(void);
void shop_screen(void);
void event_screen(void);
void treasure_screen(void);
void gameover_screen(void);
void victory_screen(void);

/* map */
void map_generate(void);
int  map_current_room(void);   /* room type of node player stands on */
extern s8 map_pending_encounter;

enum { ROOM_NONE, ROOM_MONSTER, ROOM_ELITE, ROOM_EVENT, ROOM_REST,
       ROOM_SHOP, ROOM_TREASURE, ROOM_BOSS };

/* combat result */
extern int combat_won;

#endif
