/*
 * header file for alloc, free and realloc
 *
 * /include/alloc.h
 *
 * Changed: <2023-07-04 11:17:40 curt>
 */

#define pool _pool

struct pool {
	unsigned short size;
	struct pool *next;
};

struct pool *pool;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
