#include "cards.h"
#include "sprites.h"

/* ---------- relics ---------- */

const char relic_names[N_RELICS][14] = {
    "BURNING BLOOD", "VAJRA", "BRONZE SCALES", "ANCHOR",
    "LANTERN", "STRAWBERRY", "BAG OF PREP", "BLOOD VIAL",
};
const char relic_desc[N_RELICS][24] = {
    "HEAL 6 HP AFTER COMBAT",    /* BURNING BLOOD */
    "START COMBAT: +1 STR",      /* VAJRA */
    "START COMBAT: +3 THORNS",   /* BRONZE SCALES */
    "TURN 1: +10 BLOCK",         /* ANCHOR */
    "TURN 1: +1 ENERGY",         /* LANTERN */
    "+7 MAX HP",                 /* STRAWBERRY */
    "TURN 1: DRAW 2 CARDS",      /* BAG OF PREP */
    "START COMBAT: HEAL 2 HP",   /* BLOOD VIAL */
};

/* ---------- potions ---------- */

const char potion_names[N_POTIONS][12] = {
    "", "BLOOD POT.", "STR POTION", "BLOCK POT.", "ENERGY POT",
};
const char potion_tags[N_POTIONS][5] = { "", "HP+", "STR+", "BLK+", "NRG+" };
const char potion_short[N_POTIONS][3] = { "", "HP", "ST", "BK", "NG" };

/* first empty belt slot index, or -1 if the belt is full */
int potion_first_empty(void)
{
    for (int i = 0; i < N_POTION_SLOTS; i++)
        if (run.potions[i] == POT_NONE) return i;
    return -1;
}

/* drop roll after combat: 30% normal / 50% elite, only if a slot is free */
u8 potion_roll(int elite)
{
    if (potion_first_empty() < 0) return POT_NONE;
    if ((int)rng_range(100) >= (elite ? 50 : 30)) return POT_NONE;
    return (u8)(POT_HEAL + rng_range(N_POTIONS - 1));
}

void relic_add(u8 r)
{
    if (run.nrelics < 16 && !run_has_relic(r)) {
        run.relics[run.nrelics++] = r;
        if (r == RLC_STRAWBERRY) { run.maxhp += 7; run.hp += 7; }
    }
}

u8 relic_random(void)
{
    u8 pool[N_RELICS]; int n = 0;
    for (u8 r = 0; r < N_RELICS; r++)
        if (!run_has_relic(r)) pool[n++] = r;
    if (!n) return 0xFF;
    return pool[rng_range(n)];
}

void deck_remove(int idx)
{
    for (int i = idx; i < run.ndeck - 1; i++)
        run.deck[i] = run.deck[i + 1];
    run.ndeck--;
}

/* ---------- shared bits ---------- */

static void header(const char *title, int clr)
{
    txt_clear(); ui_clear();
    txt_put(1, 0, title, clr);
    ui_tile(16, 0, T_HEART, CLR_RED);
    int xx = txt_int_at(18, 0, run.hp, CLR_WHITE);
    txt_putc(xx, 0, '/', CLR_GRAY); txt_int(xx + 1, 0, run.maxhp, CLR_GRAY);
    ui_tile(25, 0, T_GOLDPT, CLR_YELLOW);
    txt_int(27, 0, run.gold, CLR_YELLOW);
}

static void heal(int amt)
{
    run.hp += amt;
    if (run.hp > run.maxhp) run.hp = run.maxhp;
    sfx_heal();
}

/* simple vertical menu, returns index or -1 on B */
static int menu(int x, int y, const char *const *items, int n, int allow_back)
{
    int sel = 0;
    for (;;) {
        for (int i = 0; i < n; i++) {
            ui_fill(x - 1, y + i, 22, 1, T_PANEL, i == sel ? CLR_BLUE : CLR_GRAY);
            txt_putc(x - 1, y + i, i == sel ? '>' : ' ', CLR_YELLOW);
            txt_put(x, y + i, items[i], i == sel ? CLR_WHITE : CLR_GRAY);
        }
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)   { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < n-1){ sel++; sfx_blip(); break; }
            if (key_hit(KEY_A)) { sfx_ok(); return sel; }
            if (allow_back && key_hit(KEY_B)) { sfx_bad(); return -1; }
        }
    }
}

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

/* centered text within a cell whose center column is cx */
static void txt_center(int cx, int y, const char *s, int clr)
{
    txt_put(cx - slen(s) / 2, y, s, clr);
}

/* border-only frame on BG1 (unlike ui_box, no opaque interior fill, so a
   BG2 card face shows through). w>=2, h>=2. */
static void card_frame(int x, int y, int w, int h, int clr)
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

/* overlay a card face's live cost orb + dmg/blk badge (BG0/BG1, over the
   BG2 face art). x,y = top-left grid cell of the 5x7 face. */
static void face_overlay(int x, int y, int id)
{
    ui_tile(x, y, T_ENERGY, CLR_YELLOW);
    txt_int(x, y, cards[id].cost, CLR_WHITE);
    if (cards[id].dmg)        txt_int(x + 2, y + 5, cards[id].dmg, CLR_WHITE);
    else if (cards[id].block) txt_int(x + 2, y + 5, cards[id].block, CLR_WHITE);
}

/* pick 1 of 3 cards; returns card id or -1 (skip) */
#define RCARD_X(i)  (3 + (i) * 9)   /* face cols 3/12/21, 5 wide */
#define RCARD_Y     3               /* face rows 3..9 */
static int card_choice(void)
{
    u8 c3[3];
    for (int i = 0; i < 3; i++) {
        int rar, roll = rng_range(100);
        if (reward_elite) rar = roll < 45 ? CR_COMMON : roll < 88 ? CR_UNCOMMON : CR_RARE;
        else              rar = roll < 60 ? CR_COMMON : roll < 97 ? CR_UNCOMMON : CR_RARE;
        int id;
        do {
            id = rng_range(N_REAL_CARDS);
        } while (cards[id].rarity != rar ||
                 (i > 0 && id == c3[0]) || (i > 1 && id == c3[1]));
        c3[i] = id;
    }
    /* load + stamp the three big faces onto BG2 once (map persists across
       the nav loop; header() only clears BG0/BG1) */
    scene_none();
    for (int i = 0; i < 3; i++) {
        card_face_load(i, c3[i]);
        card_face_stamp(i, RCARD_X(i), RCARD_Y);
    }
    int sel = 0;
    char nb[16];
    for (;;) {
        header("CHOOSE A CARD", CLR_YELLOW);
        for (int i = 0; i < 3; i++) {
            int x = RCARD_X(i);
            card_frame(x - 1, RCARD_Y - 1, 7, 9, i == sel ? CLR_YELLOW : CLR_GRAY);
            face_overlay(x, RCARD_Y, c3[i]);
            CardInst ci = {c3[i], 0};
            txt_center(x + 2, RCARD_Y + 8, card_name(ci, nb), i == sel ? CLR_WHITE : CLR_GRAY);
        }
        /* selected-card detail line */
        txt_put(1, 13, cards[c3[sel]].desc, CLR_GRAY);
        txt_put(1, 17, "A:TAKE B:SKIP", CLR_GRAY);
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_LEFT)  && sel > 0) { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_RIGHT) && sel < 2) { sel++; sfx_blip(); break; }
            if (key_repeat(KEY_UP)    && sel > 0) { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN)  && sel < 2) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_A)) { sfx_ok(); scene_none(); return c3[sel]; }
            if (key_hit(KEY_B)) { sfx_bad(); scene_none(); return -1; }
        }
    }
}

/* ---------- reward ---------- */

void reward_screen(int elite)
{
    scene_none();
    int gold = elite ? 25 + rng_range(11) : 10 + rng_range(11);
    int got_gold = 0, got_card = 0, got_relic = !elite;
    u8 relic = elite ? relic_random() : 0xFF;
    if (relic == 0xFF) got_relic = 1;
    u8 pot = potion_roll(elite);

    for (;;) {
        header("VICTORY! REWARDS", CLR_GREEN);
        const char *items[5]; int n = 0, act[5];
        char gbuf[12] = "GOLD +";
        gbuf[6] = '0' + gold / 10; gbuf[7] = '0' + gold % 10; gbuf[8] = 0;
        if (!got_gold)  { items[n] = gbuf; act[n++] = 0; }
        if (!got_relic) { items[n] = relic_names[relic]; act[n++] = 1; }
        if (!got_card)  { items[n] = "CARD REWARD"; act[n++] = 2; }
        if (pot)        { items[n] = potion_names[pot]; act[n++] = 4; }
        items[n] = "LEAVE"; act[n++] = 3;

        int m = menu(4, 4, items, n, 0);
        switch (act[m]) {
        case 0: run.gold += gold; got_gold = 1; sfx_coin(); break;
        case 1: relic_add(relic); got_relic = 1; break;
        case 2: {
            int id = card_choice();
            if (id >= 0) deck_add(id);
            got_card = 1;
            break;
        }
        case 4: {
            int slot = potion_first_empty();
            if (slot >= 0) { run.potions[slot] = pot; pot = 0; sfx_heal(); }
            else sfx_bad();
            break;
        }
        case 3:
            if (run_has_relic(RLC_BURNINGBLOOD)) heal(6);
            gstate = ST_MAP; return;
        }
    }
}

/* ---------- neow's blessing (run start) ----------
   A one-time gift the whale offers before the first map. Loop the menu until
   the player commits to a blessing that actually takes effect: HP/gold always
   commit; remove/obtain commit only if the sub-picker wasn't cancelled. The
   deck_browse/card_choice helpers draw their own full screens (and both leave
   the backdrop on scene_none), so we simply redraw the Neow menu on the next
   loop pass. */
void neow_screen(void)
{
    static const char *const items[4] = {
        "MAX HP +8", "GAIN 100 GOLD", "REMOVE A CARD", "OBTAIN A CARD"
    };
    scene_none();
    for (;;) {
        txt_clear(); ui_clear();
        txt_put2x(1, 2, "NEOWS BLESSING", CLR_CYAN);
        txt_put(4, 6, "A WHALE OFFERS A GIFT", CLR_GRAY);
        int m = menu(6, 9, items, 4, 0);
        if (m == 0) { run.maxhp += 8; run.hp += 8; sfx_ok(); return; }
        if (m == 1) { run.gold += 100; sfx_ok(); return; }
        if (m == 2) {
            int i = deck_browse("REMOVE WHICH?", 1);
            if (i >= 0) { deck_remove(i); sfx_ok(); return; }
        } else { /* m == 3 */
            reward_elite = 0;
            int id = card_choice();
            if (id >= 0) { deck_add(id); sfx_ok(); return; }
        }
        /* sub-picker cancelled: fall through to redraw the Neow menu */
    }
}

/* ---------- rest ---------- */

void rest_screen(void)
{
    scene_none();
    header("REST SITE", CLR_ORANGE);
    txt_put(3, 3, "THE FIRE CRACKLES...", CLR_GRAY);
    static const char *const items[] = {"REST  (HEAL 30%)", "SMITH (UPGRADE)", "LEAVE"};
    for (;;) {
        int m = menu(4, 6, items, 3, 0);
        if (m == 0) { heal(run.maxhp * 3 / 10); break; }
        if (m == 1) {
            int i = deck_browse("UPGRADE WHICH?", 2);
            header("REST SITE", CLR_ORANGE);
            if (i < 0) continue;
            if (run.deck[i].up) { sfx_bad(); continue; }
            run.deck[i].up = 1; sfx_heal();
            break;
        }
        if (m == 2) break;
    }
    gstate = ST_MAP;
}

/* ---------- treasure ---------- */

void treasure_screen(void)
{
    scene_none();
    header("TREASURE!", CLR_GREEN);
    u8 r = relic_random();
    int gold = 20 + rng_range(16);
    run.gold += gold;
    sfx_coin();
    txt_put(4, 5, "FOUND:", CLR_WHITE);
    txt_put(4, 7, "GOLD +", CLR_YELLOW); txt_int(10, 7, gold, CLR_YELLOW);
    if (r != 0xFF) { relic_add(r); txt_put(4, 9, relic_names[r], CLR_CYAN); }
    txt_put(4, 12, "PRESS A", CLR_GRAY);
    sfx_heal();
    for (;;) { vsync(); key_poll(); if (key_hit(KEY_A)) break; }
    gstate = ST_MAP;
}

/* ---------- shop ---------- */

#define SCARD_X(i)  (1 + (i) * 6)   /* face cols 1/7/13/19/25, 5 wide */
#define SCARD_Y     2               /* face rows 2..8 */

void shop_screen(void)
{
    scene_shop();
    obj_show(6, SPR_LOOTER, 200, 128);
    u8 ids[5]; s16 price[5]; u8 sold[5] = {0};
    for (int i = 0; i < 5; i++) {
        int id;
        do { id = rng_range(N_REAL_CARDS); }
        while (cards[id].rarity == CR_STARTER ||
               (i > 0 && id == ids[0]) || (i > 1 && id == ids[1]) ||
               (i > 2 && id == ids[2]) || (i > 3 && id == ids[3]));
        ids[i] = id;
        price[i] = 45 + rng_range(16) + cards[id].rarity * 25;
    }
    u8 relic = relic_random();
    s16 rprice = 140 + rng_range(21);
    u8 rsold = (relic == 0xFF);
    u8 removed = 0;
    u8 pot = (u8)(POT_HEAL + rng_range(N_POTIONS - 1));
    u8 psold = 0;
    char nb[16];
    int sel = 0;

    /* stamp the 5 big card faces across the top once (BG2 map persists;
       header() clears only BG0/BG1). Re-stamped after any scene_shop redraw. */
    for (int i = 0; i < 5; i++) {
        card_face_load(i, ids[i]);
        card_face_stamp(i, SCARD_X(i), SCARD_Y);
    }

    for (;;) {
        header("SHOP", CLR_YELLOW);
        ui_fill(0, 10, 30, 5, T_PANEL, CLR_GRAY);   /* backdrop for menu rows */
        /* 5 card faces across the top + price under each */
        for (int i = 0; i < 5; i++) {
            int x = SCARD_X(i);
            if (sold[i]) { txt_center(x + 2, SCARD_Y + 3, "SOLD", CLR_GRAY); continue; }
            int can = run.gold >= price[i];
            face_overlay(x, SCARD_Y, ids[i]);
            if (sel == i) card_frame(x - 1, SCARD_Y - 1, 7, 9, CLR_YELLOW);
            txt_int(x + 1, SCARD_Y + 7, price[i], can ? CLR_YELLOW : CLR_DKRED);
        }
        /* relic / potion / remove / leave text rows */
        int y = 11;
        if (sel == 5) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 5 ? '>' : ' ', CLR_YELLOW);
        if (rsold) txt_put(2, y, "SOLD OUT", CLR_GRAY);
        else {
            txt_put(2, y, relic_names[relic], run.gold >= rprice ? CLR_CYAN : CLR_GRAY);
            txt_int(20, y, rprice, run.gold >= rprice ? CLR_YELLOW : CLR_DKRED);
        }
        y = 12;
        if (sel == 6) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 6 ? '>' : ' ', CLR_YELLOW);
        if (psold) txt_put(2, y, "SOLD OUT", CLR_GRAY);
        else {
            int can = run.gold >= POTION_PRICE && potion_first_empty() >= 0;
            txt_put(2, y, potion_names[pot], can ? CLR_GREEN : CLR_GRAY);
            txt_int(20, y, POTION_PRICE, can ? CLR_YELLOW : CLR_DKRED);
        }
        y = 13;
        if (sel == 7) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 7 ? '>' : ' ', CLR_YELLOW);
        txt_put(2, y, removed ? "REMOVED" : "REMOVE CARD", removed ? CLR_GRAY : CLR_WHITE);
        if (!removed) txt_int(20, y, 75, run.gold >= 75 ? CLR_YELLOW : CLR_DKRED);
        y = 14;
        if (sel == 8) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 8 ? '>' : ' ', CLR_YELLOW);
        txt_put(2, y, "LEAVE", CLR_WHITE);
        /* selected card name + description */
        if (sel < 5 && !sold[sel]) {
            CardInst ci = {ids[sel], 0};
            txt_put(1, 16, card_name(ci, nb), CLR_WHITE);
            txt_put(1, 17, cards[ids[sel]].desc, CLR_GRAY);
        }
        txt_put(1, 19, "A:BUY B:LEAVE", CLR_GRAY);

        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)   { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < 8) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_B)) { obj_hide_all(); gstate = ST_MAP; return; }
            if (key_hit(KEY_A)) {
                if (sel < 5 && !sold[sel] && run.gold >= price[sel]) {
                    run.gold -= price[sel]; deck_add(ids[sel]); sold[sel] = 1; sfx_coin();
                    bg2_fill(SCARD_X(sel), SCARD_Y, 5, 7, 0);   /* clear the face */
                } else if (sel == 5 && !rsold && run.gold >= rprice) {
                    run.gold -= rprice; relic_add(relic); rsold = 1; sfx_coin();
                } else if (sel == 6 && !psold && potion_first_empty() >= 0 &&
                           run.gold >= POTION_PRICE) {
                    run.gold -= POTION_PRICE;
                    run.potions[potion_first_empty()] = pot; psold = 1; sfx_coin();
                } else if (sel == 7 && !removed && run.gold >= 75) {
                    int i = deck_browse("REMOVE WHICH?", 1);
                    scene_shop();   /* bg2_clear'd -> re-stamp the unsold faces */
                    for (int k = 0; k < 5; k++)
                        if (!sold[k]) card_face_stamp(k, SCARD_X(k), SCARD_Y);
                    if (i >= 0) { run.gold -= 75; deck_remove(i); removed = 1; sfx_ok(); }
                } else if (sel == 8) { obj_hide_all(); gstate = ST_MAP; return; }
                else sfx_bad();
                break;
            }
        }
    }
}

/* ---------- events ---------- */

void event_screen(void)
{
    scene_none();
    int ev = rng_range(4);
    for (;;) {
        switch (ev) {
        case 0: {
            header("EVENT: BIG FISH", CLR_CYAN);
            txt_put(2, 3, "A BANANA AND A DONUT", CLR_GRAY);
            txt_put(2, 4, "FLOAT BEFORE YOU.", CLR_GRAY);
            static const char *const it[] = {"BANANA (HEAL 1/3)", "DONUT (+5 MAX HP)"};
            int m = menu(4, 7, it, 2, 0);
            if (m == 0) heal(run.maxhp / 3);
            else { run.maxhp += 5; run.hp += 5; sfx_heal(); }
            gstate = ST_MAP; return;
        }
        case 1: {
            header("EVENT: GOLDEN IDOL", CLR_YELLOW);
            txt_put(2, 3, "A GLEAMING IDOL SITS ON", CLR_GRAY);
            txt_put(2, 4, "A TRAPPED PEDESTAL.", CLR_GRAY);
            static const char *const it[] = {"TAKE (+40G, 8 DMG)", "LEAVE"};
            int m = menu(4, 7, it, 2, 0);
            if (m == 0) { run.gold += 40; run.hp -= 8; sfx_hit();
                          if (run.hp <= 0) { run.hp = 0; gstate = ST_GAMEOVER; return; } }
            gstate = ST_MAP; return;
        }
        case 2: {
            header("EVENT: THE CLERIC", CLR_GREEN);
            txt_put(2, 3, "\"HEALING, FOR A PRICE.\"", CLR_GRAY);
            static const char *const it[] = {"HEAL 25%  (35G)", "PURGE A CARD (50G)", "LEAVE"};
            int m = menu(4, 6, it, 3, 0);
            if (m == 0 && run.gold >= 35) { run.gold -= 35; heal(run.maxhp / 4); }
            else if (m == 1 && run.gold >= 50) {
                int i = deck_browse("PURGE WHICH?", 1);
                if (i >= 0) { run.gold -= 50; deck_remove(i); sfx_ok(); }
                else continue;
            }
            else if (m != 2) { sfx_bad(); continue; }
            gstate = ST_MAP; return;
        }
        default: {
            header("EVENT: LIVING WALL", CLR_PURPLE);
            txt_put(2, 3, "FACES IN THE WALL OFFER", CLR_GRAY);
            txt_put(2, 4, "TO CHANGE YOUR DECK.", CLR_GRAY);
            static const char *const it[] = {"FORGET (REMOVE CARD)", "GROW  (UPGRADE CARD)"};
            int m = menu(4, 7, it, 2, 0);
            int i = deck_browse(m == 0 ? "REMOVE WHICH?" : "UPGRADE WHICH?", m == 0 ? 1 : 2);
            if (i < 0) continue;
            if (m == 0) deck_remove(i);
            else run.deck[i].up = 1;
            sfx_ok();
            gstate = ST_MAP; return;
        }
        }
    }
}

/* ---------- game over / victory ---------- */

void gameover_screen(void)
{
    save_run_clear();
    txt_clear(); ui_clear();
    scene_none();
    ui_box(4, 5, 22, 8, CLR_DKRED);
    txt_put2x(7, 6, "YOU DIED", CLR_RED);
    txt_put(8, 9, "FLOOR", CLR_GRAY); txt_int(14, 9, run.floor, CLR_WHITE);
    txt_put(8, 11, "PRESS START", CLR_GRAY);
    sfx_bad();
    for (;;) {
        vsync(); key_poll();
        if (key_hit(KEY_START | KEY_A)) { gstate = ST_TITLE; return; }
    }
}

void victory_screen(void)
{
    save_run_clear();
    txt_clear(); ui_clear();
    scene_none();
    ui_box(3, 4, 24, 10, CLR_YELLOW);
    txt_put2x(4, 5, "ACT 1 CLEAR", CLR_YELLOW);
    txt_put(6, 8, "THE SPIRE AWAITS...", CLR_GRAY);
    ui_tile(8, 10, T_HEART, CLR_RED);
    int xx = txt_int_at(10, 10, run.hp, CLR_WHITE);
    txt_putc(xx, 10, '/', CLR_GRAY); txt_int(xx + 1, 10, run.maxhp, CLR_GRAY);
    txt_put(8, 12, "PRESS START", CLR_WHITE);
    sfx_heal();
    for (;;) {
        vsync(); key_poll();
        if (key_hit(KEY_START | KEY_A)) { gstate = ST_TITLE; return; }
    }
}

/* ---------- settings (from title menu) ---------- */

void settings_screen(void)
{
    scene_title();          /* brick wall + torches backdrop */
    int sel = 0;
    static const char *const names[3] = { "MUSIC", "SOUND FX", "BACK" };
    for (;;) {
        txt_clear(); ui_clear();
        txt_put2x(7, 2, "SETTINGS", CLR_YELLOW);
        for (int i = 0; i < 3; i++) {
            int y = 8 + i * 2;
            ui_fill(5, y, 20, 1, T_PANEL, i == sel ? CLR_BLUE : CLR_GRAY);
            txt_putc(5, y, i == sel ? '>' : ' ', CLR_YELLOW);
            txt_put(7, y, names[i], i == sel ? CLR_WHITE : CLR_GRAY);
        }
        txt_put(20, 8,  opt_music ? "ON" : "OFF", opt_music ? CLR_GREEN : CLR_RED);
        txt_put(20, 10, opt_sfx   ? "ON" : "OFF", opt_sfx   ? CLR_GREEN : CLR_RED);
        txt_put(7, 16, "A:TOGGLE  B:BACK", CLR_GRAY);
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)   { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < 2) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_A) | key_hit(KEY_LEFT) | key_hit(KEY_RIGHT)) {
                if (sel == 0)      { music_enable(!opt_music); save_settings(); sfx_ok(); }
                else if (sel == 1) { opt_sfx = !opt_sfx; save_settings(); sfx_ok(); }
                else               { sfx_ok(); return; }
                break;
            }
            if (key_hit(KEY_B)) { sfx_bad(); return; }
        }
    }
}

/* ---------- relic view (from pause menu) ----------
   Lists the run's owned relics with their effect text so the player can
   read what each does mid-run. Each row = the relic's 16x16 icon (BG2,
   via relic_stamp) + its name and one-line description. hud_load() is
   called first because the relic icon tiles may not be resident in cb1
   outside combat (the map/pause path never loaded them). Scrolls with
   UP/DOWN when more than VIS relics are owned (Act 1 has <=8, so scroll
   is rarely reached; the array caps at 16). B or A returns. */
void relic_view(void)
{
    scene_none();               /* dark backdrop (BG2 -> tile 0) */
    hud_load();                 /* ensure relic icon tiles/pal resident */
    if (run.nrelics == 0) {
        txt_clear(); ui_clear();
        txt_put(1, 0, "RELICS", CLR_YELLOW);
        ui_box(4, 8, 22, 4, CLR_GRAY);
        txt_center(15, 10, "NO RELICS YET", CLR_GRAY);
        txt_put(1, 19, "B:BACK", CLR_GRAY);
        for (;;) {
            vsync(); key_poll();
            if (key_hit(KEY_B) | key_hit(KEY_A)) { sfx_bad(); return; }
        }
    }
    const int VIS = 8;          /* 8 relics x 2 rows = rows 2..17 */
    int top = 0;
    for (;;) {
        txt_clear(); ui_clear();
        bg2_fill(0, 1, 30, 18, 0);       /* wipe stale icons before restamp */
        txt_put(1, 0, "RELICS", CLR_YELLOW);
        int shown = run.nrelics - top;
        if (shown > VIS) shown = VIS;
        for (int i = 0; i < shown; i++) {
            int r = run.relics[top + i], y = 2 + i * 2;
            relic_stamp(r, 0, y);
            txt_put(3, y,     relic_names[r], CLR_WHITE);
            txt_put(3, y + 1, relic_desc[r],  CLR_GRAY);
        }
        if (top > 0)                    txt_put(27, 1,  "UP",  CLR_YELLOW);
        if (top + VIS < run.nrelics)    txt_put(27, 18, "DN",  CLR_YELLOW);
        txt_put(1, 19, "B:BACK", CLR_GRAY);
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && top > 0) { top--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && top + VIS < run.nrelics) {
                top++; sfx_blip(); break;
            }
            if (key_hit(KEY_B) | key_hit(KEY_A)) { sfx_bad(); return; }
        }
    }
}

/* ---------- in-run pause menu (START in combat / on the map) ----------
   Overlay over a darkened backdrop: RESUME / DECK / SETTINGS / SAVE & QUIT.
   Returns 1 when the caller should exit its screen loop (SAVE & QUIT set
   gstate = ST_TITLE); 0 to resume. Caller hides sprites first and
   recomposes its screen after (the START branch mirrors the KEY_B/SELECT
   deck_browse path). SAVE & QUIT calls save_run() and returns to title:
   from the map this is the ordinary between-rooms snapshot; mid-combat it
   persists the live run (run.hp already reflects damage taken, run.floor
   was advanced past the node before combat began), so CONTINUE resumes on
   the map at the next node choices — the current fight is abandoned with no
   reward and no replay (no save corruption, no reward duplication). */
int pause_menu(void)
{
    static const char *const opts[5] =
        { "RESUME", "DECK", "RELICS", "SETTINGS", "SAVE & QUIT" };
    const int BX = 7, BW = 16;            /* menu panel: cols 7..22 */
    const int IY = 10;                    /* first item row */
    int sel = 0;
    for (;;) {
        scene_none();                     /* darken the backdrop (BG2 -> black) */
        txt_clear(); ui_clear();
        /* title banner — 2x PAUSED centered (12 tiles) in a padded box */
        ui_box(7, 4, 16, 4, CLR_YELLOW);
        txt_put2x(9, 5, "PAUSED", CLR_YELLOW);
        /* menu panel — one framed box, only the selected row highlighted */
        ui_box(BX, 9, BW, 7, CLR_GRAY);
        for (int i = 0; i < 5; i++) {
            int ry = IY + i, hi = (i == sel);
            if (hi) ui_fill(BX + 1, ry, BW - 2, 1, T_PANEL, CLR_BLUE);
            txt_center(15, ry, opts[i], hi ? CLR_WHITE : CLR_GRAY);
        }
        txt_center(15, 17, "A:SELECT  B:RESUME", CLR_GRAY);

        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP)   && sel > 0) { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < 4) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_B)) { sfx_bad(); return 0; }        /* RESUME */
            if (key_hit(KEY_A)) {
                if (sel == 0) { sfx_ok(); return 0; }           /* RESUME */
                sfx_ok();
                if (sel == 1) { obj_hide_all(); scene_none();
                                deck_browse("DECK", 0); break; }
                if (sel == 2) { obj_hide_all(); relic_view(); break; }
                if (sel == 3) { settings_screen(); break; }
                save_run(); gstate = ST_TITLE; return 1;        /* SAVE & QUIT */
            }
        }
    }
}
