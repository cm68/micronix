#ifndef INCLUDE_MODULE_H
#define INCLUDE_MODULE_H

// module.h
// Revision 3-jul-2006


#include "segment.h"
#include "relfile.h"
#include "var.h"


#include <set>
#include <map>


namespace pasmo {
namespace impl {


//*********************************************************
//			class Module
//*********************************************************


class Module {
public:
	Module (ostream & debout_n, ostream & warnout_n);
	Module (const Module & module);

	string getname () const;

	void clear ();

	void setdebout (ostream & debout_n);

	void loadrelfile (const string & relname);
	void saverel (RelFileOut & rel);
	void closerel (RelFileOut & rel);

	void setentrypoint (const string & name, const Value & v);
	void savemodule (RelFileOut & rel, const string & modulename);

	bool hasstartpos () const;
	address getstartpos () const;
	void setstartpos (const Value & vstart);

	void setcodebase (address base);
	void setdatabase (address base);
	address getcodesize () const;
	address getdatasize () const;

	address getafterabs () const;

	void setcurrentsegment (ValueType newseg);
	ValueType getcurrentsegment () const;
	void gencode (byte b);
	void genword (address w);
	void genrelative (Value v);
	void genextern (const string & varname, address externoff);

	byte getbyte (address pos) const;
	Value getpos () const;
	void setpos (address pos);
	void setorg (address pos);
	address phase (address value) const;

	void evallimits ();

	void evalrelatives ();
	void publicvars (Vars & vars);
	void solveextern (Vars & vars);
	void install (Segment & mainseg);

	typedef std::map <Value, Value> Relative;
	typedef std::set <string> PublicName;
	typedef std::map <string, Value> PublicValue;
	typedef std::map <string, Value> Chain;
	typedef std::map <Value, Value> Offset;
	typedef std::set <address> RelocTable;
private:
	ostream debout;
	ostream warnout;

	string module_name;

	bool startposdefined;
	Value startpos;

	PublicName publicname;
	PublicValue publicvalue;

	Segment absseg;
	Segment codeseg;
	Segment dataseg;

	ValueType inuse;

	//typedef std::map <address, Value> Relative;
	//Relative relinabs;
	//Relative relincode;
	//Relative relindata;
	Relative relative;

	Chain chainextern;
	Offset offset;

	RelocTable reloctable;

	const Segment & getseg (ValueType typeseg) const;
	Segment & getseg (ValueType typeseg);
	const Segment & curseg () const;
	Segment & curseg ();

	void prog_relative (RelFileIn & rel);
	void data_relative (RelFileIn & rel);
	void entry_symbol (RelFileIn & rel);
	void program_name (RelFileIn & rel);
	void extension_item (RelFileIn & rel);
	void define_entry_point (RelFileIn & rel);
	void chain_external (RelFileIn & rel);
	void external_plus_offset (RelFileIn & rel);
	void data_size (RelFileIn & rel);
	void location_counter (RelFileIn & rel);
	void program_size (RelFileIn & rel);
	void end_module (RelFileIn & rel);

	void savesegment (RelFileOut & rel, ValueType type);
};


} // namespace impl
} // namespace pasmo

#endif

// End of module.h
