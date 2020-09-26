// module.cpp
// Revision 10-oct-2006


#include "module.h"

#include "trace.h"


#include <fstream>
#include <stdexcept>

using std::ifstream;
using std::endl;
using std::runtime_error;
using std::logic_error;
using std::pair;
using std::make_pair;


namespace pasmo {
namespace impl {


//*********************************************************
//			class Module
//*********************************************************


Module::Module (ostream & debout_n, ostream & warnout_n) :
	debout (debout_n.rdbuf () ),
	warnout (warnout_n.rdbuf () ),
	startposdefined (false),
	inuse (ValueProgRelative)
{
}

Module::Module (const Module & module) :
	debout (module.debout.rdbuf () ),
	warnout (module.warnout.rdbuf () ),
	module_name (module.module_name),
	startposdefined (false),
	inuse (ValueProgRelative)
{
}

string Module::getname () const
{
	return module_name;
}

void Module::clear ()
{
	publicname.clear ();
	publicvalue.clear ();
	absseg.clear ();
	codeseg.clear ();
	dataseg.clear ();	
	inuse= ValueProgRelative;
	relative.clear ();
	chainextern.clear ();
	offset.clear ();
}

void Module::setdebout (ostream & debout_n)
{
	debout.rdbuf (debout_n.rdbuf () );
}

bool Module::hasstartpos () const
{
	return startposdefined;
}

address Module::getstartpos () const
{
	ASSERT (startposdefined);
	return getseg (startpos.type).getbase () + startpos.value;
}

void Module::setstartpos (const Value & vstart)
{
	startposdefined= true;
	startpos= vstart;
}

void Module::setcodebase (address base)
{
	codeseg.setbase (base);
}

void Module::setdatabase (address base)
{
	dataseg.setbase (base);
}

address Module::getcodesize () const
{
	return codeseg.getsize ();
}

address Module::getdatasize () const
{
	return dataseg.getsize ();
}

address Module::getafterabs () const
{
	address after= 0;
	if (! absseg.empty () )
	{
		after= absseg.getmaxused ();
		if (after == 0xFFFFu)
			throw runtime_error ("absolute ends at 64 KiB limit");
		else
			++after;
	}
	return after;
}

const Segment & Module::getseg (ValueType typeseg) const
{
	switch (typeseg)
	{
	case ValueAbsolute:
		return absseg;
	case ValueProgRelative:
		return codeseg;
	case ValueDataRelative:
		return dataseg;
	default:
		throw logic_error ("Invalid segment");
	}
}

Segment & Module::getseg (ValueType typeseg)
{
	switch (typeseg)
	{
	case ValueAbsolute:
		return absseg;
	case ValueProgRelative:
		return codeseg;
	case ValueDataRelative:
		return dataseg;
	default:
		throw logic_error ("Invalid segment");
	}
}

const Segment & Module::curseg () const
{
	return getseg (inuse);
}

Segment & Module::curseg ()
{
	return getseg (inuse);
}

void Module::setcurrentsegment (ValueType newseg)
{
	inuse= newseg;
}

ValueType Module::getcurrentsegment () const
{
	return inuse;
}

void Module::gencode (byte b)
{
	curseg ().putbyte (b);
}

void Module::genword (address w)
{
	curseg ().putword (w);
}

void Module::genrelative (Value v)
{
	Segment & seg= curseg ();
	address current= seg.getpos ();

	Value vcur (inuse, current);

	debout << "Generating relative to " << v <<
		" in " << vcur << endl;

	relative.insert (make_pair (vcur, v) );
	seg.putword (v.value);
}

void Module::genextern (const string & varname, address externoff)
{
	TRFUNC (tr, "Module::genextern");

	ASSERT (! varname.empty () );

	Segment & seg= curseg ();
	Value vcurpos (inuse, seg.getpos () );

	debout << "Generating extern refrence to " << varname <<
		" in " << vcurpos << endl;

	address putaddr= 0;
	pair <Chain::iterator, bool> r=
		chainextern.insert (make_pair (varname, vcurpos) );
	if (! r.second)
	{
		TRMESSAGE (tr, "Adding to chain");

		Value & vprev= r.first->second;

		#if 0
		if (vprev.type == ValueAbsolute)
		{
			putaddr= vprev.value;
		}
		else
		{
			relative.insert (make_pair (vcurpos, vprev) );
			vprev= vcurpos;
		}
		#else
		debout << "Adding reference to " << vprev <<
			" in " << vcurpos << endl;

		relative.insert (make_pair (vcurpos, vprev) );
		vprev= vcurpos;
		#endif
	}
	else
	{
		TRMESSAGE (tr, "Chain started");
		debout << "New reference chain started" << endl;
	}

	if (externoff != 0)
	{
		debout << "Extern offset " << hex4 (externoff) << endl;
		Value voff (ValueAbsolute, externoff);
		offset.insert (make_pair (vcurpos, voff) );
	}
	seg.putword (putaddr);
}

byte Module::getbyte (address pos) const
{
	return curseg ().getbyte (pos);
}

Value Module::getpos () const
{
	return Value (inuse, curseg ().getpos () );
}

void Module::setpos (address pos)
{
	curseg ().setpos (pos);
}

void Module::setorg (address pos)
{
	curseg ().setorg (pos);
}

address Module::phase (address value) const
{
	const Segment & seg= curseg ();
	return value - seg.getpos ();
}

void Module::evallimits ()
{
	absseg.evallimits ();
	codeseg.evallimits ();
	dataseg.evallimits ();
}

void Module::evalrelatives ()
{
	TRFUNC (tr, "Module::evalrelatives");

	for (Relative::iterator it= relative.begin ();
		it != relative.end ();
		++it)
	{
		const Value & vorig= it->first;
		const ValueType typeorig= vorig.type;
		Segment & segorig= getseg (typeorig);
		const address segrelpos= vorig.value;

		const Value & vdest= it->second;
		const ValueType typedest= vdest.type;
		const Segment & segdest= getseg (typedest);
		const address absdest= 	segdest.getbase () + vdest.value;

		segorig.setword (segrelpos, absdest);

		address abspos= segrelpos + segorig.getbase ();
		reloctable.insert (abspos);
	}
}

void Module::publicvars (Vars & vars)
{
	TRF;

	for (PublicValue::iterator it= publicvalue.begin ();
		it != publicvalue.end ();
		++it)
	{
		const string & name= it->first;
		const Value & v= it->second;

		address addr= v.value;
		switch (v.type)
		{
		case ValueAbsolute:
			break;
		case ValueProgRelative:
			addr+= codeseg.getbase ();
			break;
		case ValueDataRelative:
			addr+= dataseg.getbase ();
			break;
		default:
			throw logic_error ("Invalid segmentin public var");
		}

		debout << "Making public " << name <<
			" as " << hex4 (addr) << endl;

		if (vars.setvar (name, addr, PreDefined) )
			warnout << "Redefinition of " << name << endl;
		else
		{
			if (vars.ispublic (name) )
				warnout << "Already public: " << name << endl;
			vars.makepublic (name);
		}
	}
}

void Module::solveextern (Vars & vars)
{
	TRFUNC (tr, "Module::solveextern");

	TRMESSAGE (tr, "Resolving externals");

	for (Chain::iterator it= chainextern.begin ();
		it != chainextern.end ();
		++it)
	{
		const string & name= it->first;

		//const address value= vars.getvar (name).getvalue ();
		VarData vd (vars.getvar (name) );
		if (vd.def () == NoDefined)
			throw runtime_error ("Undefined extern: " + name);
		const address value (vd.getvalue () );

		Value v= it->second;
		do
		{
			Segment & seg= getseg (v.type);
			const address pos= v.value;
			Value next (ValueAbsolute, 0);
			Relative::iterator it= relative.find (v);
			if (it != relative.end () )
			{
				next= it->second;
			}
			else
				next.value= seg.getword (pos);

			seg.setword (pos, value);

			reloctable.insert (pos + seg.getbase () );

			v= next;
		} while (v.type != ValueAbsolute || v.value != 0);
	}

	TRMESSAGE (tr, "Adding offsets");

	for (Offset::iterator it= offset.begin ();
		it != offset.end ();
		++it)
	{
		const Value & vpos= it->first;
		const Value & vdest= it->second;
		address offset= getseg (vdest.type).getbase () + vdest.value;

		debout << "Adding " << vdest <<
			"(" << hex4 (offset) << ") at " << vpos << endl;

		Segment & segdest= getseg (vpos.type);

		//address pos= vpos.value;
		//address v= segdest.getword (pos);
		//segdest.setword (pos, v + offset);
		segdest.offsetword (vpos.value, offset);
	}
}

void Module::prog_relative (RelFileIn & rel)
{
	address addr= rel.readaddress ();

	debout << "Prog relative: " << hex4 (addr) << endl;

	genrelative (Value (ValueProgRelative, addr) );
}

void Module::data_relative (RelFileIn & rel)
{
	address addr= rel.readaddress ();

	debout << "Data relative: " << hex4 (addr) << endl;

	genrelative (Value (ValueDataRelative, addr) );
}

void Module::entry_symbol (RelFileIn & rel)
{
	const string name= rel.readname ();
	debout << "Public symbol: " << name << endl;

	pair <PublicName::iterator, bool> r= publicname.insert (name);
	if (! r.second)
		debout << "Already declared as public" << endl;
}

void Module::program_name (RelFileIn & rel)
{
	const string name= rel.readname ();
	debout << "MODULE: " << name << endl;
}

void Module::extension_item (RelFileIn & rel)
{
	const string name= rel.readname ();
	debout << "Extension link item: '" << name << '\'' << endl;
}

void Module::define_entry_point (RelFileIn & rel)
{
	Value v= rel.readvalue ();
	ValueType type= v.type;
	//address addr= v.value;
	debout << "Entry point: " << v << endl;

	switch (type)
	{
	case ValueAbsolute:
		break;
	case ValueProgRelative:
		//addr+= module_prog_base;
		break;
	case ValueDataRelative:
		//addr+= module_data_base;
		break;
	default:
		throw runtime_error
			("Unsupported REL format");
	}
	string name= rel.readname ();

	//debout << "Define: " << hex4 (addr) <<
	debout << "Define: " << v <<
		" as " << name << endl;
	if (publicname.find (name) == publicname.end () )
	{
		debout << "Not declared as public symbol" << endl;
		warnout << "Not declared as public symbol" << endl;
	}

	pair <PublicValue::iterator, bool> r=
		publicvalue.insert (make_pair (name, v) );
	if (! r.second)
	{
		debout << "Already defined " << name << endl;
		warnout << "Already defined "<< name << endl;
	}

	//setvar (name, addr, PreDefined);
}

void Module::chain_external (RelFileIn & rel)
{
	const Value v= rel.readvalue ();
	const string name= rel.readname ();

	debout << "Chain external: " << v << " to " << name << endl;

	chainextern.insert (make_pair (name, v) );

	ValueType type= v.type;
	//address addr= v.value;
	switch (type)
	{
	case ValueAbsolute:
		break;
	case ValueProgRelative:
		//addr+= module_prog_base;
		break;
	case ValueDataRelative:
		//addr+= module_data_base;
		break;
	default:
		throw logic_error
			("Unsupported segment");
	}

	//addchainextern (addr, name);
}

void Module::external_plus_offset (RelFileIn & rel)
{
	Value v= rel.readvalue ();

	debout << "External plus offset: " << v << endl;

	ValueType type= v.type;
	//address addr= v.value;
	
	switch (type)
	{
	case ValueAbsolute:
		break;
	case ValueProgRelative:
		break;
	case ValueDataRelative:
		break;
	default:
		throw logic_error ("Unsupported segment");
	}
	Value curval (inuse, curseg ().getpos () );

	debout << "Setting offset at " << curval << " as " << v << endl;

	offset.insert (make_pair (curval, v) );
}

void Module::data_size (RelFileIn & rel)
{
	Value v= rel.readvalue ();
	debout << "Data size: " << v << endl;

	//data_size= v.value;
	//module_data_base= module_base + prog_size;
	//dataseg.setsize (data_size);
	dataseg.setsize (v.value);
}

void Module::location_counter (RelFileIn & rel)
{
	Value v= rel.readvalue ();
	ValueType type= v.type;
	address addr= v.value;

	debout << "Location counter: " << v << endl;

	switch (type)
	{
	case ValueAbsolute:
		absseg.setpos (addr);
		break;
	case ValueProgRelative:
		codeseg.setpos (addr);
		break;
	case ValueDataRelative:
		dataseg.setpos (addr);
		break;
	default:
		throw runtime_error ("Unsupported segment");
	}
	//inuse= type;
	setcurrentsegment (type);
}

void Module::program_size (RelFileIn & rel)
{
	Value v= rel.readvalue ();
	debout << "Program size: " << v << endl;

	//prog_size= v.value;
	//module_prog_base= module_base;
	//module_data_base= module_base + prog_size;
	//codeseg.setsize (prog_size);
	codeseg.setsize (v.value);
}

void Module::end_module (RelFileIn & rel)
{
	address type= rel.readaddresstype ();
	address pos= rel.readaddress ();
	rel.skiptobyte ();
	if (type != RelFile::TypeAbsolute || pos != 0)
	{
		setstartpos (Value (ValueType (type & 0x3), pos) );
		debout << "End of module start at" << hex4 (pos) << endl;
	}
	else
	{
		debout << "End of module, no start" << endl;
	}
}

void Module::loadrelfile (const string & relname)
{
	ifstream relis (relname.c_str (), std::ios::in | std::ios::binary);
	if (! relis.is_open () )
		throw runtime_error ("Can't open module " + relname);

	module_name= relname;

	RelFileIn rel (relis);

	for (bool end_rel= false; ! end_rel; )
	{
		address type= rel.readtype ();
		switch (type)
		{
		case RelFile::TypeByte:
			gencode (rel.readchar () );
			break;
		case RelFile::TypeAbsolute:
			throw runtime_error ("Absolute segment unsupported");
		case RelFile::TypeProgRelative:
			prog_relative (rel);
			break;
		case RelFile::TypeDataRelative:
			data_relative (rel);
			break;
		case RelFile::TypeCommonRelative:
			throw runtime_error ("Common unsupported");
		case RelFile::TypeEntrySymbol:
			entry_symbol (rel);
			break;
		case RelFile::TypeSelectCommon:
			throw runtime_error ("Common unsupported");
		case RelFile::TypeProgramName:
			program_name (rel);
			break;
		case RelFile::TypeExtensionItem:
			extension_item (rel);
			break;
		case RelFile::TypeCommonSize:
			throw runtime_error ("Common unsupported");
		case RelFile::TypeChainExternal:
			chain_external (rel);
			break;
		case RelFile::TypeDefineEntryPoint:
			define_entry_point (rel);
			break;
		case RelFile::TypeExternalPlusOffset:
			external_plus_offset (rel);
			break;
		case RelFile::TypeDataSize:
			data_size (rel);
			break;
		case RelFile::TypeLocationCounter:
			location_counter (rel);
			break;
		case RelFile::TypeChainAddress:
			throw runtime_error ("Chain address unsupported");
		case RelFile::TypeProgramSize:
			program_size (rel);
			break;
		case RelFile::TypeEndModule:
			end_module (rel);
			break;
		case RelFile::TypeEndFile:
			debout << "End of REL file" << endl;
			end_rel= true;
			break;
		default:
			throw runtime_error ("Unrecognized REL format");
		}
	}
	absseg.evallimits ();
}

void Module::savesegment (RelFileOut & rel, ValueType type)
{
	TRFUNC (tr, "Module::savesegment");

	Segment & seg (getseg (type) );

	#if 0
	if (seg.empty () )
		return;

	size_t minused= seg.getminused ();
	size_t maxused= seg.getmaxused ();
	rel.putlocationcounter (type, minused);
	#else

	size_t minused= seg.getminused ();
	size_t maxused= seg.getmaxused ();
	if (seg.empty () )
	{
		if (seg.hasorg () )
		{
			rel.putlocationcounter (type, seg.getorg () );
		}
		return;
	}
	else
	{
		rel.putlocationcounter (type, minused);
	}

	#endif

	bool inbytes= false;
	for (size_t i= minused; i <= maxused; ++i)
	{
		Value v (type, i);

		Offset::iterator itoff= offset.find (v);
		if (itoff != offset.end () )
		{
			if (inbytes)
			{
				debout << endl;
				inbytes= false;
			}
			const Value & voff= itoff->second;
			debout << "Offset: " << voff << endl;
			rel.putexternalplusoffset (voff.type, voff.value);
		}

		Relative::iterator it= relative.find (v);
		if (it != relative.end () )
		{
			if (inbytes)
			{
				debout << endl;
				inbytes= false;
			}
			const Value & vdest= it->second;

			debout << "Relative: " << vdest << endl;
			rel.putworditem (vdest);

			#if 0
			const ValueType typedest= vdest.type;
			address valuedest= vdest.value;

			debout << "Relative: " << vdest << endl;
			switch (typedest)
			{
			case ValueAbsolute:
				rel.putabsolute (valuedest);
				break;
			case ValueProgRelative:
				rel.putprogramrelative (valuedest);
				break;
			case ValueDataRelative:
				rel.putdatarelative (valuedest);
				break;
			default:
				throw logic_error ("Unsupported type");
			}
			#endif
			++i;
		}
		else
		{
			if (! inbytes)
			{
				debout << "Bytes at: " << hex4 (i) << ':';
				inbytes= true;
			}
			debout << ' ' << hex2 (seg.getbyte (i) );
			rel.putbyteitem (seg.getbyte (i) );
		}
	}
	if (inbytes)
		debout << endl;
}

void Module::saverel (RelFileOut & rel)
{
	TRFUNC (tr, "Module::saverel");

	address s= dataseg.empty () ? 0 : dataseg.getmaxused () + 1;
	rel.putdatasize (s);
	s= codeseg.empty () ? 0 : codeseg.getmaxused () + 1;
	rel.putprogramsize (s);

	TRMESSAGE (tr, "Absolute segment");
	debout << "Absolute segment" << endl;
	savesegment (rel, ValueAbsolute);
	TRMESSAGE (tr, "Code segment");
	debout << "Code segment" << endl;
	savesegment (rel, ValueProgRelative);
	TRMESSAGE (tr, "Data segment");
	debout << "Data segment" << endl;
	savesegment (rel, ValueDataRelative);

	for (Chain::iterator it= chainextern.begin ();
		it != chainextern.end ();
		++it)
	{
		const Value & v= it->second;
		rel.putchainexternal (v.type, v.value, it->first);
	}
}

void Module::closerel (RelFileOut & rel)
{
	if (startposdefined)
	{
		rel.putendmodule (startpos);
	}
	else
	{
		rel.putendmodule ();
	}
}

void Module::setentrypoint (const string & name, const Value & v)
{
	publicvalue [name]= v;
}


namespace {


class MakePublicRel {
public:
	MakePublicRel (RelFileOut & rel_n) :
		rel (rel_n)
	{ }
	void operator () (const Module::PublicValue::value_type & v);
private:
	RelFileOut & rel;
	typedef set <string> Pub;
	Pub pub;
};

void MakePublicRel::operator () (const Module::PublicValue::value_type & v)
{
	const string & name= v.first;

	//cerr << "PUBLIC " << s << " as " << vd.def () << endl;

	const string exname= rel.putentrysymbol (name);
	std::pair <Pub::iterator, bool> r (pub.insert (exname) );
	if (! r.second)
		throw runtime_error ("PUBLIC '" + name + "' repeated");
}


class PutEntryPoints {
public:
	PutEntryPoints (RelFileOut & rel_n) :
		rel (rel_n)
	{ }
	void operator () (const Module::PublicValue::value_type & v);
private:
	RelFileOut & rel;
};

void PutEntryPoints::operator () (const Module::PublicValue::value_type & v)
{
	const string & name= v.first;
	const Value & val= v.second;

	rel.putdefineentrypoint (name, val);
}


} // namespace


void Module::savemodule (RelFileOut & rel, const string & modulename)
{
	TRF;

	rel.putprogramname (modulename);

	for_each (publicvalue.begin (), publicvalue.end (),
		MakePublicRel (rel) );
	saverel (rel);
	for_each (publicvalue.begin (), publicvalue.end (),
		PutEntryPoints (rel) );

	closerel (rel);
}

void Module::install (Segment & mainseg)
{
	TRFUNC (tr, "Module::install");

	TRMESSAGE (tr, "absolute");
	//absseg.evallimits ();
	absseg.copy (mainseg);
	TRMESSAGE (tr, "code");
	codeseg.copy (mainseg);
	TRMESSAGE (tr, "data");
	dataseg.copy (mainseg);
}


} // namespace impl
} // namespace pasmo


// End of module.cpp
