/* ============================================================
   PCM music streaming — MP2K-style DirectSound, minus the mixer.

   One looping 8-bit signed mono track streams from ROM into FIFO A:
   timer 0 clocks the FIFO at 13379 Hz (16777216/1254), DMA1 in
   special/repeat mode refills it with zero CPU work. 13379 Hz is
   vblank-locked: exactly 224 samples/frame, so pcm_tick() counts
   position per vsync and restarts the DMA at the loop point (we're
   inside vblank right after the swi 0x05 wait, so the restart is
   glitch-free). PSG sfx (ch1/ch4) mix on top in hardware.

   Track data: tools/mkpcm.py -> assets/music PCM files + src/pcmdata.s
   (.incbin; lengths are multiples of 224, which is also 16-aligned
   for the 4-word DMA bursts).
   ============================================================ */
#include "game.h"

#define PCM_TIMER   (65536 - 1254)   /* 16777216/1254 = 13379.4 Hz */
#define PCM_PERFRAME 224

extern const s8  pcm_title[],  pcm_run[];
extern const u32 pcm_title_len, pcm_run_len;

static const s8 *cur_pcm;
static u32 cur_len, pos;
static s8  cur_trk = -1;
static u8  playing;

static void dma_start(const s8 *src)
{
    REG_DMA1CNT_H = 0;
    REG_DMA1SAD = (u32)src;
    REG_DMA1DAD = FIFO_A_ADDR;
    /* enable | start on FIFO request | 32-bit | repeat | dst fixed */
    REG_DMA1CNT_H = 0xB640;
}

void pcm_play(int trk)
{
    if (trk == cur_trk && playing) return;
    cur_trk = (s8)trk;
    cur_pcm = trk == PCM_TITLE ? pcm_title : pcm_run;
    cur_len = trk == PCM_TITLE ? pcm_title_len : pcm_run_len;
    pos = 0;
    playing = 1;
    if (!opt_music) return;             /* starts when re-enabled */
    REG_SOUNDCNT_H |= 0x0800;           /* FIFO A reset */
    REG_TM0CNT_H = 0;
    REG_TM0CNT_L = PCM_TIMER;
    REG_TM0CNT_H = 0x0080;              /* enable, prescale 1 */
    dma_start(cur_pcm);
}

void pcm_stop(void)
{
    playing = 0;
    cur_trk = -1;
    REG_DMA1CNT_H = 0;
}

/* opt_music flipped in the settings menu */
void pcm_gate(void)
{
    if (!playing) return;
    if (opt_music) {
        REG_SOUNDCNT_H |= 0x0800;
        REG_TM0CNT_H = 0;
        REG_TM0CNT_L = PCM_TIMER;
        REG_TM0CNT_H = 0x0080;
        dma_start(cur_pcm + pos);       /* resume where it paused */
    } else {
        REG_DMA1CNT_H = 0;
    }
}

/* ---- one-shot SFX samples on FIFO B (timer 1 + DMA2) ----
   Impact sounds synthesized by tools/mksfx.py. Frame-counted stop; the
   samples end in 2 frames of silence so the overrun is inaudible. */
extern const s8 sfxpcm_hit[], sfxpcm_clang[], sfxpcm_coin[], sfxpcm_slash[],
                sfxpcm_heal[], sfxpcm_card[];
extern const u32 sfxpcm_hit_len, sfxpcm_clang_len, sfxpcm_coin_len,
                 sfxpcm_slash_len, sfxpcm_heal_len, sfxpcm_card_len;

static u32 sfxb_left;

void sfxpcm_play(int id)
{
    const s8 *src; u32 len;
    switch (id) {
    default:
    case SFXP_HIT:   src = sfxpcm_hit;   len = sfxpcm_hit_len;   break;
    case SFXP_CLANG: src = sfxpcm_clang; len = sfxpcm_clang_len; break;
    case SFXP_COIN:  src = sfxpcm_coin;  len = sfxpcm_coin_len;  break;
    case SFXP_SLASH: src = sfxpcm_slash; len = sfxpcm_slash_len; break;
    case SFXP_HEAL:  src = sfxpcm_heal;  len = sfxpcm_heal_len;  break;
    case SFXP_CARD:  src = sfxpcm_card;  len = sfxpcm_card_len;  break;
    }
    if (!opt_sfx) return;
    REG_SOUNDCNT_H |= 0x8000;           /* FIFO B reset */
    REG_TM1CNT_H = 0;
    REG_TM1CNT_L = PCM_TIMER;
    REG_TM1CNT_H = 0x0080;
    REG_DMA2CNT_H = 0;
    REG_DMA2SAD = (u32)src;
    REG_DMA2DAD = FIFO_B_ADDR;
    REG_DMA2CNT_H = 0xB640;
    sfxb_left = len;
}

void pcm_tick(void)
{
    if (sfxb_left) {                    /* one-shot end: stop DMA2 */
        sfxb_left = sfxb_left > PCM_PERFRAME ? sfxb_left - PCM_PERFRAME : 0;
        if (!sfxb_left) REG_DMA2CNT_H = 0;
    }
    if (!playing || !opt_music) return;
    pos += PCM_PERFRAME;
    if (pos >= cur_len) {
        pos = 0;
        REG_SOUNDCNT_H |= 0x0800;
        dma_start(cur_pcm);             /* loop (in vblank, seamless) */
    }
}
