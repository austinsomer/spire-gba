#ifndef GBA_H
#define GBA_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;

#define REG(a)          (*(vu16 *)(a))
#define REG32(a)        (*(vu32 *)(a))

/* display */
#define REG_DISPCNT     REG(0x04000000)
#define REG_DISPSTAT    REG(0x04000004)
#define REG_VCOUNT      REG(0x04000006)
#define REG_BG0CNT      REG(0x04000008)
#define REG_BG1CNT      REG(0x0400000A)
#define REG_BG2CNT      REG(0x0400000C)
#define REG_BG3CNT      REG(0x0400000E)
#define REG_BG0HOFS     REG(0x04000010)
#define REG_BG0VOFS     REG(0x04000012)
#define REG_BG1HOFS     REG(0x04000014)
#define REG_BG1VOFS     REG(0x04000016)
#define REG_BG2HOFS     REG(0x04000018)
#define REG_BG2VOFS     REG(0x0400001A)
#define REG_MOSAIC      REG(0x0400004C)
#define REG_BLDCNT      REG(0x04000050)
#define REG_BLDALPHA    REG(0x04000052)
#define REG_BLDY        REG(0x04000054)
#define BG_MOSAIC       (1 << 6)     /* mosaic enable bit in BGxCNT */

#define DCNT_MODE0      0x0000
#define DCNT_MODE4      0x0004
#define DCNT_BG0        0x0100
#define DCNT_BG1        0x0200
#define DCNT_BG2        0x0400
#define DCNT_OBJ        0x1000
#define DCNT_OBJ_1D     0x0040

#define BG_CBB(n)       ((n) << 2)
#define BG_SBB(n)       ((n) << 8)
#define BG_4BPP         0
#define BG_PRIO(n)      (n)
#define BG_SIZE_32x32   0

/* memory */
#define MEM_PAL_BG      ((vu16 *)0x05000000)
#define MEM_PAL_OBJ     ((vu16 *)0x05000200)
#define MEM_VRAM        ((vu16 *)0x06000000)
#define MEM_VRAM_OBJ    ((vu16 *)0x06010000)
#define MEM_OAM         ((vu16 *)0x07000000)
#define CHARBLOCK(n)    ((vu16 *)(0x06000000 + (n) * 0x4000))
#define SCREENBLOCK(n)  ((vu16 *)(0x06000000 + (n) * 0x800))

#define RGB15(r, g, b)  ((u16)((r) | ((g) << 5) | ((b) << 10)))

/* input */
#define REG_KEYINPUT    REG(0x04000130)
#define KEY_A           0x0001
#define KEY_B           0x0002
#define KEY_SELECT      0x0004
#define KEY_START       0x0008
#define KEY_RIGHT       0x0010
#define KEY_LEFT        0x0020
#define KEY_UP          0x0040
#define KEY_DOWN        0x0080
#define KEY_R           0x0100
#define KEY_L           0x0200
#define KEY_ANY         0x03FF

/* DMA1/2 (DirectSound FIFO A/B feeds) + timers 0/1 (sample clocks) */
#define REG_DMA1SAD     REG32(0x040000BC)
#define REG_DMA1DAD     REG32(0x040000C0)
#define REG_DMA1CNT_H   REG(0x040000C6)
#define REG_DMA2SAD     REG32(0x040000C8)
#define REG_DMA2DAD     REG32(0x040000CC)
#define REG_DMA2CNT_H   REG(0x040000D2)
#define REG_TM0CNT_L    REG(0x04000100)
#define REG_TM0CNT_H    REG(0x04000102)
#define REG_TM1CNT_L    REG(0x04000104)
#define REG_TM1CNT_H    REG(0x04000106)
#define FIFO_A_ADDR     0x040000A0
#define FIFO_B_ADDR     0x040000A4

/* cart SRAM (8-bit bus: byte access only) + waitstate control */
#define MEM_SRAM        ((vu8 *)0x0E000000)
#define REG_WAITCNT     REG(0x04000204)

/* interrupts */
#define REG_IE          REG(0x04000200)
#define REG_IF          REG(0x04000202)
#define REG_IME         REG(0x04000208)
#define IRQ_VBLANK      0x0001
#define ISR_VECTOR      (*(vu32 *)0x03007FFC)

/* sound */
#define REG_SOUNDCNT_L  REG(0x04000080)
#define REG_SOUNDCNT_H  REG(0x04000082)
#define REG_SOUNDCNT_X  REG(0x04000084)
#define REG_SOUND1CNT_L REG(0x04000060)   /* ch1 pulse: sweep */
#define REG_SOUND1CNT_H REG(0x04000062)   /* ch1 pulse: duty/len/env */
#define REG_SOUND1CNT_X REG(0x04000064)   /* ch1 pulse: freq/trigger */
#define REG_SOUND2CNT_L REG(0x04000068)   /* ch2 pulse: duty/len/env */
#define REG_SOUND2CNT_H REG(0x0400006C)   /* ch2 pulse: freq/trigger */
#define REG_SOUND3CNT_L REG(0x04000070)   /* ch3 wave: DAC/bank */
#define REG_SOUND3CNT_H REG(0x04000072)   /* ch3 wave: len/volume */
#define REG_SOUND3CNT_X REG(0x04000074)   /* ch3 wave: freq/trigger */
#define REG_SOUND4CNT_L REG(0x04000078)   /* ch4 noise: len/env */
#define REG_SOUND4CNT_H REG(0x0400007C)   /* ch4 noise: freq/trigger */
#define MEM_WAVE_RAM    ((vu16 *)0x04000090)  /* 16 bytes = 32 4-bit samples */

/* OAM attributes */
#define ATTR0_Y(y)      ((y) & 0xFF)
#define ATTR0_SQUARE    0x0000
#define ATTR0_WIDE      0x4000
#define ATTR0_TALL      0x8000
#define ATTR0_HIDE      0x0200
#define ATTR1_X(x)      ((x) & 0x1FF)
#define ATTR1_SIZE(n)   ((n) << 14)   /* w/ square: 0=8 1=16 2=32 3=64 */
#define ATTR1_HFLIP     0x1000
#define ATTR2_TILE(n)   ((n) & 0x3FF)
#define ATTR2_PAL(n)    ((n) << 12)
#define ATTR2_PRIO(n)   ((n) << 10)

typedef struct { u16 attr0, attr1, attr2, pad; } ObjAttr;
#define OAM_OBJS        ((volatile ObjAttr *)0x07000000)

#endif
