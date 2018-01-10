#ifndef BITCOIN_ITERATE_H
#define BITCOIN_ITERATE_H
#include <stdbool.h>
#include <ccan/short_types/short_types.h>
#include "types.h"
#include "utxo.h"

/**
 *block_function - interpolate block data into user-provided format string.
 * 
 * @utxo_map: pointer to a string specifying the directory containing block data.
 * @b: pointer to a string specifying the directory that utxo maps will be written.
 *		
 */
typedef void (*block_function)(const struct utxo_map *utxo_map, struct block *b);

/**
 *transaction_function - interpolate transaction data into user-provided format string.
 * 
 * @utxo_map: global UTXO map
 * @b: current block
 * @t: current transaction
 * @txnum: current transaction number
 *		
 */
typedef void (*transaction_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum);

/**
 *input_function - interpolate input data into user-provided format string.
 * 
 * @utxo_map: global UTXO map
 * @b: current block
 * @t: current transaction
 * @txnum: current transaction number
 * @i: current input
 */
typedef void (*input_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum, struct input *i);

/**
 *output_function - interpolate output data into user-provided format string.
 * 
 * @utxo_map: global UTXO map
 * @b: current block
 * @t: current transaction
 * @txnum: current transaction number
 * @o: current output
 */
typedef void (*output_function)(const struct utxo_map *utxo_map, struct block *b, struct transaction *t, size_t txnum, struct output *o);

/**
 *utxo_function - interpolate utxo data into user-provided format string.
 *		
 * @utxo_map: global UTXO map
 * @current_block: current block
 * @last_utxo_block: previous block
 * @u: current utxo
 */
typedef void (*utxo_function)(const struct utxo_map *utxo_map, struct block *current_block, struct block *last_utxo_block, struct utxo *u);

/**
 * iterate - Entry point and main function for bitcoin iterate. Parses bitcoin blocks
 * one by one and outputs the processed data required by the formatting functions. 
 * Optionally, iterate will build a blockcache and utxocache. In order for the blockc
 * ache to be assembled, there must be a --cache specified and no --end specified.
 * Iteration begins either at genesis or the input start block, and iteration ends
 * at either the end of the chain or at the input end block.
 *
 * 
 * @blockdir: pointer to a string specifying the directory containing block data.
 * @cachedir: pointer to a string specifying the directory that utxo maps will be written in.
 * @use_testnet: whether or not to use testnet
 * @block_start: starting block height
 * @block_end: ending block height
 * @start_hash: starting block hash
 * @tip: ending block hash
 * @needs_utxo: whether or not iterate needs to calculate UTXO data
 * @utxo_period: number of blocks in between successive UTXO function calls
 * @use_mmap: use mmap
 * @progress_marks: interval at which to print '.' to stderr, default is None
 * @quiet: whether or not to silence output
 * @blockfn: function used to process/print block struct data - specified by --block format strings
 * @txfn: function used to process/print transaction struct data - specified by --transaction format string
 * @inputfn: function used to process/print input struct data - specified by --input  format string
 * @outputfn: function used to process/print output struct data - specified by --output format string
 * @utxofn: function used to process/print utxo struct data - specified by --utxo format string
 *		
 */
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
