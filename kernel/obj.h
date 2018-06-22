	/*
	 * Whitesmith's object file format
	 * The file has 5 sections:
	 *	16-byte header
	 *	text
	 *	data
	 *	symbol table
	 *	relocation bytes
	 * The header structure is:
	 */
struct obj
	{
	UCHAR	ident,		/* see below */
		conf;		/* see below */
	UINT	table,		/* symbol table size (bytes) */
		text,		/* text segment size */
		data,		/* data segment size */
		bss,		/* bss segment size (not in file) */
		heap,		/* stack + heap size (not in file) */
		textoff,	/* text offset (origin) */
		dataoff;	/* data offset (origin) */
	};

#define OBJECT	0x99		/* Whitesmith's standard */
#define RELOC	0x14		/* relocation bytes present */
#define NORELOC 0x94		/* no reloc bytes */
