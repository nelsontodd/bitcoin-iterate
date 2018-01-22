#ifndef BITCOIN_ITERATE_CACHE_H
#define BITCOIN_ITERATE_CACHE_H
#include <ccan/tal/tal.h>
#include "utxo.h"
#include "block.h"

/**
 *  Reads and assembles the blockchain.
 *
 *  If a cache directory was given and a valid & current block cache
 *  is present in that directory, it will be used to quickly recover
 *  the block_map data.
 * 
 *  Otherwise, the block files will be read and the blockchain
 *  assembled.  If a cache directory was given, the block_map data
 *  will be written to the cache.
 *
 *  In either case, the block_map struct will be initialized and
 *  populated with blocks.
 *
 *  @param tal_ctx      -- pointer to the tal context
 *  @param quiet        -- whether to silence output
 *  @param use_mmap     -- whether to use memory mapping to handle block files
 *  @param use_testnet  -- whether to use testnet
 *  @param cachedir     -- the cache directory (string)
 *  @param block_fnames -- an array of block filenames (strings)
 *  @param block_map    -- pointer to the block map to populate
 *  @param genesis      -- pointer to the genesis block struct
 *  @param block_end    -- height of the block to stop reading at (-1 for end of chain)
 *
 *  @return the number of blocks parsed
 */
size_t read_blockchain(tal_t *tal_ctx,
		       bool quiet, bool use_mmap, bool use_testnet, 
		       char *cachedir,
		       char **block_fnames,
		       struct block_map *block_map,
		       struct block **genesis,
		       unsigned long block_end);

/**
 * Reads the UTXO cache.
 *
 *  @param ctx          -- pointer to the tal context
 *  @param quiet        -- whether to silence output
 *  @param utxo_map     -- pointer to the UTXO map to populate
 *  @param cachedir     -- the cache directory (string)
 *  @param blockid      -- the ID of the block this cache is valid for
 * 
 */
bool read_utxo_cache(const tal_t *ctx,
		     bool quiet,
		     struct utxo_map *utxo_map,
		     const char *cachedir,
		     const u8 *blockid);
  
/**
 * Writes the UTXO cache.
 *
 *  @param utxo_map     -- pointer to the UTXO map to populate
 *  @param quiet        -- whether to silence output
 *  @param cachedir     -- the cache directory (string)
 *  @param blockid      -- the ID of the block this cache is valid for
 * 
 */
void write_utxo_cache(const struct utxo_map *utxo_map,
		      bool quiet,
		      const char *cachedir,
		      const u8 *blockid);

#endif /* BITCOIN_ITERATE_CACHE_H */
