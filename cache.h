#ifndef BITCOIN_ITERATE_CACHE_H
#define BITCOIN_ITERATE_CACHE_H
#include <ccan/tal/tal.h>
#include "utxo.h"
#include "block.h"

bool read_utxo_cache(const tal_t *ctx,
		     bool quiet,
		     struct utxo_map *utxo_map,
		     const char *cachedir,
		     const u8 *blockid);
  
void write_utxo_cache(const struct utxo_map *utxo_map,
		      bool quiet,
		      const char *cachedir,
		      const u8 *blockid);

size_t read_blockchain(tal_t *tal_ctx,
		       bool quiet, bool use_mmap,
		       bool use_testnet, char *cachedir, const char *blockcache,
		       char **block_fnames,
		       struct block_map *block_map, struct block **genesis);

void write_blockcache(struct block_map *block_map,
		      bool quiet,
		      const char *cachedir,
		      const char *blockcache);
  
#endif /* BITCOIN_ITERATE_CACHE_H */
