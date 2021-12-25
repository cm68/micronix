/*
 * header file for alloc, free and realloc
 *
 * include/alloc.h
 * Changed: <2021-12-23 14:41:14 curt>
 */

# define pool _pool

struct pool {
	UINT size;
	struct pool *next;
};

struct pool *pool;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
