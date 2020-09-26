#ifndef INCLUDE_PASMOIMPL_H
#define INCLUDE_PASMOIMPL_H

// pasmoimpl.h
// Revision 10-aug-2005


#include "pasmotypes.h"


namespace pasmo {
namespace impl {


// Register codes used in some instructions.

enum regwCode {
	regBC= 0, regDE= 1, regHL= 2, regAF= 3, regSP= 3,

	regwInvalid= 8
};
// 8086 equivalents:
//	CX        DX        BX

enum regbCode {
	regB= 0,    regC= 1,    regD= 2,    regE= 3,
	regH= 4,    regL= 5,    reg_HL_= 6, regA= 7,

	reg86AL= 0, reg86CH= 5, reg86CL= 1, reg86DH= 6,
	reg86DL= 2, reg86BH= 7, reg86BL= 3,

	regbInvalid= 8
};

enum TypeByteInst {
	tiADDA= 0,
	tiADCA= 1,
	tiSUB=  2,
	tiSBCA= 3,
	tiAND=  4,
	tiXOR=  5,
	tiOR=   6,
	tiCP=   7
};

const byte
	prefixIX=   0xDD,
	prefixIY=   0xFD,
	prefixNone= 0x00,
	codeRL=     0x10,
	codeRLC=    0x00,
	codeRR=     0x18,
	codeRRC=    0x08,
	codeSLA=    0x20,
	codeSRA=    0x28,
	codeSRL=    0x38,
	codeSLL=    0x30,
	codeBIT=    0x40,
	codeRES=    0x80,
	codeSET=    0xC0,
	codeLD_A_I= 0x57,
	codeLD_A_R= 0x5F,
	codeLD_I_A= 0x47,
	codeLD_R_A= 0x4F;

inline bool isvalidprefix (byte b)
{
	return b == prefixNone || b == prefixIX || b == prefixIY;
}

} // namespace impl
} // namespace pasmo


#endif

// End of pasmoimpl.h
