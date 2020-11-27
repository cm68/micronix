/*
 * header file for alloc, free and realloc
 */

# define pool _pool

struct pool {
	UINT size;
	struct pool *next;
};

struct pool *pool;
