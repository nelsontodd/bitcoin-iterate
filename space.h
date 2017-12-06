/*******************************************************************************
 *
 *  = space.h
 *
 *  Defines functions for allocating space.
 *
 */
#ifndef BITCOIN_ITERATE_SPACE_H
#define BITCOIN_ITERATE_SPACE_H
#include <ccan/tal/tal.h>
#include <assert.h>

/**
 * space - Simple bump allocator used to label space acquired via tal.
 *
 * Set to 3MB because blocks are capped at 1MB (as of
 * 2016-03-24 ¯\_(ツ)_/¯).
 * 
 */
struct space {
	char buf[3* 1024 * 1024];
	size_t off;
};

/**
 * space_init - Initialize new space
 * @space: Allocated space
 */
static inline void space_init(struct space *space)
{
	space->off = 0;
}

/**
 * space_alloc - Allocate new space
 * @space: Space to allocate
 * @bytes: Size to allocate
 */
static inline void *space_alloc(struct space *space, size_t bytes)
{
	void *p = space->buf + space->off;
	/* If this fails, enlarge buf[] above */
	//assert(space->off + bytes <= sizeof(space->buf) && "message");

	space->off += bytes;
	return p;
}

/**
 * space_alloc_arr - Allocate an array of identical spaces for a given type.
 * @space: Space to allocate
 * @type:  Type to use to size each space
 * @num:   Number of spaces to allocate (length of array)
 */
#define space_alloc_arr(space, type, num) \
	((type *)space_alloc((space), sizeof(type) * (num)))

#endif /* BITCOIN_ITERATE_SPACE_H */
