#include "cards.h"

/* name, cost/up, type, rarity, target, exh, dmg/up, hits, blk/up, draw/up, sp, spval/up, desc */
const Card cards[N_CARDS] = {
/* starters */
[C_STRIKE]      = {"STRIKE",      1,1, CT_ATTACK, CR_STARTER,  TGT_ENEMY,0,  6,9, 1, 0,0, 0,0, SP_NONE,0,0,        ""},
[C_DEFEND]      = {"DEFEND",      1,1, CT_SKILL,  CR_STARTER,  TGT_NONE, 0,  0,0, 1, 5,8, 0,0, SP_NONE,0,0,        ""},
[C_BASH]        = {"BASH",        2,2, CT_ATTACK, CR_STARTER,  TGT_ENEMY,0,  8,10,1, 0,0, 0,0, SP_VULN,2,3,        "APPLY VULN"},
/* commons */
[C_ANGER]       = {"ANGER",       0,0, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  6,8, 1, 0,0, 0,0, SP_ANGER,0,0,       "COPY TO DISCARD"},
[C_BODYSLAM]    = {"BODY SLAM",   1,0, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  0,0, 1, 0,0, 0,0, SP_BODYSLAM,0,0,    "DMG = BLOCK"},
[C_CLEAVE]      = {"CLEAVE",      1,1, CT_ATTACK, CR_COMMON,   TGT_ALL,  0,  8,11,1, 0,0, 0,0, SP_NONE,0,0,        "HIT ALL"},
[C_CLOTHESLINE] = {"CLOTHESLINE", 2,2, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0, 12,14,1, 0,0, 0,0, SP_WEAK,2,3,        "APPLY WEAK"},
[C_FLEX]        = {"FLEX",        0,0, CT_SKILL,  CR_COMMON,   TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_FLEX,2,4,        "TEMP STR"},
[C_HEADBUTT]    = {"HEADBUTT",    1,1, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  9,12,1, 0,0, 0,0, SP_HEADBUTT,0,0,    "DISCARD TO DRAW"},
[C_HEAVYBLADE]  = {"HEAVY BLADE", 2,2, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0, 14,14,1, 0,0, 0,0, SP_HEAVYBLADE,3,5,  "STR X3"},
[C_IRONWAVE]    = {"IRON WAVE",   1,1, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  5,7, 1, 5,7, 0,0, SP_NONE,0,0,        "DMG + BLOCK"},
[C_POMMEL]      = {"POMMEL STRK", 1,1, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  9,10,1, 0,0, 1,2, SP_NONE,0,0,        "DRAW"},
[C_SHRUG]       = {"SHRUG IT OFF",1,1, CT_SKILL,  CR_COMMON,   TGT_NONE, 0,  0,0, 1, 8,11,1,1, SP_NONE,0,0,        "BLOCK + DRAW"},
[C_THUNDERCLAP] = {"THUNDERCLAP", 1,1, CT_ATTACK, CR_COMMON,   TGT_ALL,  0,  4,7, 1, 0,0, 0,0, SP_VULN,1,1,        "ALL: DMG + VULN"},
[C_TWINSTRIKE]  = {"TWIN STRIKE", 1,1, CT_ATTACK, CR_COMMON,   TGT_ENEMY,0,  5,7, 2, 0,0, 0,0, SP_NONE,0,0,        "HIT TWICE"},
/* uncommons */
[C_BLOODLET]    = {"BLOODLETTING",0,0, CT_SKILL,  CR_UNCOMMON, TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_BLOODLET,2,3,    "LOSE 3HP GAIN E"},
[C_CARNAGE]     = {"CARNAGE",     2,2, CT_ATTACK, CR_UNCOMMON, TGT_ENEMY,0, 20,28,1, 0,0, 0,0, SP_ETHEREAL,0,0,    "ETHEREAL"},
[C_DISARM]      = {"DISARM",      1,1, CT_SKILL,  CR_UNCOMMON, TGT_ENEMY,1,  0,0, 1, 0,0, 0,0, SP_DISARM,2,3,      "ENEMY -STR"},
[C_ENTRENCH]    = {"ENTRENCH",    2,1, CT_SKILL,  CR_UNCOMMON, TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_ENTRENCH,0,0,    "DOUBLE BLOCK"},
[C_FLAMEBARRIER]= {"FLAME BARIER",2,2, CT_SKILL,  CR_UNCOMMON, TGT_NONE, 0,  0,0, 1,12,16, 0,0, SP_FLAMEBARRIER,4,6,"THORNS"},
[C_INFLAME]     = {"INFLAME",     1,1, CT_POWER,  CR_UNCOMMON, TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_STR,2,3,         "+STR"},
[C_METALLICIZE] = {"METALLICIZE", 1,1, CT_POWER,  CR_UNCOMMON, TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_METALLICIZE,3,4, "BLK EACH TURN"},
[C_SHOCKWAVE]   = {"SHOCKWAVE",   2,2, CT_SKILL,  CR_UNCOMMON, TGT_ALL,  1,  0,0, 1, 0,0, 0,0, SP_SHOCKWAVE,3,5,   "ALL: WEAK+VULN"},
[C_UPPERCUT]    = {"UPPERCUT",    2,2, CT_ATTACK, CR_UNCOMMON, TGT_ENEMY,0, 13,13,1, 0,0, 0,0, SP_UPPERCUT,1,2,    "WEAK + VULN"},
[C_WHIRLWIND]   = {"WHIRLWIND",   COST_X,COST_X, CT_ATTACK, CR_UNCOMMON, TGT_ALL,0, 5,8,1, 0,0, 0,0, SP_WHIRLWIND,0,0, "ALL X TIMES"},
/* rares */
[C_BARRICADE]   = {"BARRICADE",   3,2, CT_POWER,  CR_RARE,     TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_BARRICADE,0,0,   "BLOCK STAYS"},
[C_BLUDGEON]    = {"BLUDGEON",    3,3, CT_ATTACK, CR_RARE,     TGT_ENEMY,0, 32,42,1, 0,0, 0,0, SP_NONE,0,0,        ""},
[C_DEMONFORM]   = {"DEMON FORM",  3,3, CT_POWER,  CR_RARE,     TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_DEMONFORM,2,3,   "+STR EACH TURN"},
[C_FEED]        = {"FEED",        1,1, CT_ATTACK, CR_RARE,     TGT_ENEMY,1, 10,12,1, 0,0, 0,0, SP_FEED,3,4,        "KILL: +MAXHP"},
[C_IMMOLATE]    = {"IMMOLATE",    2,2, CT_ATTACK, CR_RARE,     TGT_ALL,  0, 21,28,1, 0,0, 0,0, SP_IMMOLATE,0,0,    "ALL. ADD BURN"},
[C_IMPERVIOUS]  = {"IMPERVIOUS",  2,2, CT_SKILL,  CR_RARE,     TGT_NONE, 1,  0,0, 1,30,40, 0,0, SP_NONE,0,0,       ""},
[C_OFFERING]    = {"OFFERING",    0,0, CT_SKILL,  CR_RARE,     TGT_NONE, 1,  0,0, 1, 0,0, 0,0, SP_OFFERING,3,5,    "6HP: +2E, DRAW"},
/* statuses */
[C_WOUND]       = {"WOUND",       0,0, CT_STATUS, CR_STATUS,   TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_UNPLAYABLE,0,0,  "UNPLAYABLE"},
[C_BURN]        = {"BURN",        0,0, CT_STATUS, CR_STATUS,   TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_BURN,2,2,        "END TURN: 2 DMG"},
[C_SLIMED]      = {"SLIMED",      1,1, CT_STATUS, CR_STATUS,   TGT_NONE, 1,  0,0, 1, 0,0, 0,0, SP_NONE,0,0,        "EXHAUST"},
[C_DAZED]       = {"DAZED",       0,0, CT_STATUS, CR_STATUS,   TGT_NONE, 0,  0,0, 1, 0,0, 0,0, SP_UNPLAYABLE,0,0,  "UNPLAYABLE.ETHRL"},
};

int card_cost(CardInst c)  { return c.up ? cards[c.id].upcost  : cards[c.id].cost; }
int card_dmg(CardInst c)   { return c.up ? cards[c.id].updmg   : cards[c.id].dmg; }
int card_block(CardInst c) { return c.up ? cards[c.id].upblock : cards[c.id].block; }
int card_draw(CardInst c)  { return c.up ? cards[c.id].updraw  : cards[c.id].draw; }
int card_spval(CardInst c) { return c.up ? cards[c.id].upspval : cards[c.id].spval; }

const char *card_name(CardInst c, char *buf)
{
    int i = 0;
    const char *n = cards[c.id].name;
    while (*n) buf[i++] = *n++;
    if (c.up) buf[i++] = '+';
    buf[i] = 0;
    return buf;
}

/* ---------- run setup ---------- */

void run_new(void)
{
    run.hp = run.maxhp = 80;
    run.gold = 99;
    run.floor = 0;
    run.mapcol = 0;
    run.ndeck = 0;
    run.nrelics = 0;
    run.potion = 0;
    for (int i = 0; i < 5; i++) deck_add(C_STRIKE);
    for (int i = 0; i < 4; i++) deck_add(C_DEFEND);
    deck_add(C_BASH);
    relic_add(RLC_BURNINGBLOOD);
    run.act_boss = rng_range(3);
    map_generate();
}

/* ---------- piles ---------- */

Piles piles;

static void shuffle(CardInst *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(i + 1);
        CardInst t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

void piles_init(void)
{
    piles.ndraw = run.ndeck;
    for (int i = 0; i < run.ndeck; i++) piles.draw[i] = run.deck[i];
    shuffle(piles.draw, piles.ndraw);
    piles.ndiscard = piles.nexhaust = piles.nhand = 0;
}

void pile_shuffle_draw(void)
{
    /* discard -> draw, shuffle */
    for (int i = 0; i < piles.ndiscard; i++)
        piles.draw[piles.ndraw++] = piles.discard[i];
    piles.ndiscard = 0;
    shuffle(piles.draw, piles.ndraw);
}

int pile_draw(int n)
{
    int drawn = 0;
    while (n-- > 0 && piles.nhand < MAX_HAND) {
        if (piles.ndraw == 0) {
            if (piles.ndiscard == 0) break;
            pile_shuffle_draw();
        }
        piles.hand[piles.nhand++] = piles.draw[--piles.ndraw];
        drawn++;
    }
    return drawn;
}

static void hand_remove(int i)
{
    for (; i < piles.nhand - 1; i++) piles.hand[i] = piles.hand[i + 1];
    piles.nhand--;
}

void pile_discard_card(int i)
{
    piles.discard[piles.ndiscard++] = piles.hand[i];
    hand_remove(i);
}

void pile_exhaust_card(int i)
{
    piles.exhaust[piles.nexhaust++] = piles.hand[i];
    hand_remove(i);
}

void pile_discard_hand(void)
{
    while (piles.nhand > 0) {
        if (cards[piles.hand[0].id].sp == SP_ETHEREAL ||
            piles.hand[0].id == C_DAZED)
            pile_exhaust_card(0);
        else
            pile_discard_card(0);
    }
}

void pile_add_discard(u8 card_id)
{
    if (piles.ndiscard < MAX_DECK) {
        piles.discard[piles.ndiscard].id = card_id;
        piles.discard[piles.ndiscard].up = 0;
        piles.ndiscard++;
    }
}

/* ---------- deck browser ---------- */

static const char *type_str[] = {"ATK", "SKL", "PWR", "STS"};
static const int type_clr[] = {CLR_RED, CLR_GREEN, CLR_CYAN, CLR_GRAY};

/* border-only frame on BG1 (leaves interior clear so a BG2 face shows) */
static void zoom_frame(int x, int y, int w, int h, int clr)
{
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

static void zoom_center(int cx, int y, const char *s, int clr)
{
    int n = 0; while (s[n]) n++;
    txt_put(cx - n / 2, y, s, clr);
}

/* big-card zoom: render the selected card as the 40x56 battle face art
   (slot 0) with a frame + live cost/dmg/blk + full name/type/desc. any key
   returns. uses BG2 + charblock-1 face tiles/pal (bank 0 entries 4..15;
   list text uses entries 1..3, so no clobber of the list on return). */
static void card_zoom(CardInst c)
{
    const Card *cd = &cards[c.id];
    int fx = 12, fy = 3;               /* 5x7 face at cols 12-16, rows 3-9 */
    char nb[16];
    bg2_clear();
    card_face_load(0, c.id);
    card_face_stamp(0, fx, fy);
    txt_clear(); ui_clear();
    zoom_frame(fx - 1, fy - 1, 7, 9, c.up ? CLR_GREEN : CLR_YELLOW);
    /* live cost orb over the baked one (upgrade/X aware) */
    ui_tile(fx, fy, T_ENERGY, CLR_YELLOW);
    if (cd->cost == COST_X) txt_putc(fx, fy, 'X', CLR_WHITE);
    else txt_int(fx, fy, card_cost(c), CLR_WHITE);
    /* live dmg/blk badge in the face text region */
    if (card_dmg(c))        txt_int(fx + 2, fy + 5, card_dmg(c), CLR_WHITE);
    else if (card_block(c)) txt_int(fx + 2, fy + 5, card_block(c), CLR_WHITE);
    /* name / type / full desc below the card */
    zoom_center(15, 12, card_name(c, nb), c.up ? CLR_GREEN : CLR_WHITE);
    zoom_center(15, 14, type_str[cd->type], type_clr[cd->type]);
    if (cd->desc[0]) zoom_center(15, 16, cd->desc, CLR_GRAY);
    zoom_center(15, 19, "ANY KEY: BACK", CLR_GRAY);
    for (;;) { vsync(); key_poll(); if (key_hit(KEY_ANY)) break; }
    bg2_clear();                        /* wipe the face; list backdrop = black */
}

/* one stat's before>after on row y; green = upgraded value. advances x, with
   a trailing space. no-op (returns x unchanged) when the stat is unchanged. */
static int pv_stat(int x, int y, const char *lbl, int a, int b)
{
    if (a == b) return x;
    int i = 0; while (lbl[i]) i++;
    txt_put(x, y, lbl, CLR_GRAY); x += i;
    x = txt_int_at(x, y, a, CLR_WHITE);
    txt_putc(x, y, '>', CLR_GRAY); x++;
    x = txt_int_at(x, y, b, CLR_GREEN);
    return x + 1;
}

/* upgrade preview line for the SMITH/upgrade picker: BEFORE>AFTER for each
   stat that upgrading the card would change. already-upgraded / no-change
   cards get a plain message instead of a bogus arrow. */
static void upgrade_line(int y, CardInst c)
{
    if (c.up) { txt_put(1, y, "ALREADY UPGRADED", CLR_GRAY); return; }
    CardInst u = c; u.up = 1;
    int x0 = 1, x = x0;
    x = pv_stat(x, y, "DMG",  card_dmg(c),   card_dmg(u));
    x = pv_stat(x, y, "BLK",  card_block(c), card_block(u));
    x = pv_stat(x, y, "COST", card_cost(c),  card_cost(u));
    x = pv_stat(x, y, "DRAW", card_draw(c),  card_draw(u));
    x = pv_stat(x, y, "EFF",  card_spval(c), card_spval(u));
    if (x == x0) txt_put(1, y, "NO STAT CHANGE", CLR_GRAY);
}

/* core scrolling list over an arbitrary card array. pick_mode 1 → A returns
   the index (remove/purge). pick_mode 2 → A returns index + shows an upgrade
   before>after preview for the selected card (SMITH/upgrade pickers). pick_mode
   0 → view only; A zooms the highlighted card, returns -1. n==0 → returns -1. */
int pile_browse(const CardInst *arr, int n, const char *title, int pick_mode)
{
    if (n == 0) return -1;
    scene_none();
    int sel = 0, top = 0;
    const int ROWS = 14;
    char nb[16];

    for (;;) {
        txt_clear(); ui_clear();
        txt_put(1, 0, title, CLR_YELLOW);
        txt_int(25, 0, n, CLR_GRAY);
        ui_box(0, 1, 30, 17, CLR_GRAY);

        if (sel < top) top = sel;
        if (sel >= top + ROWS) top = sel - ROWS + 1;

        for (int r = 0; r < ROWS && top + r < n; r++) {
            CardInst c = arr[top + r];
            int y = 2 + r;
            int hi = (top + r == sel);
            if (hi) ui_fill(1, y, 28, 1, T_PANEL, CLR_BLUE);
            txt_putc(1, y, hi ? '>' : ' ', CLR_WHITE);
            txt_put(2, y, card_name(c, nb), c.up ? CLR_GREEN : CLR_WHITE);
            if (cards[c.id].cost == COST_X) txt_putc(17, y, 'X', CLR_YELLOW);
            else txt_int(17, y, card_cost(c), CLR_YELLOW);
            txt_put(19, y, type_str[cards[c.id].type], type_clr[cards[c.id].type]);
            if (card_dmg(c))   { txt_int_at(23, y, card_dmg(c), CLR_RED); }
            else if (card_block(c)) { txt_int_at(23, y, card_block(c), CLR_BLUE); }
        }
        /* row 18: upgrade before>after in upgrade mode, else selected desc */
        if (pick_mode == 2) upgrade_line(18, arr[sel]);
        else txt_put(1, 18, cards[arr[sel].id].desc, CLR_GRAY);
        txt_put(1, 19, pick_mode ? "A:PICK B:BACK" : "A:ZOOM B:BACK", CLR_GRAY);

        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)         { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < n - 1)   { sel++; sfx_blip(); break; }
            if (key_hit(KEY_B)) { sfx_bad(); return -1; }
            if (key_hit(KEY_A)) {
                if (pick_mode) { sfx_ok(); return sel; }
                sfx_ok(); card_zoom(arr[sel]); break;   /* redraw list on return */
            }
        }
    }
}

int deck_browse(const char *title, int pick_mode)
{
    return pile_browse(run.deck, run.ndeck, title, pick_mode);
}
