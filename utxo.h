#ifndef BITCOIN_ITERATE_UTXO_H
#define BITCOIN_ITERATE_UTXO_H
#include <sys/types.h>
#include <ccan/tal/tal.h>
#include <ccan/htable/htable_type.h>
#include <ccan/short_types/short_types.h>
#include "types.h"
#include "utils.h"

const u8 *keyof_utxo(const struct utxo *utxo);
bool utxohash_eq(const struct utxo *utxo, const u8 *key);
HTABLE_DEFINE_TYPE(struct utxo, keyof_utxo, hash_sha, utxohash_eq, utxo_map);

bool is_unspendable(const struct bitcoin_transaction_output *o);
u8 *output_types(struct utxo *utxo);

void add_utxo(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct bitcoin_transaction *t,
	      u32 txnum, off_t off);
  
void release_utxo(struct utxo_map *utxo_map, const struct bitcoin_transaction_input *i);

#endif /* BITCOIN_ITERATE_UTXO_H */
