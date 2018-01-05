#ifndef BITCOIN_ITERATE_BLOCKFILES_H
#define BITCOIN_ITERATE_BLOCKFILES_H
#include <ccan/tal/tal.h>
#include "block.h"

/* block_filenames - Return a tal_array of filenames. 
 * @param ctx      -- pointer to tal
 * @param base     -- pointer to base pathname
 * @param testnet3 -- if true, blocks are testnet
 */

char **block_filenames(tal_t *ctx, const char *base, bool testnet3);

/**
 * block_file - open a file and put it into cache.
 * @param block_fnames -- pointer to array of block file names 
 * @param index        -- block file index in file name array
 * @param use_mmap     -- map files into memory 
 */

struct file *block_file(char **block_fnames, unsigned int index, bool use_mmap);

/**
 * read_blockfiles - build block map by iterating from block file 0 until (user specified) end or end of chain
 * @param tal_ctx      -- used to create blocks 
 * @param use_testnet  -- true if parsing testnet blocks
 * @param quiet        -- true if silence output
 * @param use_mmap     -- map files into memory 
 * @param block_fnames -- names of files containing block data
 * @param block_map    -- block map to add parsed blocks to
 * @param genesis      -- pointer to genesis block struct
 * @param block_end    -- height of specified end block
 */
size_t read_blockfiles(tal_t *tal_ctx, bool use_testnet, bool quiet, bool use_mmap, char **block_fnames, struct block_map *block_map, struct block **genesis, unsigned long block_end);

#endif /* BITCOIN_ITERATE_BLOCKFILES_H */

