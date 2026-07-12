#include "cards.h"
#include "sprites.h"

/* ---------- relics ---------- */

const char relic_names[N_RELICS][14] = {
    "BURNING BLOOD", "VAJRA", "BRONZE SCALES", "ANCHOR",
    "LANTERN", "STRAWBERRY", "BAG OF PREP", "BLOOD VIAL",
};

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

/* pick 1 of 3 cards; returns card id or -1 (skip) */
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
    int sel = 0;
    char nb[16];
    for (;;) {
        header("CHOOSE A CARD", CLR_YELLOW);
        for (int i = 0; i < 3; i++) {
            int y = 3 + i * 4;
            ui_box(2, y, 26, 4, i == sel ? CLR_YELLOW : CLR_GRAY);
            CardInst ci = {c3[i], 0};
            txt_put(4, y + 1, card_name(ci, nb), CLR_WHITE);
            txt_int(24, y + 1, cards[c3[i]].cost, CLR_YELLOW);
            if (cards[c3[i]].dmg) { txt_put(4, y + 2, "DMG", CLR_RED); txt_int(8, y + 2, cards[c3[i]].dmg, CLR_RED); }
            else if (cards[c3[i]].block) { txt_put(4, y + 2, "BLK", CLR_BLUE); txt_int(8, y + 2, cards[c3[i]].block, CLR_BLUE); }
            txt_put(11, y + 2, cards[c3[i]].desc, CLR_GRAY);
        }
        txt_put(1, 16, "A:TAKE B:SKIP", CLR_GRAY);
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)   { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < 2) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_A)) { sfx_ok(); return c3[sel]; }
            if (key_hit(KEY_B)) { sfx_bad(); return -1; }
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

    for (;;) {
        header("VICTORY! REWARDS", CLR_GREEN);
        const char *items[4]; int n = 0, act[4];
        char gbuf[12] = "GOLD +";
        gbuf[6] = '0' + gold / 10; gbuf[7] = '0' + gold % 10; gbuf[8] = 0;
        if (!got_gold)  { items[n] = gbuf; act[n++] = 0; }
        if (!got_relic) { items[n] = relic_names[relic]; act[n++] = 1; }
        if (!got_card)  { items[n] = "CARD REWARD"; act[n++] = 2; }
        items[n] = "LEAVE"; act[n++] = 3;

        int m = menu(4, 4, items, n, 0);
        switch (act[m]) {
        case 0: run.gold += gold; got_gold = 1; break;
        case 1: relic_add(relic); got_relic = 1; break;
        case 2: {
            int id = card_choice();
            if (id >= 0) deck_add(id);
            got_card = 1;
            break;
        }
        case 3:
            if (run_has_relic(RLC_BURNINGBLOOD)) heal(6);
            gstate = ST_MAP; return;
        }
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
            int i = deck_browse("UPGRADE WHICH?", 1);
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
    txt_put(4, 5, "FOUND:", CLR_WHITE);
    txt_put(4, 7, "GOLD +", CLR_YELLOW); txt_int(10, 7, gold, CLR_YELLOW);
    if (r != 0xFF) { relic_add(r); txt_put(4, 9, relic_names[r], CLR_CYAN); }
    txt_put(4, 12, "PRESS A", CLR_GRAY);
    sfx_heal();
    for (;;) { vsync(); key_poll(); if (key_hit(KEY_A)) break; }
    gstate = ST_MAP;
}

/* ---------- shop ---------- */

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
    char nb[16];
    int sel = 0;

    for (;;) {
        header("SHOP", CLR_YELLOW);
        ui_fill(0, 2, 30, 9, T_PANEL, CLR_GRAY);
        /* rows: 5 cards, relic, remove, leave */
        for (int i = 0; i < 5; i++) {
            int y = 2 + i;
            CardInst ci = {ids[i], 0};
            int can = !sold[i] && run.gold >= price[i];
            if (sel == i) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
            txt_putc(0, y, sel == i ? '>' : ' ', CLR_YELLOW);
            if (sold[i]) { txt_put(2, y, "SOLD OUT", CLR_GRAY); continue; }
            txt_put(2, y, card_name(ci, nb), can ? CLR_WHITE : CLR_GRAY);
            txt_int(17, y, price[i], can ? CLR_YELLOW : CLR_DKRED);
        }
        if (sel < 5 && !sold[sel])
            txt_put(1, 18, cards[ids[sel]].desc, CLR_GRAY);
        int y = 8;
        if (sel == 5) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 5 ? '>' : ' ', CLR_YELLOW);
        if (rsold) txt_put(2, y, "SOLD OUT", CLR_GRAY);
        else {
            txt_put(2, y, relic_names[relic], run.gold >= rprice ? CLR_CYAN : CLR_GRAY);
            txt_int(17, y, rprice, run.gold >= rprice ? CLR_YELLOW : CLR_DKRED);
        }
        y = 9;
        if (sel == 6) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 6 ? '>' : ' ', CLR_YELLOW);
        txt_put(2, y, removed ? "REMOVED" : "REMOVE CARD", removed ? CLR_GRAY : CLR_WHITE);
        if (!removed) txt_int(17, y, 75, run.gold >= 75 ? CLR_YELLOW : CLR_DKRED);
        y = 10;
        if (sel == 7) ui_fill(0, y, 30, 1, T_PANEL, CLR_BLUE);
        txt_putc(0, y, sel == 7 ? '>' : ' ', CLR_YELLOW);
        txt_put(2, y, "LEAVE", CLR_WHITE);
        txt_put(1, 19, "A:BUY B:LEAVE", CLR_GRAY);

        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_UP) && sel > 0)   { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_DOWN) && sel < 7) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_B)) { obj_hide_all(); gstate = ST_MAP; return; }
            if (key_hit(KEY_A)) {
                if (sel < 5 && !sold[sel] && run.gold >= price[sel]) {
                    run.gold -= price[sel]; deck_add(ids[sel]); sold[sel] = 1; sfx_ok();
                } else if (sel == 5 && !rsold && run.gold >= rprice) {
                    run.gold -= rprice; relic_add(relic); rsold = 1; sfx_ok();
                } else if (sel == 6 && !removed && run.gold >= 75) {
                    int i = deck_browse("REMOVE WHICH?", 1);
                    scene_shop();
                    if (i >= 0) { run.gold -= 75; deck_remove(i); removed = 1; sfx_ok(); }
                } else if (sel == 7) { obj_hide_all(); gstate = ST_MAP; return; }
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
            int i = deck_browse(m == 0 ? "REMOVE WHICH?" : "UPGRADE WHICH?", 1);
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
