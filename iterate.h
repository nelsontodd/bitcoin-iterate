#ifndef BITCOIN_ITERATE_H
#define BITCOIN_ITERATE_H
#include <stdbool.h>
#include <ccan/short_types/short_types.h>
#include "types.h"

typedef void (*block_function)(struct block b, struct bitcoin_block bh);
typedef void (*transaction_function)(struct block b, struct bitcoin_transaction t);
typedef void (*input_function)(struct block b, struct bitcoin_transaction t, struct bitcoin_transaction_input i);
typedef void (*output_function)(struct block b, struct bitcoin_transaction t, struct bitcoin_transaction_output o);
typedef void (*utxo_function)(struct block b, struct utxo u);

void iterate(char *blockdir, char *cachedir,
	     bool use_testnet,
	     unsigned long block_start, unsigned long block_end,
	     u8 *start_hash, u8 *tip,
	     unsigned int utxo_period, bool use_mmap,
	     unsigned progress_marks, bool quiet,
	     char *blockfmt, char *txfmt, char *inputfmt, char *outputfmt, char *utxofmt,
	     block_function *blockfn,
	     transaction_function *txfn,
	     input_function *inputfn,
	     output_function *outputfn,
	     utxo_function *utxofn);
  
#endif /* BITCOIN_ITERATE_H */
