#include <ccan/err/err.h>
#include "block.h"
#include "utils.h"

const u8 *keyof_block_map(const struct block *b)
{
  return b->sha;
}

bool block_eq(const struct block *b, const u8 *key)
{
  return memcmp(b->sha, key, sizeof(b->sha)) == 0;
}

bool add_block(struct block_map *block_map, struct block *b,
		      struct block **genesis, char **block_fnames, size_t *num_misses)
{
	struct block *prev, *old = block_map_get(block_map, b->sha);

	if (old) {
		warnx("Already have "SHA_FMT" from %s %lu/%u",
		      SHA_VALS(b->sha),
		      block_fnames[old->filenum],
		      old->pos, old->bh.len);
		block_map_delkey(block_map, b->sha);
	}
	
	block_map_add(block_map, b);
	if (is_zero(b->bh.prev_hash)) {//so hash of first bh is zero
		*genesis = b;
		b->height = 0;
		*num_misses = 0;
		return true;
	}

	/* Optimistically search for previous: blocks usually in rough order */
	prev = block_map_get(block_map, b->bh.prev_hash); 
	if (prev) {
		if (prev->height != -1) {
			b->height = prev->height + 1; //if previous height is known, this bh is known
			*num_misses = 0;
			return true;
		}

		/* Every 1000 blocks we didn't get height for, try recursing. */
		if ((*num_misses)++ % 1000 == 0) { //iterate num_misses even if statement false
			if (set_height(block_map, b)) {  //try to set block height if too many blocks have height -1 - important to note that if u miss height for one block u miss it for the following blocks (i think)
				*num_misses = 0;
				return true;
			}
		}
	}
	return false;
}

bool set_height(struct block_map *block_map, struct block *b)
{
	struct block *i, *prev;

	if (b->height != -1)
		return true;

	i = b;
	do {
		prev = block_map_get(block_map, i->bh.prev_hash);
		if (!prev)
			return false;
		prev->next = i;
		i = prev;
	} while (i->height == -1);

	/* Now iterate forward, setting height for all. */
	b->next = NULL;
	for (i = prev->next; i; i = i->next) {
		i->height = prev->height + 1;
		prev = i;
	}
	return true;
}
