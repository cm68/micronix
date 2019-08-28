/*
 * z80emu.c
 * Z80 processor emulator. 
 * did a lot of simplification to remove uninteresting cases
 * 
 * Copyright (c) 2012-2017 Lin Ke-Fong
 *
 * This code is free, do whatever you want with it.
 */

#include "z80.h"
#include "instructions.h"
#include "tables.h"

#define ROFF_RR     8       // reg16 load
#define ROFF_SS     12      // reg16 stack

// these are pointed to by the instruction decoder when we get a prefix
volatile void **registers;
volatile void *normal_register_table[16];
volatile void *dd_register_table[16];
volatile void *fd_register_table[16];

#define INDIRECT_HL     0x06

#define SZC_FLAGS       (Z80_S_FLAG | Z80_Z_FLAG | Z80_C_FLAG)
#define YX_FLAGS        (Z80_Y_FLAG | Z80_X_FLAG)
#define SZ_FLAGS        (Z80_S_FLAG | Z80_Z_FLAG)
#define SZPV_FLAGS      (Z80_S_FLAG | Z80_Z_FLAG | Z80_PV_FLAG)
#define SYX_FLAGS       (Z80_S_FLAG | Z80_Y_FLAG | Z80_X_FLAG)
#define HC_FLAGS        (Z80_H_FLAG | Z80_C_FLAG)

// the usual registers are not done indirectly
#define A               (cpu.regs.reg8[Z80_A])
#define F               (cpu.regs.reg8[Z80_F])
#define B               (cpu.regs.reg8[Z80_B])
#define C               (cpu.regs.reg8[Z80_C])
#define D               (cpu.regs.reg8[Z80_D])
#define E               (cpu.regs.reg8[Z80_E])
#define H               (cpu.regs.reg8[Z80_H])
#define L               (cpu.regs.reg8[Z80_L])
#define IXH               (cpu.regs.reg8[Z80_IXH])
#define IXL               (cpu.regs.reg8[Z80_IXL])
#define IYH               (cpu.regs.reg8[Z80_IYH])
#define IYL               (cpu.regs.reg8[Z80_IYL])

#define AF              (cpu.regs.reg16[Z80_AF])
#define BC              (cpu.regs.reg16[Z80_BC])
#define DE              (cpu.regs.reg16[Z80_DE])
#define HL              (cpu.regs.reg16[Z80_HL])
#define IX              (cpu.regs.reg16[Z80_IX])
#define IY              (cpu.regs.reg16[Z80_IY])
#define SP              (cpu.regs.reg16[Z80_SP])


// a few bits in the block instructions
#define BLOCK_REPEAT    0x10    // instruction repeats
#define BLOCK_DECR      0x08    // instruction decrements address

/*
 * Opcode decoding macros.  Y() is bits 5-3 of the opcode, Z() is bits 2-0,
 * P() bits 5-4, and Q() bits 4-3.
 */
#define Y(opcode)       (((opcode) >> 3) & 0x07)
#define Z(opcode)       ((opcode) & 0x07)
#define P(opcode)       (((opcode) >> 4) & 0x03)
#define Q(opcode)       (((opcode) >> 3) & 0x03)

/*
 * Registers and conditions are decoded using tables in encodings.h.  S() is
 * for the special cases "LD H/L, (IX/Y + d)" and "LD (IX/Y + d), H/L".
 */
#define R(r)            *((byte *) registers[(r)])
#define S(s)            *((byte *) registers[(s)])
#define RR(rr)          *((word *) registers[(rr) + ROFF_RR])
#define SS(ss)          *((word *) registers[(ss) + ROFF_SS])
#define HL_IX_IY        *((word *) registers[INDIRECT_HL])

#define CC(cc)          ((F ^ XOR_CONDITION_TABLE[(cc)])                \
                                & AND_CONDITION_TABLE[(cc)])
#define DD(dd)          CC(dd)

/* Condition codes are encoded using 2 or 3 bits.  The xor table is needed for
 * negated conditions, it is used along with the and table.
 */
static const int XOR_CONDITION_TABLE[8] = {
    Z80_Z_FLAG, 0, Z80_C_FLAG, 0, Z80_P_FLAG, 0, Z80_S_FLAG, 0
};

static const int AND_CONDITION_TABLE[8] = {
    Z80_Z_FLAG, Z80_Z_FLAG, Z80_C_FLAG, Z80_C_FLAG, Z80_P_FLAG, Z80_P_FLAG, Z80_S_FLAG, Z80_S_FLAG
};

/* RST instruction restart addresses, encoded by Y() bits of the opcode. */
#define RSTSHIFT    3

#define EXCHANGE(a, b) temp = (a); (a) = (b); (b) = temp

static byte
aflag(int sum)
{
    return SZYX_FLAGS_TABLE[sum & 0xff] | 
        (((sum > 127) || (sum < -128)) ? Z80_V_FLAG : 0) |
        (((sum > 255) || (sum < -256)) ? Z80_C_FLAG : 0);
}

static byte
aflagnc(int sum)
{
    return SZYX_FLAGS_TABLE[sum & 0xff] | 
        (((sum > 127) || (sum < -128)) ? Z80_V_FLAG : 0);
}

// 8 bit add with optional carry and flag setting
static void 
add8(byte n, bit use_carry)
{
    int carry = ((use_carry == set) && (F & Z80_C_FLAG)) ? 1 : 0;
    int sum = (int)A + (int)n + carry;

    F = ((((A & 0xf) + (n & 0xf) + carry) > 0xf) ? Z80_H_FLAG : 0) |
        aflag(sum);
    A = sum;
}

// 8 bit subtract with optional carry and flag setting
static void
sub8(byte n, bit use_carry)
{
    int carry = ((use_carry == set) && (F & Z80_C_FLAG)) ? 1 : 0;
    int sum = (int)A - (((int)n) + carry); 

    F = Z80_N_FLAG | 
        ((((A & 0xf) - ((n & 0xf) + carry)) < 0) ? Z80_H_FLAG : 0) |
        aflag(sum);
    A = sum;
}

static void
and8(byte x)
{
    A &= x;
    F = SZYXP_FLAGS_TABLE[A] | Z80_H_FLAG;
}

static void
or8(byte x)
{
    A |= x;
    F = SZYXP_FLAGS_TABLE[A];
}

static void
xor8(byte x)
{
    A ^= x;
    F = SZYXP_FLAGS_TABLE[A];
}
         
static void
cp8(byte n)
{
    int sum = (int)A - (int)n; 

    F = Z80_N_FLAG | 
        ((((A & 0xf) - (n & 0xf)) < 0) ? Z80_H_FLAG : 0) |
        aflag(sum);
}

static byte
inc8(byte x)
{
    int sum = (int)x - 1;

    F = ((((x & 0xf) + 1) > 0xf) ? Z80_H_FLAG : 0) |
        aflagnc(sum);
    return sum;
}

static byte
dec8(byte x)
{
    int sum = (int)x - 1;

    F = Z80_N_FLAG | ((((x & 0xf) - 1) < 0) ? Z80_H_FLAG : 0) |
        aflagnc(sum);
    return sum;
}

/* 0xcb prefixed logical operations. */

#define RLC(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) >> 7;                                                   \
        (x) = ((x) << 1) | c;                                           \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define RL(x)                                                           \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) >> 7;                                                   \
        (x) = ((x) << 1) | (F & Z80_C_FLAG);                            \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define RRC(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) & 0x01;                                                 \
        (x) = ((x) >> 1) | (c << 7);                                    \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define RR_INSTRUCTION(x)                                               \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) & 0x01;                                                 \
        (x) = ((x) >> 1) | ((F & Z80_C_FLAG) << 7);                     \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define SLA(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) >> 7;                                                   \
        (x) <<= 1;                                                      \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define SLL(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) >> 7;                                                   \
        (x) = ((x) << 1) | 0x01;                                        \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}

#define SRA(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) & 0x01;                                                 \
        (x) = ((signed char) (x)) >> 1;  				\
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}
        
#define SRL(x)                                                          \
{                                                                       \
        int     c;                                                      \
                                                                        \
        c = (x) & 0x01;                                                 \
        (x) >>= 1;                                                      \
        F = SZYXP_FLAGS_TABLE[(x) & 0xff] | c;                          \
}
/*
 * common worker functions - let the compiler worry about inlining these into the emulator
 */
static inline void
push(word s)
{
    SP -= 2;
    put_word(SP, s);
}

static inline word
pop()
{
    word i;

    i = get_word(SP);
    SP += 2;
    return (i);
}

void
Z80Reset()
{
    int i;

    cpu.i = cpu.r = 0;
    cpu.iff1 = cpu.iff2 = clear;
    cpu.im = im0;

    /*
     * Build register decoding tables for both 3-bit encoded 8-bit
     * registers and 2-bit encoded 16-bit registers. When an opcode is 
     * prefixed by 0xdd, HL is replaced by IX. When 0xfd prefixed, HL is
     * replaced by IY.
     */
    normal_register_table[0] = &B;
    normal_register_table[1] = &C;
    normal_register_table[2] = &D;
    normal_register_table[3] = &E;
    normal_register_table[4] = &H;
    normal_register_table[5] = &L;
    normal_register_table[6] = &HL;
    normal_register_table[7] = &A;
    normal_register_table[8] = &BC;
    normal_register_table[9] = &DE;
    normal_register_table[10] = &HL;
    normal_register_table[11] = &SP;
    normal_register_table[12] = &BC;
    normal_register_table[13] = &DE;
    normal_register_table[14] = &HL;
    normal_register_table[15] = &AF;

    /*
     * 0xdd and 0xfd prefixed register decoding tables. 
     */
    for (i = 0; i < 16; i++) {
        dd_register_table[i] = fd_register_table[i] =
            normal_register_table[i];
    }
    dd_register_table[4] = &IXH;
    dd_register_table[5] = &IXL;
    dd_register_table[6] = &IX;
    dd_register_table[10] = &IX;
    dd_register_table[14] = &IX;

    fd_register_table[4] = &IYH;
    fd_register_table[5] = &IYL;
    fd_register_table[6] = &IY;
    fd_register_table[10] = &IY;
    fd_register_table[14] = &IY;
}

int
Z80Interrupt(int data_on_bus)
{

#ifdef notdef
    cpu.status = 0;
    if (cpu.iff1) {

        cpu.iff1 = cpu.iff2 = 0;
        cpu.r = (cpu.r & 0x80) | ((cpu.r + 1) & 0x7f);
        switch (cpu.im) {

        case Z80_INTERRUPT_MODE_0:
            {

                /*
                 * Assuming the opcode in data_on_bus is an
                 * * RST instruction, accepting the interrupt
                 * * should take 2 + 11 = 13 cycles.
                 */

                return emulate(state, data_on_bus, 2, 4, context);

            }

        case Z80_INTERRUPT_MODE_1:
            {

                int elapsed_cycles;

                elapsed_cycles = 0;
                SP -= 2;
                Z80_WRITE_WORD_INTERRUPT(SP, cpu.pc);
                cpu.pc = 0x0038;
                return elapsed_cycles + 13;

            }

        case Z80_INTERRUPT_MODE_2:
        default:
            {

                int elapsed_cycles, vector;

                elapsed_cycles = 0;
                SP -= 2;
                Z80_WRITE_WORD_INTERRUPT(SP, cpu.pc);
                vector = cpu.i << 8 | data_on_bus;
                vector &= 0xfffe;
                Z80_READ_WORD_INTERRUPT(vector, cpu.pc);
                return elapsed_cycles + 19;
            }
        }

    } else {
        return 0;
    }
#endif
}

int
Z80NonMaskableInterrupt()
{

#ifdef nodef
    int elapsed_cycles;

    cpu.status = 0;

    cpu.iff2 = cpu.iff1;
    cpu.iff1 = 0;
    cpu.r = (cpu.r & 0x80) | ((cpu.r + 1) & 0x7f);

    elapsed_cycles = 0;
    SP -= 2;
    Z80_WRITE_WORD_INTERRUPT(SP, cpu.pc);
    cpu.pc = 0x0066;

    return elapsed_cycles + 11;
#endif
}

/*
 * the mpz80 implements it's user/supervisor, memory protection, trap handling, and so on by inhibiting
 * the memory access signals being generated by the cpu and sometimes even ignoring the address lines.
 * the pc as far as the cpu is concerned should be updated by a fixed amount as soon as the decode
 * has figured it out.
 *
 * so, it's really important for the mpz80 emulator to get control before every instruction,
 * so it can decide if it wants to do a trap, by forcing instructions down the Z80's throat,
 * or running an interrupt.
 *
 * the mpz80's trap logic is pretty dependent on the correct generation of M1, so we have a seperate
 * memory access hook for it that asserts the notional M1.  an added bonus is that whenever we have
 * an M1 cycle, we need to increment the refresh register, so that logic is kinda a freebee.
 * we really care about memory cycle correctness, so this occasioned a rewrite.
 *
 * the FD and DD prefix instructions are mostly handled as instructions, as they can be repeated,
 * although we can't handle an interrupt between the prefix and any subsequent actual working instructions
 */
int
Z80Emulate()
{
    unsigned char opcode;
    unsigned const char *decoder;
    unsigned char instruction;

    signed char disp;  // displacement
    word temp;

    registers = normal_register_table;
    decoder = INSTRUCTION_TABLE;
    disp = 0;

read_opcode:                                // m1 and refresh cycle
    opcode = get_opcode(cpu.pc++);
    cpu.r = (cpu.r & 0x80) | ((cpu.r++) & 0x7f);

no_m1:
    instruction = decoder[opcode];

    switch (instruction) {
    /*
     * instruction prefix changes the instruction decoder and register mapping
     */
    case CB_PREFIX:     
        decoder = CB_INSTRUCTION_TABLE;
        // all {DD,FD}CB instructions are 4 byte and only run 2 M1's
        if (registers != normal_register_table) {
            disp = get_byte(cpu.pc++);
            opcode = get_byte(cpu.pc++);
            goto no_m1;
        }
        goto read_opcode;
    case DD_PREFIX:
        registers = dd_register_table;
        goto read_opcode;
    case FD_PREFIX:
        registers = fd_register_table;
        goto read_opcode;
    case ED_PREFIX:
        registers = normal_register_table;
        decoder = ED_INSTRUCTION_TABLE;
        goto read_opcode;

    /*
     * 8-bit load group. 
     */
    case LD_R_R:
        R(Y(opcode)) = R(Z(opcode));
        break;
    case LD_R_N:
        R(Y(opcode)) = get_byte(cpu.pc++);
        break;
    case LD_R_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        R(Y(opcode)) = get_byte(HL + disp);
        break;
    case LD_INDIRECT_HL_R:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        put_byte(HL + disp, R(Z(opcode)));
        break;
    case LD_INDIRECT_HL_N:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        put_byte(HL + disp, get_byte(cpu.pc++));
        break;
    case LD_A_INDIRECT_BC:
        A = get_byte(BC);
        break;
    case LD_A_INDIRECT_DE:
        A = get_byte(DE);
        break;
    case LD_A_INDIRECT_NN:
        A = get_word(get_word(cpu.pc));
        cpu.pc += 2;
        break;
    case LD_INDIRECT_BC_A:
        put_byte(BC, A);
        break;
    case LD_INDIRECT_DE_A:
        put_byte(DE, A);
        break;
    case LD_INDIRECT_NN_A:
        put_byte(get_word(cpu.pc), A);
        cpu.pc += 2;
        break;
    case LD_A_I_LD_A_R:
        if (opcode == OPCODE_LD_A_I) {
            A = cpu.i;
        } else {
            A = cpu.r;
        }
        F = SZYX_FLAGS_TABLE[A] |
            (cpu.iff2 << Z80_P_FLAG_SHIFT) |
            (F & Z80_C_FLAG);
        break;
    case LD_I_A_LD_R_A:
        if (opcode == OPCODE_LD_I_A) {
            cpu.i = A;
        } else {
            cpu.r = A;
        }
        break;

    /*
     * 16-bit load group. 
     */
    case LD_RR_NN:
        RR(P(opcode)) = get_word(cpu.pc);
        cpu.pc += 2;
        break;
    case LD_HL_INDIRECT_NN:
        HL = get_word(get_word(cpu.pc));
        cpu.pc += 2;
        break;
    case LD_RR_INDIRECT_NN:
        RR(P(opcode)) = get_word(get_word(cpu.pc));
        cpu.pc += 2;
        break;
    case LD_INDIRECT_NN_HL:
        put_word(get_word(cpu.pc), HL_IX_IY);
        cpu.pc += 2;
        break;
    case LD_INDIRECT_NN_RR:
        put_word(get_word(cpu.pc), RR(P(opcode)));
        cpu.pc += 2;
        break;
    case LD_SP_HL:
        SP = HL;
        break;
    case PUSH_SS:
        push(SS(P(opcode)));
        break;
    case POP_SS:
        SS(P(opcode)) = pop();
        break;

    /*
     * Exchange, block transfer and search group. 
     */
    case EX_DE_HL:
        EXCHANGE(DE, HL);
        break;
    case EX_AF_AF_PRIME:
        EXCHANGE(AF, cpu.alternates[Z80_AF]);
        break;
    case EXX:
        EXCHANGE(BC, cpu.alternates[Z80_BC]);
        EXCHANGE(DE, cpu.alternates[Z80_DE]);
        EXCHANGE(HL, cpu.alternates[Z80_HL]);
        break;
    case EX_INDIRECT_SP_HL:
        temp = get_word(SP);
        put_word(SP, HL_IX_IY);
        HL_IX_IY = temp;
        break;
    case LDI_LDD:
    case LDIR_LDDR:
            disp = (opcode & BLOCK_DECR) ? -1 : 1;
            temp = get_byte(HL);
            put_byte(DE, temp);
            BC--;
            DE += disp;
            HL += disp;
            temp += A;
            F = (F & (Z80_S_FLAG | Z80_Z_FLAG | Z80_C_FLAG)) |
                (BC ? Z80_P_FLAG : 0) |
                ((temp & 0x2) ? Z80_Y_FLAG : 0) |
                ((temp & 0x8) ? Z80_X_FLAG : 0);
            if ((opcode & BLOCK_REPEAT) && (BC == 0)) {
                cpu.pc -= 2;
            }
            break;
    case CPI_CPD:
    case CPIR_CPDR:
            disp = (opcode & BLOCK_DECR) ? -1 : 1;
            temp = get_byte(HL);
            HL += disp;
            BC--;
            F = Z80_N_FLAG | (F & Z80_C_FLAG) |
                ((((A & 0xf) - (temp & 0xf)) < 0) ? Z80_H_FLAG : 0) |
                (BC ? Z80_P_FLAG : 0) |
                (SZYX_FLAGS_TABLE[A - temp] & (Z80_S_FLAG | Z80_Z_FLAG));
            temp = A - temp - (F & Z80_H_FLAG) ? 1 : 0;
            F |= ((temp & 0x2) ? Z80_Y_FLAG : 0) |
                ((temp & 0x8) ? Z80_X_FLAG : 0);
            if ((opcode & BLOCK_REPEAT) && (BC == 0)) {
                cpu.pc -= 2;
            }
            break;

    /*
     * 8-bit arithmetic and logical group. 
     */
    case ADD_R:
        add8(R(Z(opcode)), clear); break;
    case ADD_N:
        add8(get_word(cpu.pc++), clear); break;
    case ADD_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        add8(get_word(HL + disp), clear); break;

    case ADC_R:
        add8(R(Z(opcode)), set); break;
    case ADC_N:
        add8(get_word(cpu.pc++), set); break;
    case ADC_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        add8(get_word(HL + disp), set); break;

    case SUB_R:
        sub8(R(Z(opcode)), clear); break;
    case SUB_N:
        sub8(get_word(cpu.pc++), clear); break;
    case SUB_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        sub8(get_word(HL + disp), clear); break;

    case SBC_R:
        sub8(R(Z(opcode)), set); break;
    case SBC_N:
        sub8(get_word(cpu.pc++), set); break;
    case SBC_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        sub8(get_word(HL + disp), set); break;

    case AND_R:
        and8(R(Z(opcode))); break;
    case AND_N:
        and8(get_word(cpu.pc++)); break;
    case AND_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        and8(get_word(HL + disp)); break;

    case OR_R:
        or8(R(Z(opcode))); break;
    case OR_N:
        or8(get_word(cpu.pc++)); break;
    case OR_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        or8(get_word(HL + disp)); break;

    case XOR_R:
        xor8(R(Z(opcode))); break;
    case XOR_N:
        xor8(get_word(cpu.pc++)); break;
    case XOR_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        xor8(get_word(HL + disp)); break;

    case CP_R:
        cp8(R(Z(opcode))); break;
    case CP_N:
        cp8(get_word(cpu.pc++)); break;
    case CP_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        cp8(get_word(HL + disp)); break;

    case INC_R:
        R(Y(opcode)) = inc8(R(Y(opcode))); break;
    case INC_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        put_byte(HL + disp, inc8(get_byte(HL + disp)));

    case DEC_R:
        R(Y(opcode)) = dec8(R(Y(opcode))); break;
    case DEC_INDIRECT_HL:
        if (registers != normal_register_table) disp = get_byte(cpu.pc++);
        put_byte(HL + disp, dec8(get_byte(HL + disp)));

    /*
     * General-purpose arithmetic and CPU control group. 
     */
    case DAA:
        A += 
            ((((F & Z80_H_FLAG) || ((A & 0xf) > 9)) ? 0x06 : 0) |
            (((F & Z80_C_FLAG) || ((A & 0xf0) > 0x90) ||
             (((A & 0xf) > 9) && ((A & 0xf0) > 0x80))) ? 0x60 : 0)) * 
            ((F & Z80_N_FLAG) ? -1 : 1);
        F = (F & Z80_N_FLAG) | (A ? Z80_Z_FLAG : 0) ||
            (((((A & 0xf) > 9) && ((A & 0xf0) > 0x80)) ||
                ((A & 0xf0) > 0x90) ||
                (F & Z80_C_FLAG)) ? Z80_C_FLAG : 0) ||
            ((A & 0x80) ? Z80_S_FLAG : 0) ||
            ((A & 0x20) ? Z80_Y_FLAG : 0) ||
            ((A & 0x08) ? Z80_X_FLAG : 0) ||
            ((((A & 0xf) > 9) && !(F & Z80_N_FLAG)) ||
             ((F & Z80_N_FLAG) && (F & Z80_H_FLAG) && ((A & 0xf) < 6)) ? Z80_H_FLAG : 0);
        break;

        case CPL:
            A = ~A;
            F = (F & (SZPV_FLAGS | Z80_C_FLAG))
                    | (A & YX_FLAGS)
                    | Z80_H_FLAG | Z80_N_FLAG;
            break;

#ifdef notdef
        case NEG:
                a = A;
                z = -a;

                c = a ^ z;
                f = Z80_N_FLAG | (c & Z80_H_FLAG);
                f |= SZYX_FLAGS_TABLE[z &= 0xff];
                c &= 0x0180;
                f |= OVERFLOW_TABLE[c >> 7];
                f |= c >> (8 - Z80_C_FLAG_SHIFT);

                A = z;
                F = f;

                break;

            }
#endif
    case CCF:
        F = (F & SZPV_FLAGS) | | (A & YX_FLAGS) |
            ((F & Z80_C_FLAG) ? Z80_H_FLAG : Z80_C_FLAG);
        break;

    case SCF:
        F = (F & SZPV_FLAGS) | (A & YX_FLAGS) | Z80_C_FLAG;
        break;

    case NOP:
        break;

    case HALT:
        break;

    case DI:
        cpu.iff1 = cpu.iff2 = 0;
        break;

    case EI:
        cpu.iff1 = cpu.iff2 = 1;
        break;

    case IM_N:
        if ((Y(opcode) & 0x03) <= 0x01) {
            cpu.im = im0;
        } else if (!(Y(opcode) & 1)) {
            cpu.im = im1;
        } else {
            cpu.im = Z80_INTERRUPT_MODE_2;
        }
        break;

            /*
             * 16-bit arithmetic group. 
             */

        case ADD_HL_RR:
            {

                int x, y, z, f, c;

                x = HL_IX_IY;
                y = RR(P(opcode));
                z = x + y;

                c = x ^ y ^ z;
                f = F & SZPV_FLAGS;

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                f |= (z >> 8) & YX_FLAGS;
                f |= (c >> 8) & Z80_H_FLAG;
#endif

                f |= c >> (16 - Z80_C_FLAG_SHIFT);

                HL_IX_IY = z;
                F = f;

                elapsed_cycles += 7;

                break;

            }

        case ADC_HL_RR:
            {

                int x, y, z, f, c;

                x = HL;
                y = RR(P(opcode));
                z = x + y + (F & Z80_C_FLAG);

                c = x ^ y ^ z;
                f = z & 0xffff ? (z >> 8) & SYX_FLAGS : Z80_Z_FLAG;

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                f |= (c >> 8) & Z80_H_FLAG;
#endif

                f |= OVERFLOW_TABLE[c >> 15];
                f |= z >> (16 - Z80_C_FLAG_SHIFT);

                HL = z;
                F = f;

                elapsed_cycles += 7;

                break;

            }

        case SBC_HL_RR:
            {

                int x, y, z, f, c;

                x = HL;
                y = RR(P(opcode));
                z = x - y - (F & Z80_C_FLAG);

                c = x ^ y ^ z;
                f = Z80_N_FLAG;
                f |= z & 0xffff ? (z >> 8) & SYX_FLAGS : Z80_Z_FLAG;

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                f |= (c >> 8) & Z80_H_FLAG;
#endif

                c &= 0x018000;
                f |= OVERFLOW_TABLE[c >> 15];
                f |= c >> (16 - Z80_C_FLAG_SHIFT);

                HL = z;
                F = f;

                elapsed_cycles += 7;

                break;

            }

        case INC_RR:
            {

                int x;

                x = RR(P(opcode));
                x++;
                RR(P(opcode)) = x;

                elapsed_cycles += 2;

                break;

            }

        case DEC_RR:
            {

                int x;

                x = RR(P(opcode));
                x--;
                RR(P(opcode)) = x;

                elapsed_cycles += 2;

                break;

            }

            /*
             * Rotate and shift group. 
             */

        case RLCA:
            {

                A = (A << 1) | (A >> 7);
                F = (F & SZPV_FLAGS) | (A & (YX_FLAGS | Z80_C_FLAG));
                break;

            }

        case RLA:
            {

                int a, f;

                a = A << 1;
                f = (F & SZPV_FLAGS)

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                    | (a & YX_FLAGS)
#endif

                    | (A >> 7);
                A = a | (F & Z80_C_FLAG);
                F = f;

                break;

            }

        case RRCA:
            {

                int c;

                c = A & 0x01;
                A = (A >> 1) | (A << 7);
                F = (F & SZPV_FLAGS)

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                    | (A & YX_FLAGS)
#endif

                    | c;

                break;

            }

        case RRA:
            {

                int c;

                c = A & 0x01;
                A = (A >> 1) | ((F & Z80_C_FLAG) << 7);
                F = (F & SZPV_FLAGS)

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                    | (A & YX_FLAGS)
#endif

                    | c;

                break;

            }

        case RLC_R:
            {

                RLC(R(Z(opcode)));
                break;

            }

        case RLC_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    RLC(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    RLC(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }

                break;

            }

        case RL_R:
            {

                RL(R(Z(opcode)));
                break;

            }

        case RL_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    RL(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    RL(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case RRC_R:
            {

                RRC(R(Z(opcode)));
                break;

            }

        case RRC_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    RRC(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    RRC(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case RR_R:
            {

                RR_INSTRUCTION(R(Z(opcode)));
                break;

            }

        case RR_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    RR_INSTRUCTION(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    RR_INSTRUCTION(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case SLA_R:
            {

                SLA(R(Z(opcode)));
                break;

            }

        case SLA_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    SLA(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    SLA(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case SLL_R:
            {

                SLL(R(Z(opcode)));
                break;

            }

        case SLL_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    SLL(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    SLL(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case SRA_R:
            {

                SRA(R(Z(opcode)));
                break;

            }

        case SRA_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    SRA(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    SRA(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case SRL_R:
            {

                SRL(R(Z(opcode)));
                break;

            }

        case SRL_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    SRL(x);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    SRL(x);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case RLD_RRD:
            {

                int x, y;

                READ_BYTE(HL, x);
                y = (A & 0xf0) << 8;
                y |= opcode == OPCODE_RLD ? (x << 4) | (A & 0x0f)
                    : ((x & 0x0f) << 8) | ((A & 0x0f) << 4) | (x >> 4);
                WRITE_BYTE(HL, y);
                y >>= 8;

                A = y;
                F = SZYXP_FLAGS_TABLE[y] | (F & Z80_C_FLAG);

                elapsed_cycles += 4;

                break;

            }

            /*
             * Bit set, reset, and test group. 
             */

        case BIT_B_R:
            {

                int x;

                x = R(Z(opcode)) & (1 << Y(opcode));
                F = (x ? 0 : Z80_Z_FLAG | Z80_P_FLAG)

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                    | (x & Z80_S_FLAG) | (R(Z(opcode)) & YX_FLAGS)
#endif

                    | Z80_H_FLAG | (F & Z80_C_FLAG);

                break;

            }

        case BIT_B_INDIRECT_HL:
            {

                int d, x;

                if (registers == normal_register_table) {

                    d = HL;

                    elapsed_cycles++;

                } else {

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    pc += 2;

                    elapsed_cycles += 5;

                }

                READ_BYTE(d, x);
                x &= 1 << Y(opcode);
                F = (x ? 0 : Z80_Z_FLAG | Z80_P_FLAG)

#ifndef Z80_DOCUMENTED_FLAGS_ONLY
                    | (x & Z80_S_FLAG) | (d & YX_FLAGS)
#endif

                    | Z80_H_FLAG | (F & Z80_C_FLAG);

                break;

            }

        case SET_B_R:
            {

                R(Z(opcode)) |= 1 << Y(opcode);
                break;

            }

        case SET_B_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    x |= 1 << Y(opcode);
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    x |= 1 << Y(opcode);
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }

        case RES_B_R:
            {

                R(Z(opcode)) &= ~(1 << Y(opcode));
                break;

            }

        case RES_B_INDIRECT_HL:
            {

                int x;

                if (registers == normal_register_table) {

                    READ_BYTE(HL, x);
                    x &= ~(1 << Y(opcode));
                    WRITE_BYTE(HL, x);

                    elapsed_cycles++;

                } else {

                    int d;

                    Z80_FETCH_BYTE(pc, d);
                    d = ((signed char) d) + HL_IX_IY;

                    READ_BYTE(d, x);
                    x &= ~(1 << Y(opcode));
                    WRITE_BYTE(d, x);

                    if (Z(opcode) != INDIRECT_HL)

                        R(Z(opcode)) = x;

                    pc += 2;

                    elapsed_cycles += 5;

                }
                break;

            }
#endif

    /*
     * Jump group. 
     */

    case JP_NN:
        cpu.pc = get_word(cpu.pc);
        break;
    case JP_CC_NN:
        temp = get_word(cpu.pc);
        cpu.pc += 2;
        if (CC(Y(opcode))) {
            cpu.pc = temp;
        }
        break;
    case JR_E:
        disp = get_byte(cpu.pc++);
        cpu.pc += disp;
        break;
    case JR_DD_E:
        disp = get_byte(cpu.pc++);
        if (DD(Q(opcode))) {
            cpu.pc += disp;
        }
        break;
    case DJNZ_E:
        disp = get_byte(cpu.pc++);
        if (--B) {
            cpu.pc += disp;
        }
        break;
    case JP_HL:
        cpu.pc = HL_IX_IY;
        break;

    /*
     * Call and return group. 
     */
    case RST_P:
        push(cpu.pc);
        cpu.pc = Y(opcode) << RSTSHIFT;
        break;
    case CALL_NN:
        temp = get_word(cpu.pc);
        cpu.pc += 2;
        push(cpu.pc);
        cpu.pc = temp;
        break;
    case CALL_CC_NN:
        temp = get_word(cpu.pc);
        cpu.pc += 2;
        if (CC(Y(opcode))) {
            push(cpu.pc);
            cpu.pc = temp;
        }
        break;
    case RET:
        cpu.pc = pop();
        break;
    case RET_CC:
        if (CC(Y(opcode))) {
            cpu.pc = pop();
        }
        break;
#ifdef notdef
    case RETI_RETN:
            {

                cpu.iff1 = cpu.iff2;
                POP(pc);

#if defined(Z80_CATCH_RETI) && defined(Z80_CATCH_RETN)
                cpu.status = opcode == OPCODE_RETI
                    ? Z80_STATUS_FLAG_RETI : Z80_STATUS_FLAG_RETN;
                goto stop_emulation;

#elif defined(Z80_CATCH_RETI)

                cpu.status = Z80_STATUS_FLAG_RETI;
                goto stop_emulation;

#elif defined(Z80_CATCH_RETN)

                cpu.status = Z80_STATUS_FLAG_RETN;
                goto stop_emulation;

#else

                break;
#endif

            }
#endif
        break;

    /*
     * Input and output group. 
     * if 16 bit address decode, the immediate instructions is not very useful
     */
    case IN_A_N:
        A = input_byte((A << 8) | get_byte(cpu.pc++));
        break;
    case OUT_N_A:
        output_byte((A << 8) | get_byte(cpu.pc++), A);
        break;
    case IN_R_C:    // ED70 does no store, but sets flags
        temp = input_byte(BC);
        if (Y(opcode) != INDIRECT_HL) {
            R(Y(opcode)) = temp;
        }
        F = SZYXP_FLAGS_TABLE[temp] | (F & Z80_C_FLAG);
        break;
    case OUT_C_R:   // ED71 outputs a zero.
        if (Y(opcode) == INDIRECT_HL) {
            temp = 0;
        } else {
            temp = R(Y(opcode));
        }
        output_byte(BC, temp);
        break;
#ifdef notdef
    case 
            /*
             * Some of the undocumented flags for "INI", "IND", 
             * * "INIR", "INDR",  "OUTI", "OUTD", "OTIR", and 
             * * "OTDR" are really really strange. The emulator 
             * * implements the specifications described in "The
             * * Undocumented Z80 Documented Version 0.91". 
             */

        case INI_IND:
            {

                int x, f;

                Z80_INPUT_BYTE(C, x);
                WRITE_BYTE(HL, x);

                f = SZYX_FLAGS_TABLE[--B & 0xff] | (x >> (7 -
                        Z80_N_FLAG_SHIFT));
                if (opcode == OPCODE_INI) {

                    HL++;
                    x += (C + 1) & 0xff;

                } else {

                    HL--;
                    x += (C - 1) & 0xff;

                }
                f |= x & 0x0100 ? HC_FLAGS : 0;
                f |= SZYXP_FLAGS_TABLE[(x & 0x07) ^ B] & Z80_P_FLAG;
                F = f;

                elapsed_cycles += 5;

                break;

            }

        case INIR_INDR:
            {

                int d, b, hl, x, f;

#ifdef Z80_HANDLE_SELF_MODIFYING_CODE
                int p, q;

                p = (pc - 2) & 0xffff;
                q = (pc - 1) & 0xffff;
#endif

                d = opcode == OPCODE_INIR ? +1 : -1;

                b = B;
                hl = HL;

                r -= 2;
                elapsed_cycles -= 8;
                for (;;) {

                    r += 2;

                    Z80_INPUT_BYTE(C, x);
                    Z80_WRITE_BYTE(hl, x);

                    hl += d;

                    if (--b)

                        elapsed_cycles += 21;

                    else {

                        f = Z80_Z_FLAG;
                        elapsed_cycles += 16;
                        break;

                    }

#ifdef Z80_HANDLE_SELF_MODIFYING_CODE
                    if (((hl - d) & 0xffff) == p || ((hl - d) & 0xffff) == q) {

                        f = SZYX_FLAGS_TABLE[b];
                        pc -= 2;
                        break;

                    }
#endif

                    if (elapsed_cycles < number_cycles)

                        continue;

                    else {

                        f = SZYX_FLAGS_TABLE[b];
                        pc -= 2;
                        break;

                    }

                }

                HL = hl;
                B = b;

                f |= x >> (7 - Z80_N_FLAG_SHIFT);
                x += (C + d) & 0xff;
                f |= x & 0x0100 ? HC_FLAGS : 0;
                f |= SZYXP_FLAGS_TABLE[(x & 0x07) ^ b] & Z80_P_FLAG;
                F = f;

                break;

            }

        case OUTI_OUTD:
            {

                int x, f;

                READ_BYTE(HL, x);
                Z80_OUTPUT_BYTE(C, x);

                HL += opcode == OPCODE_OUTI ? +1 : -1;

                f = SZYX_FLAGS_TABLE[--B & 0xff] | (x >> (7 -
                        Z80_N_FLAG_SHIFT));
                x += HL & 0xff;
                f |= x & 0x0100 ? HC_FLAGS : 0;
                f |= SZYXP_FLAGS_TABLE[(x & 0x07) ^ B] & Z80_P_FLAG;
                F = f;

                break;

            }

        case OTIR_OTDR:
            {

                int d, b, hl, x, f;

                d = opcode == OPCODE_OTIR ? +1 : -1;

                b = B;
                hl = HL;

                r -= 2;
                elapsed_cycles -= 8;
                for (;;) {

                    r += 2;

                    Z80_READ_BYTE(hl, x);
                    Z80_OUTPUT_BYTE(C, x);

                    hl += d;
                    if (--b)

                        elapsed_cycles += 21;

                    else {

                        f = Z80_Z_FLAG;
                        elapsed_cycles += 16;
                        break;

                    }

                    if (elapsed_cycles < number_cycles)

                        continue;

                    else {

                        f = SZYX_FLAGS_TABLE[b];
                        pc -= 2;
                        break;

                    }

                }

                HL = hl;
                B = b;

                f |= x >> (7 - Z80_N_FLAG_SHIFT);
                x += hl & 0xff;
                f |= x & 0x0100 ? HC_FLAGS : 0;
                f |= SZYXP_FLAGS_TABLE[(x & 0x07) ^ b] & Z80_P_FLAG;
                F = f;

                break;

            }
#endif
    default:
        printf("unhandled instruction %x at %x %x\n", instruction, cpu.pc, opcode);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

