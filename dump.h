#ifndef BITCOIN_ITERATE_DUMP_H
#define BITCOIN_ITERATE_DUMP_H
#include "types.h"
#include "io.h"
#include "utxo.h"

void print_hex(const void *data, size_t len);
void print_hash(const u8 *hash);

void dump_block_header(const struct bitcoin_block *b);
void dump_tx(const struct bitcoin_transaction *tx);
void dump_tx_input(const struct bitcoin_transaction_input *input);
void dump_tx_output(const struct bitcoin_transaction_output *output);

void print_format(const char *format,
		  const struct utxo_map *utxo_map,
		  struct block *b,
		  struct bitcoin_transaction *t,
		  size_t txnum,
		  struct bitcoin_transaction_input *i,
		  struct bitcoin_transaction_output *o,
		  struct utxo *u);

#endif /* BITCOIN_ITERATE_DUMP_H */
