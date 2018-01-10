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

/**
 * Defines a UTXO struct from the given transaction & block, then adds it to 
 * the UTXO map utxo_map.
 *
 *  @param tal_ctx  -- tal
 *  @param utxo_map -- pointer to the utxo map that this utxo will be added to.
 *  @param b        -- pointer to the block containing this UTXO group.
 *  @param t        -- pointer to the transaction containing this UTXO group.
 *  @param txnum    -- index of the transaction in the containing block.
 *  @param off    -- offset
 *
 *
 */
void add_utxo(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct transaction *t,
	      u32 txnum, off_t off);
  
/**
 * Removes UTXO from UTXO map if it no longer has unspent_outputs, or else it
 * decrements the number of unspent_outputs. 
 *
 * For context, this function is called inside of iterate in order to 'release' (delete)
 * UTXO struct composed of tx inputs (which are the outputs added to block_map in 
 * the previous iteration) and replace them with an 
 * updated UTXO struct consisting of tx outputs.
 *
 *  @param utxo_map -- pointer to the utxo map that this utxo will be added to.
 *  @param tal_ctx  -- current input
 *
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

u8 *output_types(struct utxo *utxo);

#endif /* BITCOIN_ITERATE_UTXO_H */
