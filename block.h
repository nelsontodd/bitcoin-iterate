#ifndef BITCOIN_ITERATE_BLOCK_H
#define BITCOIN_ITERATE_BLOCK_H
#include <ccan/htable/htable_type.h>
#include <ccan/short_types/short_types.h>
#include "utils.h"
#include "types.h"

const u8 *keyof_block_map(const struct block *b);
bool block_eq(const struct block *b, const u8 *key);
HTABLE_DEFINE_TYPE(struct block, keyof_block_map, hash_sha, block_eq, block_map);

bool add_block(struct block_map *block_map, struct block *b, struct block **genesis, char **block_fnames, size_t *num_misses);
bool set_height(struct block_map *block_map, struct block *b);

#endif /* BITCOIN_ITERATE_BLOCK_H */
