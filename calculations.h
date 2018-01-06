#ifndef BITCOIN_ITERATE_CALCULATIONS_H
#define BITCOIN_ITERATE_CALCULATIONS_H
#include "types.h"
#include "utxo.h"

/**
 * calculate_fees - Calculate fees for a transaction.
 * 
 * @param utxo_map    -- pointer to utxo_map
 * @param t           -- pointer to transaction struct
 * @param is_coinbase -- whether transaction is coinbase
 * 
 * @return transaction fees (in Satoshis)
 */
s64 calculate_fees(const struct utxo_map *utxo_map,
		   const struct transaction *t,
		   bool is_coinbase);
  
/**
 * calculate_bdc - Calculate Bitcoin days created by a group of
 * transaction outputs ("UTXO").
 *
 * The age of the outputs will be the difference between the current
 * and last timestamps.
 * 
 * @param utxo              -- pointer to utxo struct
 * @param current_timestamp -- current UNIX timestamp
 * @param last_timestamp    -- prior UNIX timestamp
 */
s64 calculate_bdc(const struct utxo *u, 
		  const u32 current_timestamp, 
		  const u32 last_timestamp);

/**
 * calculate_bdd - Calculate Bitcoin days destroyed by a given transaction.
 * 
 * @param utxo_map    -- pointer to utxo_map
 * @param t           -- pointer to transaction struct
 * @param is_coinbase -- whether transaction is coinbase
 * @param timestamp   -- timestamp of the transaction
 *
 * @return Satoshi-days destroyed
 */

s64 calculate_bdd(const struct utxo_map *utxo_map,
		  const struct transaction *t,
		  bool is_coinbase, u32 timestamp);

/**
 * to_btc - Convert from Satoshi -> BTC
 * 
 * @param satoshis -- amount in Satoshis
 */

double to_btc(s64 satoshis);

/**
 * segwit_length - Calculate the length of a transaction in vbytes.
 *
 * @param t -- pointer to a transaction struct
 *
 */
u32 segwit_length(const struct transaction *t);

/**
 * segwit_weight - Calculate the weight of a transaction in sipa.
 *
 * @param t -- pointer to a transaction struct
 *
 */
u32 segwit_weight(const struct transaction *t);
  
#endif /* BITCOIN_ITERATE_CALCULATIONS_H */
