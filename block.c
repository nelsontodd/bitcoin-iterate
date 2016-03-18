#include <ccan/tal/tal.h>
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

void add_block(struct block_map *block_map, struct block *b, struct block **genesis, char **block_fnames)
{
  struct block *old = block_map_get(block_map, b->sha);
  if (old) {
    warnx("Already have "SHA_FMT" from %s %lu/%u",
	  SHA_VALS(b->sha),
	  block_fnames[old->filenum],
	  old->pos, old->bh.len);
    block_map_delkey(block_map, b->sha);
  }
  block_map_add(block_map, b);
  if (is_zero(b->bh.prev_hash)) {
    *genesis = b;
    b->height = 0;
  }
}

bool set_height(struct block_map *block_map, struct block *b)
{
  struct block *i, *prev;

  if (b->height != -1)
    return true;

  i = b;
  do {
    prev = block_map_get(block_map, i->bh.prev_hash);
    if (!prev) {
      warnx("Block "SHA_FMT" has unknown prev "SHA_FMT,
	    SHA_VALS(i->sha),
	    SHA_VALS(i->bh.prev_hash));
      /* Remove it, and all children. */
      for (; i; i = i->next)
	block_map_del(block_map, i);
      return false;
    }
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

