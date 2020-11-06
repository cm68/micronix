/*                                                                               
 * pseudo storage classes                                                        
 */
# define AFAST  register                                                        
# define FAST   register                                                         
# define GLOBAL extern                                                           
# define IMPORT extern
# define INTERN static
# define LOCAL  static

/*
 *  pseudo types
 */
typedef char TEXT, TINY;
typedef double DOUBLE;
typedef float FLOAT;
typedef int ARGINT, BOOL, VOID;
typedef long LONG;
typedef short BITS, COUNT, FILE;
typedef unsigned BYTES;
typedef unsigned char UTINY;
typedef unsigned long ULONG;
typedef unsigned short UCOUNT;


