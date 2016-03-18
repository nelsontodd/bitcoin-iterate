#ifndef BITCOIN_ITERATE_CACHE_H
#define BITCOIN_ITERATE_CACHE_H
#include <ccan/tal/tal.h>
#include "utxo.h"
#include "block.h"

bool read_utxo_cache(const tal_t *ctx,
		       struct utxo_map *utxo_map,
		       const char *cachedir,
		       const u8 *blockid);
  
void write_utxo_cache(const struct utxo_map *utxo_map,
			const char *cachedir,
			const u8 *blockid);

void read_blockcache(const tal_t *tal_ctx,
		       bool quiet,
		       struct block_map *block_map,
		       const char *blockcache,
		       struct block **genesis,
		       char **block_fnames);

void write_blockcache(struct block_map *block_map,
			const char *cachedir,
			const char *blockcache);
  
#endif /* BITCOIN_ITERATE_CACHE_H */
