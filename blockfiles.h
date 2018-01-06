#ifndef BITCOIN_ITERATE_BLOCKFILES_H
#define BITCOIN_ITERATE_BLOCKFILES_H
#include <ccan/tal/tal.h>
#include "block.h"

/**
 * Returns an array in which each string is the path to a blockfile on
 * disk.
 * 
 * @param ctx     -- pointer to tal context
 * @param base    -- path of the blocks directory (string)
 * @param testnet -- whether to use testnet
 */

char **block_filenames(tal_t *ctx, const char *base, bool testnet);

/**
 * Returns an open file handle for the block file at the given index
 * in the array of block filenames.
 *
 * Keeps a temporary cache so repeated calls with the same array of
 * block filenames and index will return the same open file handle.
 *
 * @param block_fnames -- an array of block filenames (strings)
 * @param index        -- the index of the block file to open in the array of filenames
 * @param use_mmap     -- whether to use memory mapping when handling block files
 *
 */
struct file *block_file(char **block_fnames, unsigned int index, bool use_mmap);

/**
 * Reads blockfiles from disk from the genesis block to the given end
 * block (or end of chain).
 *
 * The block map given will be initialized and populated with blocks
 * as they are parsed.
 *
 * Block parsing may extend up to 100 blocks past the intended ending
 * block (to avoid minor forks).
 * 
 * @param tal_ctx      -- pointer to tal context
 * @param use_testnet  -- whether to use testnet
 * @param quiet        -- whether to silence output
 * @param use_mmap     -- whether to use memory mapping when handling block files
 * @param block_fnames -- an array of block filenames (strings)
 * @param block_map    -- the block map to populate
 * @param genesis      -- pointer to genesis block struct
 * @param block_end    -- height of specified end block (-1 for end of chain)
 * @return number of blocks parsed
 * 
 */
size_t read_blockfiles(tal_t *tal_ctx,
		       bool use_testnet, bool quiet, bool use_mmap,
		       char **block_fnames, struct block_map *block_map,
		       struct block **genesis, unsigned long block_end);

#endif /* BITCOIN_ITERATE_BLOCKFILES_H */
