/*
 * pseudo types that are compiler-independent
 * the idea is that data structures in a cross environment have exactly
 * the same size as the native.
 * ALL fields and variables in include files have a type
 * of the form:  [U] INT [8,16.32]
 * the original sources had crap like TINY and COUNT, and so help me,
 * BYTES.  WTF, really?
 *
 * the only exception to this is UINT, which is UINT16 
 *
 * and as for floating point, that's a can of worms that luckily
 * is not going to come up, since there are no shared data structures
 * that use those.
 *
 * include/types.h
 * Changed: <2021-12-23 15:16:27 curt>
 */
typedef char INT8;
typedef unsigned char UINT8;

typedef	short INT16;
typedef unsigned short UINT16;

typedef unsigned short UINT;

#ifdef linux
#define	INTEGER_32
#endif

#ifdef __APPLE__
#define	INTEGER_32
#endif

#ifdef INTEGER_32
typedef int INT32;
typedef unsigned int UINT32;
#else
typedef	long INT32;
typedef unsigned long UINT32;
#endif

union bytepair {
	struct {
		UINT8 low;
		UINT8 high;
	} bytes;
	UINT16 word;
};

#define	min(a,b) (((a) < (b)) ? (a) : (b))
#define	max(a,b) (((a) > (b)) ? (a) : (b))

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
