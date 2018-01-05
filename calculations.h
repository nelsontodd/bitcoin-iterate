#ifndef BITCOIN_ITERATE_CALCULATIONS_H
#define BITCOIN_ITERATE_CALCULATIONS_H
#include "types.h"
#include "utxo.h"

/* calculate_fees - calculate fees for a transaction. 
 * @param utxo_map    -- pointer to utxo_map (linked list of utxo tx hashes)
 * @param t           -- pointer to transaction struct
 * @param is_coinbase -- if true, transaction is coinbase
 */

s64 calculate_fees(const struct utxo_map *utxo_map,
		   const struct transaction *t,
		   bool is_coinbase);
  
/* calculate_bdc - Calculate block days created for utxo
 * @param utxo_map    -- pointer to utxo_map (linked list of utxo tx hashes)
 * @param t           -- pointer to transaction struct
 * @param is_coinbase -- if true, transaction is coinbase
 */

s64 calculate_bdc(const struct utxo *u, struct block *current_block, struct block *last_utxo_block);

/* calculate_bdd - Calculate block days destroyed for a given utxo
 * @param utxo_map    -- pointer to utxo_map (linked list of utxo tx hashes)
 * @param t           -- pointer to transaction struct
 * @param is_coinbase -- if true, transaction is coinbase
 * @param timestamp   -- timestamp of the transaction
 */

s64 calculate_bdd(const struct utxo_map *utxo_map,
		  const struct transaction *t,
		  bool is_coinbase, u32 timestamp);

/* to_btc - satoshi -> btc 
 * @param satoshis -- convert to satoshi
 */

double to_btc(s64 satoshis);
  
#endif /* BITCOIN_ITERATE_CALCULATIONS_H */
