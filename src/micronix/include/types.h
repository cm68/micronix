/*
 * pseudo storage classes
 */
#define AFAST  register
#define FAST   register
#define GLOBAL extern
#define IMPORT extern
#define INTERN static
#define LOCAL  static

/*
 *  pseudo types
 */
typedef char TEXT, TINY;
typedef double DOUBLE;
typedef float FLOAT;
typedef int ARGINT, BOOL, VOID;
typedef long LONG;
typedef short BITS, COUNT;
typedef unsigned BYTES;
typedef unsigned char UTINY;
typedef unsigned char UCHAR;
typedef unsigned short UCOUNT;
typedef unsigned short UINT;

#ifdef linux
typedef unsigned int ULONG;
#else
typedef unsigned long ULONG;
#endif
