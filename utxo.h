#ifndef BITCOIN_ITERATE_UTXO_H
#define BITCOIN_ITERATE_UTXO_H
#include <sys/types.h>
#include <ccan/tal/tal.h>
#include <ccan/htable/htable_type.h>
#include <ccan/short_types/short_types.h>
#include "types.h"
#include "utils.h"

/**
 * This function is used to map UTXO structs to keys in the utxo_map.
 *
 * @param utxo -- pointer to a UTXO struct 
 * @return pointer to the UTXO struct's `txid` member
 * 
 */
const u8 *keyof_utxo(const struct utxo *utxo);

/**
 * This function is used to map UTXO structs keys to hash values
 * utxo_map.
 *
 * Reads the contents of the transaction TXID as well as the output
 * index (which are laid out sequentially in the `utxo` struct).
 *
 *  @param key -- pointer to a UTXO's key (its `txid` member)
 */
size_t hashof_utxo(const u8 *key);

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

/**
 * Defines UTXO structs from the given transaction & block, then adds
 * them to the UTXO map utxo_map.
 *
 *  @param tal_ctx  -- tal
 *  @param utxo_map -- pointer to the utxo map that this utxo will be added to.
 *  @param b        -- pointer to the block containing creating UTXOs
 *  @param t        -- pointer to the transaction creating these UTXOs
 *  @param txnum    -- index of the transaction in the containing block.
 *  
 */
void add_utxos(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct transaction *t,
	      u32 txnum);
  
/**
 * Removes UTXO from UTXO map, indiciating it was spent by the given
 * input.
 *
 *  @param utxo_map -- pointer to the utxo map
 *  @param i        -- pointer to the input
 *  
 */
void release_utxo(struct utxo_map *utxo_map, const struct input *i);

/**
 * Returns true if an output is unspendable.
 *
 *  @param o -- pointer to output o
 *
 */
bool is_unspendable(const struct output *o);

#endif /* BITCOIN_ITERATE_UTXO_H */
