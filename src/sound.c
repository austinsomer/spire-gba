#include "game.h"

/* ============================================================
   PSG music driver — 60Hz tracker over the 3 channels sfx
   doesn't own for melody/bass, plus shared noise for drums.
     ch2 pulse  -> lead        (sustained, constant volume)
     ch3 wave   -> bass        (sustained)
     ch4 noise  -> drums       (short envelope, auto-decays)
   sfx keeps ch1 (square) exclusively. sfx_hit() also pokes ch4,
   so an sfx drum-hit briefly stomps a music drum; music retriggers
   on its next drum row. Melody/bass never collide with sfx.
   ============================================================ */

#define N_NOTES 72   /* 6 octaves, note 0 = C2 */

/* equal-tempered semitone freqs for the C2 octave (Hz, rounded);
   higher octaves = base << octave */
static const u16 oct_hz[12] =
    { 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123 };

/* period register values, precomputed per note */
static u16 pulse_per[N_NOTES];   /* freq = 131072/(2048-x) */
static u16 wave_per[N_NOTES];    /* freq = 65536/(2048-x)  */

static u16 clamp_per(int x)
{
    if (x < 1)    x = 1;
    if (x > 2047) x = 2047;
    return (u16)x;
}

void music_init(void)
{
    for (int n = 0; n < N_NOTES; n++) {
        int freq = oct_hz[n % 12] << (n / 12);
        pulse_per[n] = clamp_per(2048 - 131072 / freq);
        wave_per[n]  = clamp_per(2048 - 65536 / freq);
    }
}

/* a soft triangle-ish waveform for the bass (32 nibbles, 16 bytes) */
static const u16 wave_tri[8] =
    { 0x0123, 0x4567, 0x89AB, 0xCDEF, 0xFEDC, 0xBA98, 0x7654, 0x3210 };

static const Song *cur;
static u16 cur_row;
static u8  tick_ctr;
static u8  wave_loaded;

/* ---- per-channel note application ---- */

static void pulse_note(u8 code)
{
    if (code == 0) return;                 /* HOLD */
    if (code == 1) { REG_SOUND2CNT_L = 0; REG_SOUND2CNT_H = 0x8000; return; } /* OFF */
    int n = code - 2;
    if (n >= N_NOTES) return;
    REG_SOUND2CNT_L = 0x0080 | (10 << 12); /* 50% duty, vol 10, no envelope */
    REG_SOUND2CNT_H = (u16)(0x8000 | pulse_per[n]); /* trigger, no length ctr */
}

static void wave_note(u8 code)
{
    if (code == 0) return;                 /* HOLD */
    if (code == 1) { REG_SOUND3CNT_L = 0; return; }  /* OFF: DAC off */
    int n = code - 2;
    if (n >= N_NOTES) return;
    if (!wave_loaded) {
        REG_SOUND3CNT_L = 0x0020;          /* bit5=1: CPU window = bank 0 */
        for (int i = 0; i < 8; i++) MEM_WAVE_RAM[i] = wave_tri[i];
        wave_loaded = 1;
    }
    REG_SOUND3CNT_L = 0x0080;              /* DAC on, play bank 0 */
    REG_SOUND3CNT_H = 0x2000;              /* volume 100% */
    REG_SOUND3CNT_X = (u16)(0x8000 | wave_per[n]); /* trigger, no length */
}

static void noise_drum(u8 code)
{
    switch (code) {
    case 2: /* kick: low, quick decay */
        REG_SOUND4CNT_L = (10 << 12) | (2 << 8);
        REG_SOUND4CNT_H = 0x8000 | 0x0064;
        break;
    case 3: /* snare: mid, metallic */
        REG_SOUND4CNT_L = (9 << 12) | (2 << 8);
        REG_SOUND4CNT_H = 0x8000 | 0x0048;
        break;
    case 4: /* hat: high, very short */
        REG_SOUND4CNT_L = (6 << 12) | (4 << 8);
        REG_SOUND4CNT_H = 0x8000 | 0x0028;
        break;
    default: break;                        /* 0 HOLD / 1 OFF: leave ringing */
    }
}

void music_play(const Song *s)
{
    cur = s;
    cur_row = 0;
    tick_ctr = 1;          /* fire row 0 on next tick */
}

void music_stop(void)
{
    cur = 0;
    REG_SOUND2CNT_L = 0; REG_SOUND2CNT_H = 0x8000;  /* pulse off */
    REG_SOUND3CNT_L = 0;                            /* wave DAC off */
}

/* settings toggle: off silences channels but keeps the song position,
   so re-enabling resumes mid-song (PSG tracker and PCM stream both) */
void music_enable(int on)
{
    opt_music = (u8)on;
    if (!on) {
        REG_SOUND2CNT_L = 0; REG_SOUND2CNT_H = 0x8000;
        REG_SOUND3CNT_L = 0;
    }
    pcm_gate();
}

void music_tick(void)
{
    if (!cur || !opt_music) return;
    if (--tick_ctr) return;
    tick_ctr = cur->ticks_per_row;

    const u8 *r = cur->rows + (u32)cur_row * 3;
    pulse_note(r[0]);
    wave_note(r[1]);
    noise_drum(r[2]);

    if (++cur_row >= cur->nrows) {
        if (cur->loop) cur_row = 0;
        else           music_stop();
    }
}

/* ============================================================
   demo song — I-V-vi-IV loop proving all 3 channels.
   Replace this with converted song data (see pipeline).
   ============================================================ */
#define HOLD 0
#define OFF  1
#define N(oct, semi) (2 + (oct) * 12 + (semi))
#define KICK  2
#define SNARE 3
#define HAT   4

static const u8 demo_rows[] = {
/*  lead(ch2)  bass(ch3)  drum(ch4) */
    N(2,0),    N(0,0),    KICK,    /* C  */
    HOLD,      HOLD,      HAT,
    N(2,4),    HOLD,      SNARE,
    HOLD,      HOLD,      HAT,
    N(2,7),    N(0,7),    KICK,    /* G  */
    HOLD,      HOLD,      HAT,
    N(2,4),    HOLD,      SNARE,
    HOLD,      HOLD,      HAT,
    N(2,9),    N(0,9),    KICK,    /* Am */
    HOLD,      HOLD,      HAT,
    N(2,5),    HOLD,      SNARE,
    HOLD,      HOLD,      HAT,
    N(2,5),    N(0,5),    KICK,    /* F  */
    HOLD,      HOLD,      HAT,
    N(2,7),    HOLD,      SNARE,
    HOLD,      HOLD,      HAT,
};

const Song song_demo = {
    demo_rows,
    sizeof(demo_rows) / 3,
    7,      /* ticks/row ~= 8.5 rows/sec */
    1,      /* loop */
};
