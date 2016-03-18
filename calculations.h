#ifndef BITCOIN_ITERATE_CALCULATIONS_H
#define BITCOIN_ITERATE_CALCULATIONS_H
#include "types.h"
#include "utxo.h"

s64 calculate_fees(const struct utxo_map *utxo_map,
		   const struct bitcoin_transaction *t,
		   bool is_coinbase);
  
s64 calculate_bdc(const struct utxo *u, u32 timestamp);

s64 calculate_bdd(const struct utxo_map *utxo_map,
		  const struct bitcoin_transaction *t,
		  bool is_coinbase, u32 timestamp);
  
#endif /* BITCOIN_ITERATE_CALCULATIONS_H */
