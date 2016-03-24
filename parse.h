/*******************************************************************************
 *
 *  = parse.h
 *
 *  Defines functions for parsing block headers & transactions from files
 *  on disk.
 *
 */

#ifndef BITCOIN_PARSE_PARSE_H
#define BITCOIN_PARSE_PARSE_H
#include "types.h"
#include "io.h"
#include "space.h"

/**
 * next_block_header_prefix - Fast-forward *off to head of next block,
 * or false if not found.
 *
 * @f: current file
 * @off: current file offset
 * @marker: marker for block header
 *
 * This is the first step in iterating over the blockchain as it
 * allows scanning to the beginning of the block you want to iterate
 * from.
 */
bool next_block_header_prefix(struct file *f, off_t *off, const u32 marker);

/**
 * read_bitcoin_block_header - Reads the block header from disk into
 * *block.
 *
 * @block: block header data structure to populate
 * @f: current file
 * @off: current file offset
 * @block_md: hash for the block
 * @marker: marker for block header
 *
 *  This is the second step in iterating over the blockchain and it
 *  occurs again for each new block iterated over.
 */
bool read_bitcoin_block_header(struct bitcoin_block *block,
				 struct file *f, off_t *off,
				 u8 block_md[SHA256_DIGEST_LENGTH],
				 const u32 marker);

/**
 * skip_bitcoin_transactions - Fast-forward *off through past block
 * *b's transactions.
 *
 * @b: block
 * @block_start: block offset
 * @off: current file offset
 * 
 *  This is the third step in iterating over the blockchain as it
 *  allows skipping all the transactions in a block and (therefore)
 *  jumping to the next block.
 */
void skip_bitcoin_transactions(const struct bitcoin_block *b,
				 off_t block_start, off_t *off);

/**
 * read_bitcoin_transaction - Reads a transaction from disk into *t.
 *
 * @space: space used for allocation
 * @t: transaction data structure to populate
 * @f: current file
 * @off: current file offset
 * 
 *  This is the fourth step in iterating over the blockchain as, once
 *  the first block has been reached, this allows iterating over all
 *  subsequent transactions.
 */
void read_bitcoin_transaction(struct space *space,
				struct bitcoin_transaction *t,
				struct file *f, off_t *off);
#endif /* BITCOIN_PARSE_PARSE_H */
