#ifndef CARDS_H
#define CARDS_H

#include "game.h"

/* card ids */
enum {
    /* starters */
    C_STRIKE, C_DEFEND, C_BASH,
    /* commons */
    C_ANGER, C_BODYSLAM, C_CLEAVE, C_CLOTHESLINE, C_FLEX, C_HEADBUTT,
    C_HEAVYBLADE, C_IRONWAVE, C_POMMEL, C_SHRUG, C_THUNDERCLAP, C_TWINSTRIKE,
    /* uncommons */
    C_BLOODLET, C_CARNAGE, C_DISARM, C_ENTRENCH, C_FLAMEBARRIER,
    C_INFLAME, C_METALLICIZE, C_SHOCKWAVE, C_UPPERCUT, C_WHIRLWIND,
    /* rares */
    C_BARRICADE, C_BLUDGEON, C_DEMONFORM, C_FEED, C_IMMOLATE,
    C_IMPERVIOUS, C_OFFERING,
    N_REAL_CARDS,          /* == 32 */
    /* statuses (not in reward pools) */
    C_WOUND = N_REAL_CARDS, C_BURN, C_SLIMED, C_DAZED,
    N_CARDS
};

/* card type */
enum { CT_ATTACK, CT_SKILL, CT_POWER, CT_STATUS };
/* rarity */
enum { CR_STARTER, CR_COMMON, CR_UNCOMMON, CR_RARE, CR_STATUS };
/* target */
enum { TGT_NONE, TGT_ENEMY, TGT_ALL };
/* special effects */
enum {
    SP_NONE,
    SP_VULN,          /* apply spval vulnerable to target(s) */
    SP_WEAK,          /* apply spval weak */
    SP_ANGER,         /* copy self to discard */
    SP_BODYSLAM,      /* dmg = current block */
    SP_FLEX,          /* +spval str, lose at end of turn */
    SP_HEADBUTT,      /* discard card -> top of draw */
    SP_HEAVYBLADE,    /* str counts spval x */
    SP_BLOODLET,      /* lose 3 hp, +spval energy */
    SP_ETHEREAL,      /* exhaust at end of turn if in hand */
    SP_DISARM,        /* target -spval str */
    SP_ENTRENCH,      /* double block */
    SP_FLAMEBARRIER,  /* thorns spval this combat */
    SP_STR,           /* power: +spval str */
    SP_METALLICIZE,   /* power: spval block end of turn */
    SP_SHOCKWAVE,     /* all: spval weak + spval vuln */
    SP_UPPERCUT,      /* spval weak + spval vuln to target */
    SP_WHIRLWIND,     /* X cost: dmg all, X times */
    SP_BARRICADE,     /* power: block persists */
    SP_DEMONFORM,     /* power: +spval str each turn */
    SP_FEED,          /* on kill: +spval maxhp */
    SP_IMMOLATE,      /* add Burn to discard */
    SP_OFFERING,      /* lose 6 hp, +2 energy, draw spval */
    SP_BURN,          /* status: 2 dmg at end of turn */
    SP_UNPLAYABLE,
};

#define COST_X 15

typedef struct {
    char name[13];
    u8 cost   : 4;   /* COST_X = X cost */
    u8 upcost : 4;
    u8 type   : 2;
    u8 rarity : 3;
    u8 target : 2;
    u8 exhaust: 1;
    u8 dmg, updmg;
    u8 hits;         /* multi-hit count, 1 default */
    u8 block, upblock;
    u8 draw, updraw;
    u8 sp;
    u8 spval, upspval;
    char desc[22];   /* short effect text, numbers rendered live */
} Card;

extern const Card cards[N_CARDS];

/* values for an instance (upgrade-aware) */
int card_cost(CardInst c);
int card_dmg(CardInst c);
int card_block(CardInst c);
int card_draw(CardInst c);
int card_spval(CardInst c);
/* name with + suffix into buf (>=14 bytes), returns buf */
const char *card_name(CardInst c, char *buf);

/* ---------- combat piles ---------- */
typedef struct {
    CardInst draw[MAX_DECK], discard[MAX_DECK], exhaust[MAX_DECK];
    CardInst hand[MAX_HAND];
    u8 ndraw, ndiscard, nexhaust, nhand;
} Piles;

extern Piles piles;

void piles_init(void);                 /* from run.deck, shuffled */
int  pile_draw(int n);                 /* draw n, reshuffle as needed; returns drawn */
void pile_discard_hand(void);          /* end of turn (ethereal → exhaust) */
void pile_discard_card(int hand_i);    /* play → discard */
void pile_exhaust_card(int hand_i);
void pile_add_discard(u8 card_id);     /* Anger/Immolate etc */
void pile_shuffle_draw(void);

/* deck browser: shows run.deck. pick_mode 1 → returns index or -1 (B).
   pick_mode 0 → view only, returns -1 */
int deck_browse(const char *title, int pick_mode);

#endif
