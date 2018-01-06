/*******************************************************************************
 *
 *  = format.h
 *
 *  Defines a function for interpolating blockchain data into a
 *  user-provided format string.
 *
 */

#ifndef BITCOIN_ITERATE_DUMP_H
#define BITCOIN_ITERATE_DUMP_H
#include "types.h"
#include "utxo.h"

/**
 * print_format - Interpolate blockchain data into user-provided format string.
 * 
 * @format: format string
 * @utxo_map: global UTXO map
 * @b: current block
 * @t: current transaction
 * @txnum: current transaction number
 * @i: current transaction input
 * @o: current transaction output
 * @u: current UTXO
 * @last_utxo_block: last block for which UTXOs were iterated over
 *
 * In typical usage, most arguments will be `NULL` as shown in the
 * following examples:
 *
 * Example:
 *      // Print block b
 *	print_format(blockfmt, &utxo_map, b, NULL, 0, NULL, NULL, NULL);
 *
 * Example:
 * 
 *      // Print transaction t with number txnum in block b
 *	print_format(txfmt, &utxo_map, b, t, txnum, NULL, NULL, NULL);
 *
 * Example:
 *	
 *      // Print transaction input i in transaction t with number txnum in block b
 *	print_format(inputfmt, &utxo_map, b, t, txnum, i, NULL, NULL);
 *
 * Example:
 *	
 *      // Print transaction output o in transaction t with number txnum in block b
 *	print_format(outputfmt, &utxo_map, b, t, txnum, NULL, o, NULL);
 *
 * Example:
 *	
 *      // Print UTXO u in block b
 *	print_format(utxofmt, &utxo_map, b, NULL, 0, NULL, NULL, u);
 *		
 */
void print_format(const char *format,
		  const struct utxo_map *utxo_map,
		  struct block *b,
		  struct transaction *t,
		  size_t txnum,
		  struct input *i,
		  struct output *o,
		  struct utxo *u,
		  struct block *last_utxo_block);

#endif /* BITCOIN_ITERATE_DUMP_H */
