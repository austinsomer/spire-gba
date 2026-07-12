#include "cards.h"
#include "sprites.h"
#include "bgtiles.h"

/* ---------- encounters ---------- */
/* ids match game.h usage: normals, elites, bosses */
enum {
    ENC_CULTIST, ENC_JAWWORM, ENC_LOUSES, ENC_SLIMES, ENC_LOOTER,
    ENC_FUNGI, ENC_SLAVER, N_ENC_NORMAL,
    ENC_NOB = N_ENC_NORMAL, ENC_LAGAVULIN, ENC_SENTRIES, N_ENC_ELITE,
    ENC_SLIMEBOSS = N_ENC_ELITE, ENC_GUARDIAN, ENC_HEXAGHOST, N_ENC
};

/* enemy species */
enum {
    EN_CULTIST, EN_JAWWORM, EN_LOUSE_R, EN_LOUSE_G,
    EN_ACID_M, EN_SPIKE_M, EN_ACID_L, EN_SPIKE_L,
    EN_LOOTER, EN_FUNGI, EN_SLAVER,
    EN_NOB, EN_LAGAVULIN, EN_SENTRY,
    EN_SLIMEBOSS, EN_GUARDIAN, EN_HEXAGHOST,
    N_ENEMIES
};

static const struct { char name[11]; u8 hplo, hphi; } species[N_ENEMIES] = {
    [EN_CULTIST]   = {"CULTIST",   48, 54},
    [EN_JAWWORM]   = {"JAW WORM",  40, 44},
    [EN_LOUSE_R]   = {"RED LOUSE", 10, 15},
    [EN_LOUSE_G]   = {"GRN LOUSE", 11, 17},
    [EN_ACID_M]    = {"ACID SLIME",28, 32},
    [EN_SPIKE_M]   = {"SPIKESLIME",28, 32},
    [EN_ACID_L]    = {"ACID SLM L",65, 69},
    [EN_SPIKE_L]   = {"SPIKESLM L",64, 70},
    [EN_LOOTER]    = {"LOOTER",    44, 48},
    [EN_FUNGI]     = {"FUNGI",     22, 28},
    [EN_SLAVER]    = {"SLAVER",    46, 50},
    [EN_NOB]       = {"GREML NOB", 82, 86},
    [EN_LAGAVULIN] = {"LAGAVULIN", 109,111},
    [EN_SENTRY]    = {"SENTRY",    38, 42},
    [EN_SLIMEBOSS] = {"SLIME BOSS",140,140},
    [EN_GUARDIAN]  = {"GUARDIAN",  240,240},
    [EN_HEXAGHOST] = {"HEXAGHOST", 250,250},
};

/* move kinds + rider effects */
enum { MV_ATK, MV_DEF, MV_BUF, MV_DEBUF, MV_SLEEP };
enum { EF_NONE, EF_WEAK, EF_VULN, EF_SLIMED, EF_DAZED, EF_STR, EF_RITUAL,
       EF_STEAL, EF_BURN, EF_SPLIT, EF_ESCAPE, EF_STRDOWN, EF_THORNS };

typedef struct {
    char name[10];
    u8 kind, dmg, hits, blk, eff, effval;
} Move;

typedef struct {
    u8 id, alive;
    s16 hp, maxhp, block;
    s8 str;
    u8 weak, vuln;
    u8 turn, flag;      /* flag: nob enrage / guardian mode / lagavulin asleep */
    Move mv;
} Enemy;

static Enemy en[MAX_ENEMY];
static int nen;
static int boss_fight;

/* player combat state */
static struct {
    s16 energy, block;
    s8 str, tempstr;
    u8 weak, vuln;
    u8 thorns, metallicize;
    u8 barricade, demonform;
} pl;

int combat_turn;

/* ---------- enemy AI: fill e->mv from id + turn ---------- */

#define M(nm,k,d,h,b,e,v) (Move){nm,k,d,h,b,e,v}

static void roll_move(Enemy *e)
{
    int t = e->turn;
    u32 r = rng();
    switch (e->id) {
    case EN_CULTIST:
        e->mv = (t == 0) ? M("RITUAL", MV_BUF, 0,0,0, EF_RITUAL,3)
                         : M("DARK STK", MV_ATK, 6,1,0, EF_NONE,0);
        break;
    case EN_JAWWORM:
        switch (t == 0 ? 0 : r % 3) {
        case 0: e->mv = M("CHOMP",  MV_ATK, 11,1,0, EF_NONE,0); break;
        case 1: e->mv = M("THRASH", MV_ATK, 7,1,5,  EF_NONE,0); break;
        default:e->mv = M("BELLOW", MV_BUF, 0,0,6,  EF_STR,3);  break;
        }
        break;
    case EN_LOUSE_R:
        e->mv = (r % 4 == 0) ? M("GROW", MV_BUF, 0,0,0, EF_STR,3)
                             : M("BITE", MV_ATK, 6,1,0, EF_NONE,0);
        break;
    case EN_LOUSE_G:
        e->mv = (r % 4 == 0) ? M("SPIT WEB", MV_DEBUF, 0,0,0, EF_WEAK,2)
                             : M("BITE",     MV_ATK, 6,1,0, EF_NONE,0);
        break;
    case EN_ACID_M: case EN_ACID_L:
        switch (r % 3) {
        case 0: e->mv = M("SPIT",   MV_ATK, 8,1,0, EF_SLIMED,1); break;
        case 1: e->mv = M("TACKLE", MV_ATK, 10,1,0,EF_NONE,0);   break;
        default:e->mv = M("LICK",   MV_DEBUF,0,0,0,EF_WEAK,1);   break;
        }
        if (e->id == EN_ACID_L) e->mv.dmg += 3;
        break;
    case EN_SPIKE_M: case EN_SPIKE_L:
        e->mv = (r % 3 == 0) ? M("LICK",   MV_DEBUF,0,0,0, EF_WEAK,1)
                             : M("FLAME TK",MV_ATK, 8,1,0, EF_SLIMED,1);
        if (e->id == EN_SPIKE_L) e->mv.dmg += 4;
        break;
    case EN_LOOTER:
        if (t == 0 || t == 1) e->mv = M("MUG", MV_ATK, 10,1,0, EF_STEAL,15);
        else if (t == 2)      e->mv = M("SMOKE", MV_DEF, 0,0,6, EF_NONE,0);
        else                  e->mv = M("ESCAPE", MV_BUF, 0,0,0, EF_ESCAPE,0);
        break;
    case EN_FUNGI:
        e->mv = (r % 3 == 0) ? M("GROW", MV_BUF, 0,0,0, EF_STR,3)
                             : M("BITE", MV_ATK, 6,1,0, EF_NONE,0);
        break;
    case EN_SLAVER:
        e->mv = (r % 3 == 0) ? M("RAKE", MV_ATK, 7,1,0, EF_WEAK,1)
                             : M("STAB", MV_ATK, 12,1,0, EF_NONE,0);
        break;
    case EN_NOB:
        if (t == 0) e->mv = M("BELLOW", MV_BUF, 0,0,0, EF_NONE,0); /* enrage on */
        else if (r % 3 == 0) e->mv = M("SKULL BSH", MV_ATK, 6,1,0, EF_VULN,2);
        else                 e->mv = M("RUSH",      MV_ATK, 14,1,0, EF_NONE,0);
        break;
    case EN_LAGAVULIN:
        if (e->flag) { e->mv = M("SLEEP", MV_SLEEP, 0,0,0, EF_NONE,0); break; }
        e->mv = (t % 3 == 2) ? M("SIPHON", MV_DEBUF, 0,0,0, EF_STRDOWN,2)
                             : M("ATTACK", MV_ATK, 18,1,0, EF_NONE,0);
        break;
    case EN_SENTRY:
        /* stagger by flag (position): even turns beam, odd bolt */
        e->mv = ((t + e->flag) & 1) ? M("BOLT", MV_DEBUF, 0,0,0, EF_DAZED,1)
                                    : M("BEAM", MV_ATK, 9,1,0, EF_NONE,0);
        break;
    case EN_SLIMEBOSS:
        switch (t % 3) {
        case 0: e->mv = M("GOO SPRAY", MV_DEBUF, 0,0,0, EF_SLIMED,3); break;
        case 1: e->mv = M("PREPARING", MV_DEF, 0,0,0, EF_NONE,0);     break;
        default:e->mv = M("SLAM",      MV_ATK, 35,1,0, EF_NONE,0);    break;
        }
        break;
    case EN_GUARDIAN:
        if (e->flag) { /* defensive mode: 2 turns thorns then back */
            e->mv = M("SHARPHIDE", MV_DEF, 0,0,9, EF_THORNS,3);
            if (e->flag++ >= 3) e->flag = 0;
        } else switch (t % 3) {
        case 0: e->mv = M("CHARGE UP", MV_DEF, 0,0,9, EF_NONE,0);  break;
        case 1: e->mv = M("FRC BASH",  MV_ATK, 32,1,0, EF_NONE,0); break;
        default:e->mv = M("SWIPE",     MV_ATK, 8,2,0, EF_NONE,0);  break;
        }
        break;
    case EN_HEXAGHOST:
        switch (t % 6) {
        case 0: e->mv = M("ACTIVATE", MV_BUF, 0,0,0, EF_NONE,0);   break;
        case 1: e->mv = M("DIVIDER",  MV_ATK, 6,2,0, EF_NONE,0);   break;
        case 2: e->mv = M("SEAR",     MV_ATK, 6,1,0, EF_BURN,1);   break;
        case 3: e->mv = M("TACKLE",   MV_ATK, 5,2,0, EF_NONE,0);   break;
        case 4: e->mv = M("INFLAME",  MV_BUF, 0,0,12, EF_STR,2);   break;
        default:e->mv = M("INFERNO",  MV_ATK, 6,2,0, EF_BURN,2);   break;
        }
        break;
    }
}

static void spawn(int slot, u8 id)
{
    Enemy *e = &en[slot];
    e->id = id; e->alive = 1;
    e->maxhp = e->hp = species[id].hplo +
        rng_range(species[id].hphi - species[id].hplo + 1);
    e->block = 0; e->str = 0; e->weak = e->vuln = 0;
    e->turn = 0; e->flag = 0;
    if (id == EN_LAGAVULIN) { e->flag = 1; e->block = 8; } /* asleep */
    if (id == EN_SENTRY) e->flag = slot;
    roll_move(e);
}

static void setup_encounter(int enc)
{
    nen = 0; boss_fight = (enc >= ENC_SLIMEBOSS);
    switch (enc) {
    case ENC_CULTIST:  spawn(nen++, EN_CULTIST); break;
    case ENC_JAWWORM:  spawn(nen++, EN_JAWWORM); break;
    case ENC_LOUSES:   spawn(nen++, EN_LOUSE_R); spawn(nen++, EN_LOUSE_G); break;
    case ENC_SLIMES:   spawn(nen++, EN_ACID_M);  spawn(nen++, EN_SPIKE_M); break;
    case ENC_LOOTER:   spawn(nen++, EN_LOOTER); break;
    case ENC_FUNGI:    spawn(nen++, EN_FUNGI);   spawn(nen++, EN_FUNGI); break;
    case ENC_SLAVER:   spawn(nen++, EN_SLAVER); break;
    case ENC_NOB:      spawn(nen++, EN_NOB); break;
    case ENC_LAGAVULIN:spawn(nen++, EN_LAGAVULIN); break;
    case ENC_SENTRIES: spawn(nen++, EN_SENTRY); spawn(nen++, EN_SENTRY);
                       spawn(nen++, EN_SENTRY); break;
    case ENC_SLIMEBOSS:spawn(nen++, EN_SLIMEBOSS); break;
    case ENC_GUARDIAN: spawn(nen++, EN_GUARDIAN); break;
    case ENC_HEXAGHOST:spawn(nen++, EN_HEXAGHOST); break;
    default:           spawn(nen++, EN_CULTIST); break;
    }
}

static int enemies_alive(void)
{
    int n = 0;
    for (int i = 0; i < nen; i++) if (en[i].alive) n++;
    return n;
}

static int first_alive(void)
{
    for (int i = 0; i < nen; i++) if (en[i].alive) return i;
    return -1;
}

/* ---------- damage ---------- */

static void hurt_player(int dmg)
{
    if (dmg <= 0) return;
    if (pl.block >= dmg) { pl.block -= dmg; sfx_block(); return; }
    dmg -= pl.block; pl.block = 0;
    run.hp -= dmg;
    sfx_hit();
}

static void enemy_attack_player(Enemy *e)
{
    int dmg = e->mv.dmg + e->str;
    if (e->weak) dmg = dmg * 3 / 4;
    if (pl.vuln) dmg = dmg * 3 / 2;
    if (dmg < 0) dmg = 0;
    for (int h = 0; h < e->mv.hits && run.hp > 0; h++) {
        hurt_player(dmg);
        if (pl.thorns && e->alive) {
            e->block = 0; /* thorns pierce for simplicity */
            e->hp -= pl.thorns;
            if (e->hp <= 0) { e->hp = 0; e->alive = 0; }
        }
    }
}

/* returns dmg dealt to hp (for feed) */
static void hit_enemy(Enemy *e, int dmg)
{
    if (dmg <= 0) return;
    if (e->block >= dmg) { e->block -= dmg; sfx_block(); return; }
    dmg -= e->block; e->block = 0;
    e->hp -= dmg;
    sfx_hit();
    if (e->id == EN_LAGAVULIN && e->flag) { e->flag = 0; roll_move(e); }
    if (e->hp <= 0) { e->hp = 0; e->alive = 0; }
}

static void attack_enemy(Enemy *e, CardInst c, int per_hit_bonus_mult)
{
    int base = card_dmg(c);
    int dmg;
    if (cards[c.id].sp == SP_HEAVYBLADE)
        dmg = base + pl.str * card_spval(c);
    else if (cards[c.id].sp == SP_BODYSLAM)
        dmg = pl.block;
    else
        dmg = base + pl.str;
    (void)per_hit_bonus_mult;
    if (pl.weak) dmg = dmg * 3 / 4;
    if (e->vuln) dmg = dmg * 3 / 2;
    if (dmg < 0) dmg = 0;
    for (int h = 0; h < cards[c.id].hits && e->alive; h++)
        hit_enemy(e, dmg);
}

/* guardian mode shift check + slime split, after damage */
static void post_damage_checks(void)
{
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        if (!e->alive) continue;
        if (e->id == EN_SLIMEBOSS && e->hp <= e->maxhp / 2) {
            /* split into large acid + spike at current hp */
            s16 hp = e->hp;
            e->alive = 0;
            nen = 0;
            spawn(nen++, EN_ACID_L);  en[0].hp = en[0].maxhp = hp;
            spawn(nen++, EN_SPIKE_L); en[1].hp = en[1].maxhp = hp;
            sfx_bad();
        }
    }
}

/* ---------- card effects ---------- */

static int play_card(int hi, int target)
{
    CardInst c = piles.hand[hi];
    const Card *cd = &cards[c.id];
    int cost = card_cost(c);
    int xspent = 0;

    if (cd->sp == SP_UNPLAYABLE) { sfx_bad(); return 0; }
    if (cd->cost == COST_X) { xspent = pl.energy; cost = pl.energy; }
    if (cost > pl.energy) { sfx_bad(); return 0; }
    pl.energy -= cost;

    /* damage */
    if (cd->type == CT_ATTACK) {
        if (cd->target == TGT_ALL) {
            int times = (cd->sp == SP_WHIRLWIND) ? xspent : 1;
            for (int t = 0; t < times; t++)
                for (int i = 0; i < nen; i++)
                    if (en[i].alive) attack_enemy(&en[i], c, 1);
        } else if (target >= 0 && en[target].alive) {
            attack_enemy(&en[target], c, 1);
        }
    }

    /* block */
    int blk = card_block(c);
    if (blk) { pl.block += blk; sfx_block(); }

    /* specials */
    int sv = card_spval(c);
    Enemy *tg = (target >= 0) ? &en[target] : 0;
    switch (cd->sp) {
    case SP_VULN:
        if (cd->target == TGT_ALL) {
            for (int i = 0; i < nen; i++) if (en[i].alive) en[i].vuln += sv;
        } else if (tg) tg->vuln += sv;
        break;
    case SP_WEAK: if (tg) tg->weak += sv; break;
    case SP_ANGER: pile_add_discard(C_ANGER); break;
    case SP_FLEX: pl.str += sv; pl.tempstr += sv; break;
    case SP_HEADBUTT:
        if (piles.ndiscard > 0 && piles.ndraw < MAX_DECK) {
            /* last discarded -> top of draw */
            piles.draw[piles.ndraw++] = piles.discard[--piles.ndiscard];
        }
        break;
    case SP_BLOODLET: run.hp -= 3; pl.energy += sv; break;
    case SP_DISARM: if (tg) tg->str -= sv; break;
    case SP_ENTRENCH: pl.block *= 2; break;
    case SP_FLAMEBARRIER: pl.thorns += sv; break;
    case SP_STR: pl.str += sv; break;
    case SP_METALLICIZE: pl.metallicize += sv; break;
    case SP_SHOCKWAVE:
        for (int i = 0; i < nen; i++)
            if (en[i].alive) { en[i].weak += sv; en[i].vuln += sv; }
        break;
    case SP_UPPERCUT: if (tg) { tg->weak += sv; tg->vuln += sv; } break;
    case SP_BARRICADE: pl.barricade = 1; break;
    case SP_DEMONFORM: pl.demonform += sv; break;
    case SP_FEED:
        if (tg && !tg->alive) { run.maxhp += sv; run.hp += sv; sfx_heal(); }
        break;
    case SP_IMMOLATE: pile_add_discard(C_BURN); break;
    case SP_OFFERING: run.hp -= 6; pl.energy += 2; pile_draw(sv); break;
    }

    /* draw rider */
    if (card_draw(c)) pile_draw(card_draw(c));

    post_damage_checks();

    /* move card out of hand */
    if (cd->exhaust) pile_exhaust_card(hi);
    else pile_discard_card(hi);
    sfx_ok();
    return 1;
}

/* ---------- enemy turn ---------- */

static void enemy_turn(void)
{
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        if (!e->alive) continue;
        e->block = 0;
        Move *m = &e->mv;
        if (m->kind == MV_SLEEP) { e->block = 8; e->turn++; roll_move(e); continue; }
        if (m->kind == MV_ATK) enemy_attack_player(e);
        if (!e->alive) continue;          /* died to thorns */
        if (m->blk) e->block += m->blk;
        switch (m->eff) {
        case EF_WEAK:  pl.weak += m->effval; break;
        case EF_VULN:  pl.vuln += m->effval; break;
        case EF_SLIMED: for (int k = 0; k < m->effval; k++) pile_add_discard(C_SLIMED); break;
        case EF_DAZED:  for (int k = 0; k < m->effval; k++) pile_add_discard(C_DAZED); break;
        case EF_BURN:   for (int k = 0; k < m->effval; k++) pile_add_discard(C_BURN); break;
        case EF_STR:    e->str += m->effval; break;
        case EF_RITUAL: e->flag = m->effval; break;   /* str/turn from now */
        case EF_STEAL:
            run.gold -= m->effval; if (run.gold < 0) run.gold = 0; break;
        case EF_ESCAPE: e->alive = 0; break;
        case EF_STRDOWN: pl.str -= m->effval; break;
        case EF_THORNS: break; /* handled as flat block in move */
        }
        if (e->id == EN_CULTIST && e->flag && e->turn > 0) e->str += e->flag;
        if (e->id == EN_GUARDIAN && !e->flag &&
            e->hp <= e->maxhp - 60 && e->turn >= 2) e->flag = 1;
        e->turn++;
        if (run.hp <= 0) return;
        roll_move(e);
    }
}

/* ---------- UI ---------- */


/* species -> sprite id (sprites.h enum order) */
static const u8 species_spr[N_ENEMIES] = {
    [EN_CULTIST] = SPR_CULTIST, [EN_JAWWORM] = SPR_JAWWORM,
    [EN_LOUSE_R] = SPR_LOUSE_R, [EN_LOUSE_G] = SPR_LOUSE_G,
    [EN_ACID_M] = SPR_ACIDSLIME, [EN_ACID_L] = SPR_ACIDSLIME,
    [EN_SPIKE_M] = SPR_SPIKESLIME, [EN_SPIKE_L] = SPR_SPIKESLIME,
    [EN_LOOTER] = SPR_LOOTER, [EN_FUNGI] = SPR_FUNGI,
    [EN_SLAVER] = SPR_SLAVER, [EN_NOB] = SPR_NOB,
    [EN_LAGAVULIN] = SPR_LAGAVULIN, [EN_SENTRY] = SPR_SENTRY,
    [EN_SLIMEBOSS] = SPR_SLIMEBOSS, [EN_GUARDIAN] = SPR_GUARDIAN,
    [EN_HEXAGHOST] = SPR_HEXAGHOST,
};

/* battle stage: player sprite left (oam 5), enemies right (oam 0..2).
   sprites stand at y=72 (feet on floor row 13). enemy i at px 128+i*36. */
#define EN_X(i) (128 + (i) * 40)
#define STAGE_Y 80
#define OAM_PLAYER 5
#define OAM_SHADOW0 6           /* 6..9: player + 3 enemy shadows */

/* framed hp bar with centered numbers over it */
static void hp_bar_top(int x, int w, int hp, int maxhp)
{
    ui_bar(x, 0, w, hp, maxhp, CLR_RED);
    char b[10]; int i = 0, v = hp;
    /* build "hp/max" */
    char tmp[10]; int n = 0;
    do { tmp[n++] = '0' + v % 10; v /= 10; } while (v);
    while (n) b[i++] = tmp[--n];
    b[i++] = '/'; v = maxhp; n = 0;
    do { tmp[n++] = '0' + v % 10; v /= 10; } while (v);
    while (n) b[i++] = tmp[--n];
    b[i] = 0;
    txt_put(x + (w - i) / 2, 0, b, CLR_WHITE);
}

static void status_line(int *y, int icon, const char *label, int val, int clr)
{
    if (val <= 0) return;
    ui_icon(0, *y, icon);
    int xx = 2;
    txt_put(xx, *y, label, clr);
    while (label[xx - 2]) xx++;
    txt_int_at(xx, *y, val, clr);
    (*y)++;
}

static void draw_enemies(int cursor, int targeting)
{
    char b[12];
    int focus = targeting ? cursor : first_alive();
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i];
        int cx = EN_X(i) / 8;
        if (!e->alive) { obj_hide(i); obj_hide(OAM_SHADOW0 + 1 + i); continue; }
        obj_show(OAM_SHADOW0 + 1 + i, SPR_SHADOW, EN_X(i), STAGE_Y + 8);
        obj_show(i, species_spr[e->id], EN_X(i), STAGE_Y);
        /* intent above sprite: icon + numbers */
        int iy = 7;
        if (e->mv.kind == MV_ATK) {
            int dmg = e->mv.dmg + e->str;
            if (e->weak) dmg = dmg * 3 / 4;
            if (pl.vuln) dmg = dmg * 3 / 2;
            if (dmg < 0) dmg = 0;
            ui_icon(cx, iy, TI_ATK);
            int xx = txt_int_at(cx + 1, iy, dmg, CLR_ORANGE);
            if (e->mv.hits > 1) {
                txt_putc(xx, iy, 'X', CLR_ORANGE);
                txt_int(xx + 1, iy, e->mv.hits, CLR_ORANGE);
            }
        } else if (e->mv.kind == MV_DEF)  ui_icon(cx + 1, iy, TI_DEF);
        else if (e->mv.kind == MV_BUF)    ui_icon(cx + 1, iy, TI_BUFF);
        else if (e->mv.kind == MV_SLEEP)  txt_put(cx, iy, "ZZZ", CLR_GRAY);
        else                              ui_icon(cx + 1, iy, TI_DEBUFF);
        if (targeting && i == cursor) txt_putc(cx - 1, 7, '>', CLR_YELLOW);
        /* mini hp bar under sprite; statuses above intent */
        ui_bar(cx, 13, 4, e->hp, e->maxhp, CLR_RED);
        int sx = cx;
        if (e->block) { txt_putc(sx, 6, 'B', CLR_BLUE); sx = txt_int_at(sx+1, 6, e->block, CLR_BLUE); }
        if (e->str > 0) { txt_putc(sx, 6, 'S', CLR_RED); sx = txt_int_at(sx+1, 6, e->str, CLR_RED); }
        if (e->weak)  { txt_putc(sx, 6, 'W', CLR_GREEN); sx = txt_int_at(sx+1, 6, e->weak, CLR_GREEN); }
        if (e->vuln)  { txt_putc(sx, 6, 'V', CLR_PURPLE); sx = txt_int_at(sx+1, 6, e->vuln, CLR_PURPLE); }
        (void)b;
    }
    for (int i = nen; i < 3; i++) { obj_hide(i); obj_hide(OAM_SHADOW0 + 1 + i); }
    /* top-right framed bar + name for focused enemy */
    if (focus >= 0) {
        hp_bar_top(20, 8, en[focus].hp, en[focus].maxhp);
        txt_put(30 - 9, 12, species[en[focus].id].name,
                targeting ? CLR_YELLOW : CLR_WHITE);
    }
}

static void draw_player(void)
{
    obj_show(OAM_SHADOW0, SPR_SHADOW, 24, STAGE_Y + 8);
    obj_show(OAM_PLAYER, SPR_IRONCLAD, 24, STAGE_Y);
    hp_bar_top(2, 8, run.hp, run.maxhp);
    /* status lines with icons, from row 1 */
    int y = 1;
    status_line(&y, TI_DEF,    "BLOCK ",    pl.block, CLR_BLUE);
    status_line(&y, TI_BUFF,   "STRENGTH+", pl.str, CLR_YELLOW);
    status_line(&y, TI_DEBUFF, "WEAK ",     pl.weak, CLR_GREEN);
    status_line(&y, TI_DEBUFF, "VULN ",     pl.vuln, CLR_PURPLE);
    if (y == 1) y++;
    status_line(&y, TI_BUFF,   "THORNS ",   pl.thorns, CLR_ORANGE);
    status_line(&y, TI_BUFF,   "METALLIC ", pl.metallicize, CLR_CYAN);
    /* energy orb bottom-left */
    ui_tile(0, 17, T_ENERGY, CLR_YELLOW);
    int xx = txt_int_at(0, 18, pl.energy, CLR_YELLOW);
    txt_putc(xx, 18, '/', CLR_GRAY);
    txt_putc(xx + 1, 18, '3', CLR_GRAY);
    /* draw/discard piles tiny, bottom-left under orb */
    xx = txt_int_at(0, 19, piles.ndraw, CLR_CYAN);
    txt_putc(xx, 19, '+', CLR_GRAY);
    txt_int(xx + 1, 19, piles.ndiscard, CLR_ORANGE);
}

/* big card faces: 4 visible slots, 7 cols x 5 rows at rows 15-19 */
static const int card_frame_clr[4] = {CLR_RED, CLR_GREEN, CLR_CYAN, CLR_GRAY};

static int card_art(const Card *cd)
{
    switch (cd->type) {
    case CT_ATTACK: return TB_ART_STRIKE;
    case CT_POWER:  return TB_ART_POWER;
    case CT_STATUS: return TB_ART_STATUS;
    default:        return cd->block ? TB_ART_DEFEND : TB_ART_SKILL;
    }
}

static void draw_hand(int sel)
{
    char nb[16];
    int top = 0;
    if (sel >= 4) top = sel - 3;
    for (int s = 0; s < 4 && top + s < piles.nhand; s++) {
        int i = top + s, x = 2 + s * 7;
        CardInst c = piles.hand[i];
        const Card *cd = &cards[c.id];
        int hi = (i == sel);
        int can = card_cost(c) <= pl.energy && cd->sp != SP_UNPLAYABLE;
        if (cd->cost == COST_X) can = 1;
        int fr = hi ? CLR_YELLOW : (can ? card_frame_clr[cd->type] : CLR_GRAY);
        ui_box(x, 15, 7, 5, fr);
        /* face fill (panel of frame color) + banner; leave a 2x2 window
           at the art cells so the BG2 illustration shows through */
        for (int cy = 16; cy <= 18; cy++)
            for (int cx = x + 1; cx <= x + 5; cx++) {
                if (cy >= 16 && cy <= 17 && cx >= x + 2 && cx <= x + 3) {
                    ui_tile(cx, cy, 0, 0);   /* clear ui_box interior fill
                                                so BG2 card art shows */
                    continue;
                }
                ui_fill(cx, cy, 1, 1, T_PANEL,
                        can ? card_frame_clr[cd->type] : CLR_GRAY);
            }
        ui_fill(x + 1, 15, 5, 1, T_PANEL, hi ? CLR_BLUE : CLR_GRAY);
        /* name (5 chars) on banner */
        card_name(c, nb);
        for (int k = 0; k < 5 && nb[k]; k++)
            txt_putc(x + 1 + k, 15, nb[k],
                     hi ? CLR_WHITE : (c.up ? CLR_GREEN : CLR_WHITE));
        /* cost orb top-left corner */
        ui_tile(x, 15, T_ENERGY, CLR_YELLOW);
        if (cd->cost == COST_X) txt_putc(x, 15, 'X', CLR_WHITE);
        else txt_int(x, 15, card_cost(c), can ? CLR_WHITE : CLR_DKRED);
        /* 16x16 art centered on face (BG2 stamp over battle bg) */
        {
            int a = card_art(cd);
            bg2_stamp(x + 2, 16, a,     13, 0);
            bg2_stamp(x + 3, 16, a + 1, 13, 0);
            bg2_stamp(x + 2, 17, a + 2, 13, 0);
            bg2_stamp(x + 3, 17, a + 3, 13, 0);
        }
        /* dmg/blk bottom row of face */
        if (card_dmg(c) || cd->sp == SP_BODYSLAM) {
            int d = cd->sp == SP_BODYSLAM ? pl.block
                  : card_dmg(c) + (cd->sp == SP_HEAVYBLADE ? pl.str * card_spval(c) : pl.str);
            txt_int(x + 3, 18, d, CLR_WHITE);
        } else if (card_block(c)) txt_int(x + 3, 18, card_block(c), CLR_WHITE);
    }
    if (piles.nhand == 0) txt_put(12, 17, "NO CARDS", CLR_GRAY);
    if (top > 0) txt_putc(1, 17, '<', CLR_YELLOW);
    if (top + 4 < piles.nhand) txt_putc(29, 17, '>', CLR_YELLOW);
}

static void draw_all(int sel, int cursor, int targeting)
{
    txt_clear(); ui_clear();
    scene_battle();                 /* restore bg (card art stamps over it) */
    draw_enemies(cursor, targeting);
    draw_player();
    draw_hand(sel);
    /* row 13: selected card full name + desc, left side */
    if (piles.nhand > 0 && sel < piles.nhand) {
        char nb[16];
        card_name(piles.hand[sel], nb);
        txt_put(0, 12, nb, CLR_WHITE);
    }
    txt_put(0, 19, targeting ? "A:GO B:NO" : "", CLR_GRAY);
    txt_put(24, 19, "R:END", CLR_GRAY);
}

/* ---------- start / end of turn ---------- */

static void start_player_turn(void)
{
    combat_turn++;
    if (!pl.barricade) pl.block = 0;
    pl.energy = 3;
    if (pl.demonform) pl.str += pl.demonform;
    pile_draw(5);
}

static void end_player_turn(void)
{
    /* burns in hand */
    for (int i = 0; i < piles.nhand; i++)
        if (piles.hand[i].id == C_BURN) hurt_player(2);
    /* flex wears off */
    pl.str -= pl.tempstr; pl.tempstr = 0;
    if (pl.metallicize) pl.block += pl.metallicize;
    pile_discard_hand();
    if (pl.weak) pl.weak--;
    if (pl.vuln) pl.vuln--;
}

static void end_enemy_turn(void)
{
    for (int i = 0; i < nen; i++) {
        if (!en[i].alive) continue;
        if (en[i].weak) en[i].weak--;
        if (en[i].vuln) en[i].vuln--;
    }
}

/* ---------- main combat loop ---------- */

void combat_screen(int encounter)
{
    battle_bg_load();
    scene_battle();
    setup_encounter(encounter);
    piles_init();
    pl.energy = 0; pl.block = 0; pl.str = pl.tempstr = 0;
    pl.weak = pl.vuln = 0; pl.thorns = pl.metallicize = 0;
    pl.barricade = pl.demonform = 0;
    combat_turn = 0;
    combat_won = 0;

    /* relics: combat start */
    if (run_has_relic(RLC_VAJRA)) pl.str += 1;
    if (run_has_relic(RLC_BRONZESCALES)) pl.thorns += 3;
    if (run_has_relic(RLC_BLOODVIAL)) {
        run.hp += 2; if (run.hp > run.maxhp) run.hp = run.maxhp;
    }

    start_player_turn();
    /* relics: first turn */
    if (run_has_relic(RLC_ANCHOR)) pl.block += 10;
    if (run_has_relic(RLC_LANTERN)) pl.energy += 1;
    if (run_has_relic(RLC_BAGPREP)) pile_draw(2);

    int sel = 0;

    for (;;) {
        if (sel >= piles.nhand) sel = piles.nhand ? piles.nhand - 1 : 0;
        draw_all(sel, -1, 0);

        int act = 0; /* 0 none, 1 play, 2 end turn */
#ifdef AUTOPLAY
        {
            for (int f = 0; f < 15; f++) vsync();
            act = 2;
            for (int i = 0; i < piles.nhand; i++) {
                CardInst c = piles.hand[i];
                if (cards[c.id].sp == SP_UNPLAYABLE) continue;
                if (cards[c.id].cost != COST_X && card_cost(c) > pl.energy) continue;
                if (cards[c.id].cost == COST_X && pl.energy == 0) continue;
                sel = i; act = 1; break;
            }
        }
        if (0)
#endif
        for (;;) {
            vsync(); key_poll();
            if (key_repeat(KEY_LEFT) && sel > 0) { sel--; sfx_blip(); break; }
            if (key_repeat(KEY_RIGHT) && sel < piles.nhand - 1) { sel++; sfx_blip(); break; }
            if (key_hit(KEY_A) && piles.nhand > 0) { act = 1; break; }
            if (key_hit(KEY_R)) { act = 2; break; }
            if (key_hit(KEY_B)) {
                obj_hide_all(); scene_none();
                deck_browse("DECK", 0);
                break;
            }
        }

        if (act == 1) {
            CardInst c = piles.hand[sel];
            int target = -1;
            if (cards[c.id].target == TGT_ENEMY) {
                if (enemies_alive() == 1) target = first_alive();
                else {
                    /* target picker */
                    target = first_alive();
                    int done = 0;
#ifdef AUTOPLAY
                    done = 1;
#endif
                    while (!done) {
                        draw_all(sel, target, 1);
                        for (;;) {
                            vsync(); key_poll();
                            if (key_hit(KEY_LEFT)) {
                                for (int i = target - 1; i >= 0; i--)
                                    if (en[i].alive) { target = i; break; }
                                sfx_blip(); break;
                            }
                            if (key_hit(KEY_RIGHT)) {
                                for (int i = target + 1; i < nen; i++)
                                    if (en[i].alive) { target = i; break; }
                                sfx_blip(); break;
                            }
                            if (key_hit(KEY_A)) { done = 1; break; }
                            if (key_hit(KEY_B)) { done = 2; break; }
                        }
                    }
                    if (done == 2) continue;
                }
            }
            play_card(sel, target);
        } else if (act == 2) {
            end_player_turn();
            draw_all(sel, -1, 0);
            /* brief pause so player sees enemy act */
            for (int f = 0; f < 20; f++) vsync();
            enemy_turn();
            end_enemy_turn();
            if (run.hp > 0) start_player_turn();
        }

        /* outcomes */
        if (run.hp <= 0) {
            run.hp = 0;
            obj_hide_all();
            gstate = ST_GAMEOVER;
            return;
        }
        if (enemies_alive() == 0) {
            combat_won = 1;
            obj_hide_all();
            gstate = boss_fight ? ST_VICTORY : ST_REWARD;
            return;
        }
    }
}
