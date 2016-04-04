#ifndef BITCOIN_ITERATE_H
#define BITCOIN_ITERATE_H
#include <stdbool.h>
#include <ccan/short_types/short_types.h>
#include "types.h"
#include "utxo.h"

typedef void (*block_function)(const struct utxo_map *utxo_map, struct block *b);
typedef void (*transaction_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum);
typedef void (*input_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum, struct input *i);
typedef void (*output_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum, struct output *o);
typedef void (*utxo_function)(const struct utxo_map *utxo_map, struct block *current_block, struct block *last_utxo_block, struct utxo *u);

void iterate(char *blockdir, char *cachedir,
	     bool use_testnet,
	     unsigned long block_start, unsigned long block_end,
	     u8 *start_hash, u8 *tip,
	     bool needs_utxo, unsigned int utxo_period,
	     bool use_mmap,
	     unsigned progress_marks, bool quiet,
	     block_function blockfn,
	     transaction_function txfn,
	     input_function inputfn,
	     output_function outputfn,
	     utxo_function utxofn);
  
#endif /* BITCOIN_ITERATE_H */
