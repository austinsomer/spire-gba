/* ============================================================
   Cart SRAM persistence — settings + one mid-run save slot.

   SRAM is on an 8-bit bus at 0x0E000000: byte reads/writes only.
   The "SRAM_V113" marker string below is what emulators (mGBA)
   scan for to pick the save type, so it must stay in ROM.

   Layout:  [0..15]  settings block  'S''1' music sfx csum16
            [16..]   run block       'R''1' len16 csum16 payload
   Payload = Run struct bytes + map blob (nodes+path) + rng state.
   ============================================================ */
#include "game.h"

static const char sram_id[] = "SRAM_V113";

#define SET_OFF 0
#define RUN_OFF 16
#define RUN_PAYLOAD (sizeof(Run) + MAP_BLOB_SIZE + 4)

static u16 csum(const u8 *p, int n)
{
    u16 s = 0x1234;
    for (int i = 0; i < n; i++) s = (u16)(s * 31 + p[i]);
    return s;
}

static void sram_write(int off, const u8 *src, int n)
{
    for (int i = 0; i < n; i++) MEM_SRAM[off + i] = src[i];
}

static void sram_read(int off, u8 *dst, int n)
{
    for (int i = 0; i < n; i++) dst[i] = MEM_SRAM[off + i];
}

void save_init(void)
{
    REG_WAITCNT = (u16)((REG_WAITCNT & ~3) | 3);   /* SRAM 8-cycle wait */
    (void)*(volatile const char *)sram_id;         /* keep marker in ROM */
    u8 b[6];
    sram_read(SET_OFF, b, 6);
    if (b[0] == 'S' && b[1] == '1' &&
        csum(b, 4) == (u16)(b[4] | (b[5] << 8))) {
        opt_music = b[2] & 1;
        opt_sfx   = b[3] & 1;
    }
}

void save_settings(void)
{
    u8 b[6] = { 'S', '1', opt_music, opt_sfx, 0, 0 };
    u16 c = csum(b, 4);
    b[4] = (u8)c; b[5] = (u8)(c >> 8);
    sram_write(SET_OFF, b, 6);
}

void save_run(void)
{
    u8 buf[RUN_PAYLOAD];
    int n = 0;
    const u8 *rp = (const u8 *)&run;
    for (unsigned i = 0; i < sizeof(Run); i++) buf[n++] = rp[i];
    n += map_export(buf + n);
    u32 rs = rng_state_get();
    buf[n++] = (u8)rs; buf[n++] = (u8)(rs >> 8);
    buf[n++] = (u8)(rs >> 16); buf[n++] = (u8)(rs >> 24);
    u16 c = csum(buf, n);
    u8 h[6] = { 'R', '1', (u8)n, (u8)(n >> 8), (u8)c, (u8)(c >> 8) };
    sram_write(RUN_OFF, h, 6);
    sram_write(RUN_OFF + 6, buf, n);
}

int save_run_valid(void)
{
    u8 h[6];
    sram_read(RUN_OFF, h, 6);
    if (h[0] != 'R' || h[1] != '1') return 0;
    int n = h[2] | (h[3] << 8);
    if (n != (int)RUN_PAYLOAD) return 0;
    u8 buf[RUN_PAYLOAD];
    sram_read(RUN_OFF + 6, buf, n);
    return csum(buf, n) == (u16)(h[4] | (h[5] << 8));
}

int save_run_load(void)
{
    if (!save_run_valid()) return 0;
    u8 buf[RUN_PAYLOAD];
    sram_read(RUN_OFF + 6, buf, RUN_PAYLOAD);
    int n = 0;
    u8 *rp = (u8 *)&run;
    for (unsigned i = 0; i < sizeof(Run); i++) rp[i] = buf[n++];
    map_import(buf + n);
    n += MAP_BLOB_SIZE;
    rng_seed((u32)buf[n] | ((u32)buf[n + 1] << 8) |
             ((u32)buf[n + 2] << 16) | ((u32)buf[n + 3] << 24));
    return 1;
}

void save_run_clear(void)
{
    MEM_SRAM[RUN_OFF] = 0;
}
