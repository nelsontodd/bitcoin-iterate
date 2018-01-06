#ifndef BITCOIN_ITERATE_UTXO_H
#define BITCOIN_ITERATE_UTXO_H
#include <sys/types.h>
#include <ccan/tal/tal.h>
#include <ccan/htable/htable_type.h>
#include <ccan/short_types/short_types.h>
#include "types.h"
#include "utils.h"

/**
 * Return the SHA256 hash of the transaction for the given transaction
 * output group ("UTXO").
 *
 * This function is used to match UTXO structs to keys in the utxo_map.
 *
 *  @param utxo -- pointer to a UTXO struct
 */
const u8 *keyof_utxo(const struct utxo *utxo);

/**
 * Does the given UTXO match the given key/hash?
 *
 *  @param utxo -- pointer to a UTXO struct
 *  @param key  -- pointer to a SHA256 block hash
 */
bool utxohash_eq(const struct utxo *utxo, const u8 *key);

/**
 * Defines the utxo_map and the following methods:
 *
 *   utxo_map_init
 *   utxo_map_clear
 *   utxo_map_add
 *   utxo_map_del
 *   utxo_map_get
 *   utxo_map_first
 *   utxo_map_next
 *
 * See ccan/htable/htable_type.h for more details.
 * 
 */
HTABLE_DEFINE_TYPE(struct utxo, keyof_utxo, hash_sha, utxohash_eq, utxo_map);


void add_utxo(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct transaction *t,
	      u32 txnum, off_t off);
  
void release_utxo(struct utxo_map *utxo_map, const struct input *i);

bool is_unspendable(const struct output *o);

u8 *output_types(struct utxo *utxo);

#endif /* BITCOIN_ITERATE_UTXO_H */
