// codeaux.cpp
// Revision 20-nov-2005

#include "codeaux.h"

#include "trace.h"


#include <stdexcept>


namespace pasmo {
namespace impl {


using std::logic_error;

namespace {

logic_error UnexpectedRegisterCode ("Unexpected register code");
logic_error InvalidPrefixUsed ("Invalid use of prefix");
logic_error InvalidRegisterUsed ("Invalid register used");
logic_error InvalidInstructionType ("Invalid instruction type");
logic_error UnexistentMode ("Mode of code generation invalid");

char openIndir=  '[';
char closeIndir= ']';

} // namespace


byte getregb86 (regbCode rb)
{
	switch (rb)
	{
	case regA: return reg86AL;
	case regB: return reg86CH;
	case regC: return reg86CL;
	case regD: return reg86DH;
	case regE: return reg86DL;
	case regH: return reg86BH;
	case regL: return reg86BL;
	default:
		ASSERT (false);
		throw UnexpectedRegisterCode;
	}
}

string nameHLpref (byte prefix)
{
	switch (prefix)
	{
	case prefixNone: return "HL";
	case prefixIX: return "IX";
	case prefixIY: return "IY";
	default:
		throw InvalidPrefixUsed;
	}
}

string regwName (regwCode code, bool useSP, byte prefix)
{
	ASSERT (code == regHL || prefix == prefixNone);

	switch (code)
	{
	case regBC: return "BC";
	case regDE: return "DE";
	case regHL: return nameHLpref (prefix);
	case regAF: return useSP ? "SP" : "AF";
	default:
		throw InvalidRegisterUsed;
	}
}

string regw8080Name (regwCode code)
{
	switch (code)
	{
	case regBC: return "B";
	case regDE: return "D";
	case regHL: return "H";
	case regSP: return "SP";
	default:
		throw InvalidRegisterUsed;
	}
}

string regw8080NamePSW (regwCode code)
{
	switch (code)
	{
	case regBC: return "B";
	case regDE: return "D";
	case regHL: return "H";
	case regAF: return "PSW";
	default:
		throw InvalidRegisterUsed;
	}
}

string byteinstName (TypeByteInst ti)
{
	switch (ti)
	{
	case tiADDA: return "ADD A,";
	case tiADCA: return "ADC A,";
	case tiSUB:  return "SUB";
	case tiSBCA: return "SBC A,";
	case tiAND:  return "AND";
	case tiXOR:  return "XOR";
	case tiOR:   return "OR";
	case tiCP:   return "CP";
	default:
		throw InvalidInstructionType;
	}
}

string byteinst8080Name (TypeByteInst ti)
{
	switch (ti)
	{
	case tiADDA: return "ADD";
	case tiADCA: return "ADC";
	case tiSUB:  return "SUB";
	case tiSBCA: return "SBB";
	case tiAND:  return "ANA";
	case tiXOR:  return "XRA";
	case tiOR:   return "ORA";
	case tiCP:   return "CPM";
	default:
		throw InvalidInstructionType;
	}
}

string byteinst8080iName (TypeByteInst ti)
{
	switch (ti)
	{
	case tiADDA: return "ADI";
	case tiADCA: return "ACI";
	case tiSUB:  return "SUI";
	case tiSBCA: return "SBI";
	case tiAND:  return "ANI";
	case tiXOR:  return "XRI";
	case tiOR:   return "ORI";
	case tiCP:   return "CPI";
	default:
		throw InvalidInstructionType;
	}
}

byte getbaseByteInst (TypeByteInst ti, GenCodeMode genmode)
{
	static byte byte86 []=
		{ 0x00, 0x10, 0x28, 0x18, 0x20, 0x30, 0x08, 0x38 };
	switch (genmode)
	{
	case gen80:
		return 0x80 | (ti << 3);
	case gen86:
		return byte86 [ti];
	default:
		ASSERT (false);
		throw UnexistentMode;
	}
}

byte getByteInstInmediate (TypeByteInst ti, GenCodeMode genmode)
{
	static byte byte86 []=
		{ 0x04, 0x14, 0x2C, 0x1C, 0x24, 0x34, 0x0C, 0x3C };
	switch (genmode)
	{
	case gen80:
		return 0xC6 | (ti << 3);
	case gen86:
		return byte86 [ti];
	default:
		ASSERT (false);
		throw UnexistentMode;
	}
}

string nameIdesp (byte prefix, bool hasdesp, byte desp)
{
	string r (1, openIndir);
	r+= nameHLpref (prefix);
	if (hasdesp)
	{
		r+= '+';
		r+= hex2str (desp);
	}
	r+= closeIndir;
	return r;
}

string getregbname (regbCode rb, byte prefix,
	bool hasdesp, byte desp)
{
	ASSERT (! hasdesp || (rb == reg_HL_ && prefix != prefixNone) );

	switch (rb)
	{
	case regA: return "A";
	case regB: return "B";
	case regC: return "C";
	case regD: return "D";
	case regE: return "E";
	case regH:
		switch (prefix)
		{
		case prefixNone: return "H";
		case prefixIX: return "IXH";
		case prefixIY: return "IYH";
		default:
			throw InvalidPrefixUsed;
		}
	case regL:
		switch (prefix)
		{
		case prefixNone: return "L";
		case prefixIX: return "IXL";
		case prefixIY: return "IYL";
		default:
			throw InvalidPrefixUsed;
		}
	case reg_HL_:
		return nameIdesp (prefix, hasdesp, desp);
	default:
		ASSERT (false);
		throw UnexpectedRegisterCode;
	}
}

string getregb8080name (regbCode rb)
{
	switch (rb)
	{
	case regA: return "A";
	case regB: return "B";
	case regC: return "C";
	case regD: return "D";
	case regE: return "E";
	case regH: return "H";
	case regL: return "L";
	case reg_HL_: return "M";
	default:
		ASSERT (false);
		throw UnexpectedRegisterCode;
	}
}


} // namespace impl
} // namespace pasmo


// End of codeaux.cpp
