/* z80.h 
 * Define or comment out macros in this file to configure the emulator. 
 *
 * derived from z80emu, lots cut out 
 * Copyright (c) 2016, 2017 Lin Ke-Fong
 *
 * This code is free, do whatever you want with it.
 */

#define Z80_S_FLAG_SHIFT        7
#define Z80_Z_FLAG_SHIFT        6
#define Z80_Y_FLAG_SHIFT        5
#define Z80_H_FLAG_SHIFT        4
#define Z80_X_FLAG_SHIFT        3
#define Z80_PV_FLAG_SHIFT       2
#define Z80_N_FLAG_SHIFT        1
#define Z80_C_FLAG_SHIFT        0

#define Z80_S_FLAG              (1 << Z80_S_FLAG_SHIFT)
#define Z80_Z_FLAG              (1 << Z80_Z_FLAG_SHIFT)
#define Z80_Y_FLAG              (1 << Z80_Y_FLAG_SHIFT)
#define Z80_H_FLAG              (1 << Z80_H_FLAG_SHIFT)
#define Z80_X_FLAG              (1 << Z80_X_FLAG_SHIFT)
#define Z80_PV_FLAG             (1 << Z80_PV_FLAG_SHIFT)
#define Z80_N_FLAG              (1 << Z80_N_FLAG_SHIFT)
#define Z80_C_FLAG              (1 << Z80_C_FLAG_SHIFT)

#define Z80_P_FLAG_SHIFT        Z80_PV_FLAG_SHIFT
#define Z80_V_FLAG_SHIFT        Z80_PV_FLAG_SHIFT
#define Z80_P_FLAG              Z80_PV_FLAG
#define Z80_V_FLAG              Z80_PV_FLAG

typedef unsigned char byte;
typedef unsigned short word;

typedef enum { set, clear } bit;
typedef enum { im0, im1, im2 } int_mode;

enum register_pair_index { Z80_BC, Z80_DE, Z80_HL, Z80_AF, Z80_IX, Z80_IY, Z80_SP };

/*
 * we use a union to do the register pairs.  the indexes in the union are
 * byte sex dependent.  the default is little-endian.
 */
#ifdef BIG_ENDIAN
enum register_index { 
    Z80_B, Z80_C, Z80_D, Z80_E, Z80_H, Z80_L, Z80_A, Z80_F, 
    Z80_IXH, Z80_IXL, Z80_IYH, Z80_IYL 
};
#else
enum register_index { 
    Z80_C, Z80_B, Z80_E, Z80_D, Z80_L, Z80_H, Z80_F, Z80_A, 
    Z80_IXL, Z80_IXH, Z80_IYL, Z80_IYH 
};
#endif

/*
 * Z80 processor's state.
 */
typedef struct {
    union {
        byte reg8[14];
        word reg16[7];
    } regs;

    word alternates[4];

    byte i;         // interrupt vector register for im2
	byte r;         // 7 bit refresh count
	word pc;        // the location where the next M1 cycle will be run.
	bit iff1;       // if set, interrupt enabled
	int iff2;
	int_mode im;
    bit m1;         // if this memory cycle an m1
} MACHINE;

extern void Z80Reset();

extern int Z80Interrupt(int data_on_bus);       // queue an interrupt
extern int Z80NonMaskableInterrupt();           // queue an nmi
extern int Z80Emulate();                        // run an instruction

// all the memory and port hooks
extern byte get_byte(word a);
extern word get_word(word a);
extern byte get_opcode(word a);
extern byte input_byte(byte p);
extern void put_byte(word a, byte v);
extern void put_word(word a, word v);
extern void output_byte(byte p, byte v);

extern volatile MACHINE cpu;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

