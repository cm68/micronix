// simpleinst.cpp
// Revision 17-dec-2006

#include "simpleinst.h"
#include "parser.h"

#include "trace.h"

#include <stdexcept>


namespace pasmo
{
namespace impl
{

using std::logic_error;

SimpleInst::SimpleInst ()
{ }

SimpleInst::SimpleInst (byte code_n) :
	code (code_n),
	edprefix (true),
	valid8080 (false),
	valid86 (false),
	code86_1 (0),
	code86_2 (0),
	code86_3 (0)
{ }

SimpleInst::SimpleInst (byte code_n, bool edprefix_n, bool valid8080_n) :
	code (code_n),
	edprefix (edprefix_n),
	valid8080 (valid8080_n),
	valid86 (false),
	code86_1 (0),
	code86_2 (0),
	code86_3 (0)
{ }

SimpleInst::SimpleInst (byte code_n, bool edprefix_n, bool valid8080_n,
		unsigned long code86) :
	code (code_n),
	edprefix (edprefix_n),
	valid8080 (valid8080_n),
	valid86 (true),
	code86_1 (static_cast <byte> (code86 >> 16) ),
	code86_2 (hibyte (code86 & 0xFFFF) ),
	code86_3 (lobyte (code86 & 0xFFFF) )
{
	ASSERT ( (code86 & 0xFFFFFF) != 0);
}

SimpleInst::SimpleInst (byte code_n, const string & code86_n) :
	code (code_n),
	edprefix (true),
	valid8080 (false),
	valid86 (true),
	code86 (code86_n.begin (), code86_n.end () )
{
}

bool SimpleInst::isED () const
{
	return edprefix;
}

bool SimpleInst::is8080 () const
{
	return valid8080;
}

bool SimpleInst::is86 () const
{
	return valid86;
}

byte SimpleInst::getcode () const
{
	return code;
}

std::vector <byte> SimpleInst::getcode86 () const
{
	if (code86.empty () )
	{
		std::vector <byte> retcode;
		if (code86_1 != 0)
		{
			retcode.push_back (code86_1);
			retcode.push_back (code86_2);
		}
		else
		{
			if (code86_2 != 0)
				retcode.push_back (code86_2);
		}
		retcode.push_back (code86_3);
		return retcode;
	}
	else
	{
		return code86;
	}
}

const SimpleInst & SimpleInst::get (TypeToken tt)
{
	simpleinst_t::iterator it= simpleinst.find (tt);
	if (it == simpleinst.end () )
		throw logic_error ("Invalid no arg instruction");
	return it->second;
}


SimpleInst::simpleinst_t SimpleInst::simpleinst;

SimpleInst::Initializer::Initializer (simpleinst_t & si)
{
	SimpleInst siCCF  (0x3F, false, true, 0x00F5);
	SimpleInst siCPL  (0x2F, false, true, 0xF6D0);
	SimpleInst siHALT (0x76, false, true, 0x00F4);
	SimpleInst siRLA  (0x17, false, true, 0xD0D0);
	SimpleInst siRLCA (0x07, false, true, 0xD0C0);
	SimpleInst siRRA  (0x1F, false, true, 0xD0D8);
	SimpleInst siRRCA (0x0F, false, true, 0xD0C8);
	SimpleInst siSCF  (0x37, false, true, 0x00F9);

	si [TypeCCF]=       siCCF;
	si [TypeCPD]=       SimpleInst (0xA9);
	si [TypeCPDR]=      SimpleInst (0xB9);
	si [TypeCPI]=       SimpleInst (0xA1);
	si [TypeCPIR]=      SimpleInst (0xB1);
	si [TypeCPL]=       siCPL;
	si [TypeDAA]=       SimpleInst (0x27, false, true, 0x0027);
	si [TypeDI]=        SimpleInst (0xF3, false, true, 0x00FA);
	si [TypeEI]=        SimpleInst (0xFB, false, true, 0x00FB);
	si [TypeEXX]=       SimpleInst (0xD9, false);
	si [TypeHALT]=      siHALT;
	si [TypeIND]=       SimpleInst (0xAA);
	si [TypeINDR]=      SimpleInst (0xBA);
	si [TypeINI]=       SimpleInst (0xA2);
	si [TypeINIR]=      SimpleInst (0xB2);
	si [TypeLDD]=       SimpleInst (0xA8);

	//si [TypeLDDR]=      SimpleInst (0xB8);
	si [TypeLDDR]=      SimpleInst (0xB8,
		string (
			"\xFD"		// STD
			"\x89\xDE"	// MOV SI, BX
			"\x89\xD7"	// MOV DI, DX
			"\xF3\xA4"	// REP MOVSB
			"\x89\xF3"	// MOV BX, SI
			"\x89\xFA"	// MOV DX, DI
			)
		);

	si [TypeLDI]=       SimpleInst (0xA0);

	//si [TypeLDIR]=      SimpleInst (0xB0);
	si [TypeLDIR]=      SimpleInst (0xB0,
		string (
			"\xFC"		// CLD
			"\x89\xDE"	// MOV SI, BX
			"\x89\xD7"	// MOV DI, DX
			"\xF3\xA4"	// REP MOVSB
			"\x89\xF3"	// MOV BX, SI
			"\x89\xFA"	// MOV DX, DI
			)
		);

	si [TypeNEG]=       SimpleInst (0x44);
	si [TypeNOP]=       SimpleInst (0x00, false, true, 0x0090);
	si [TypeOTDR]=      SimpleInst (0xBB, true);
	si [TypeOTIR]=      SimpleInst (0xB3, true);
	si [TypeOUTD]=      SimpleInst (0xAB, true);
	si [TypeOUTI]=      SimpleInst (0xA3, true);
	si [TypeSCF]=       siSCF;
	si [TypeRETI]=      SimpleInst (0x4D);
	si [TypeRETN]=      SimpleInst (0x45);
	si [TypeRLA]=       siRLA;
	si [TypeRLCA]=      siRLCA;
	si [TypeRLD]=       SimpleInst (0x6F);
	si [TypeRRA]=       siRRA;
	si [TypeRRCA]=      siRRCA;
	si [TypeRRD]=       SimpleInst (0x67);

	si [TypeCMA_8080]=  siCPL;
	si [TypeCMC_8080]=  siCCF;
	si [TypeHLT_8080]=  siHALT;
	si [TypePCHL_8080]= SimpleInst (codeJP_indHL, false, true, 0xFFE3);
	si [TypeRAL_8080]=  siRLA;
	si [TypeRAR_8080]=  siRRA;
	si [TypeRC_8080]=   SimpleInst (0xD8, false, true, 0x7301C3);
	si [TypeRET_8080]=  SimpleInst (codeRET, false, true, codeRET_86);
	si [TypeRLC_8080]=  siRLCA;
	si [TypeRM_8080]=   SimpleInst (0xF8, false, true, 0x7901C3);
	si [TypeRNC_8080]=  SimpleInst (0xD0, false, true, 0x7201C3);
	si [TypeRNZ_8080]=  SimpleInst (0xC0, false, true, 0x7401C3);
	si [TypeRP_8080]=   SimpleInst (0xF0, false, true, 0x7801C3);
	si [TypeRPE_8080]=  SimpleInst (0xE8, false, true, 0x7B01C3);
	si [TypeRPO_8080]=  SimpleInst (0xE0, false, true, 0x7A01C3);
	si [TypeRRC_8080]=  siRRCA;
	si [TypeRZ_8080]=   SimpleInst (0xC8, false, true, 0x7501C3);
	si [TypeSPHL_8080]= SimpleInst (codeLD_SP_HL, false, true, 0x89DC);
	si [TypeSTC_8080]=  siSCF;
	si [TypeXCHG_8080]= SimpleInst (0xEB, false, true, 0x87D3);
}

SimpleInst::Initializer SimpleInst::initializer (simpleinst);

} // namespace impl

} // namespace pasmo

// End of simpleinst.cpp
