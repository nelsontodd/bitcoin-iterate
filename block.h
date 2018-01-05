#ifndef BITCOIN_ITERATE_BLOCK_H
#define BITCOIN_ITERATE_BLOCK_H
#include <ccan/htable/htable_type.h>
#include <ccan/short_types/short_types.h>
#include "utils.h"
#include "types.h"

/* Defines the block_map and the following methods:
 *
 *   block_map_init
 *   block_map_clear
 *   block_map_add
 *   block_map_del
 *   block_map_get
 *   block_map_first
 *   block_map_next
 *
 * See ccan/htable/htable_type.h for more details.
 * 
 */
HTABLE_DEFINE_TYPE(struct block, keyof_block_map, hash_sha, block_eq, block_map);

/* Return the SHA256 hash for the given block.
 *
 * This function is used to match blocks to keys in the block_map.
 *
 *  @param b -- pointer to a block struct
 */
const u8 *keyof_block_map(const struct block *b);

/* Does the given block match the given key/hash?
 *
 *  @param b   -- pointer to a block struct
 *  @param key -- pointer to a SHA256 block hash
 */
bool block_eq(const struct block *b, const u8 *key);

/* Add the given block to the block map.
 *
 * If the block already exists in the block_map, it will be replaced
 * by the given block (and a warning generated).
 *
 * If the block being added has a zero hash, it will be recognized as
 * the genesis block.
 *
 * If a block with hash equal to the prev_hash of the block being
 * added is found, the block being added will have its height set and
 * the number of misses reset.  Otherwise, the number of misses will
 * be incremented.
 *
 * If the number of misses reaches 1000, set_height is called in an
 * attempt to connect known blocks.
 *
 *  @param block_map    -- pointer to the block map
 *  @param b            -- pointer to a block struct
 *  @param genesis      -- pointer to the genesis block struct
 *  @param block_fnames -- pointer to an array of block filename strings
 *  @param num_misses   -- pointer to the number of misses
 *
 */
bool add_block(struct block_map *block_map, struct block *b, struct block **genesis, char **block_fnames, size_t *num_misses);

/* Sets the heights of blocks in the block map if possible.
 *
 * Finds the last block with a known height then iterates forward
 * setting the height of the rest of the blocks.
 *
 *  @param block_map -- pointer to the block map
 *  @param b         -- pointer to a block struct
 */
bool set_height(struct block_map *block_map, struct block *b);

#endif /* BITCOIN_ITERATE_BLOCK_H */
