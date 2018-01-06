/*******************************************************************************
 *
 *  = types.h
 *
 *  Defines blockchain data structures.
 *
 */
#ifndef BITCOIN_PARSE_TYPES_H
#define BITCOIN_PARSE_TYPES_H
#include <ccan/tal/tal.h>
#include <ccan/short_types/short_types.h>
#include <openssl/sha.h>
#include <sys/types.h>

/* We unpack varints for our in-memory representation */
#define varint_t u64

/**
 * block_header - Block header
 * 
 * @D9B4BEF9: Netmarker
 * @len: Length of block in bytes
 * @version: Bitcoin protocol version
 * @prev_hash: Hash of previous (parent) block
 * @merkle_hash: Hash of Merkle root for block's chain
 * @timestamp: UNIX timestamp of block
 * @target: Difficulty for block
 * @nonce: Which solved block
 * @transaction_count: Number of transactions in block
 *
 * Block headers are stored little-endian on disk.
 *
 */
struct block_header {
	u32 D9B4BEF9;
	u32 len;
	u32 version;
	u8 prev_hash[32];
	u8 merkle_hash[32];
	u32 timestamp;
	u32 target;
	u32 nonce;
	varint_t transaction_count;
};

/**
 * block - A single bitcoin block
 *
 * @id: The block ID (hash as an integer)
 * @height: Height in the blockchain (-1 for not-yet-known)
 * @filenum: Which block file index contained this block
 * @pos: The position within this block's file of its first transaction
 * @next: pointer to the next block struct in the blockchain
 * @bh: this block's header
 *
 */
struct block {
	u8 id[SHA256_DIGEST_LENGTH];
	s32 height;
	unsigned int filenum;
	off_t pos;
	struct block *next;
	struct block_header bh;
};

/**
 * transaction - A single bitcoin transaction
 * 
 * @version: Bitcoin protocol version
 * @input_count: Number of inputs
 * @input: Array of inputs
 * @output_count: Number of outputs
 * @output: Array of outputs
 * @lock_time: Lock time for this transaction
 * @segwit: Is the transaction segwit
 * @txid: Hash for this transaction
 * @wtxid: Segwit hash for this transaction
 * @total_len: Total length of this transaction (bytes)
 * @non_swlen: Total length of this transaction without segwit data (bytes)
 *
 */
struct transaction {
  u32 version;
  varint_t input_count;
  struct input *input;
  varint_t output_count;
  struct output *output;
  u32 lock_time;
  u8 segwit;

  u8 txid[SHA256_DIGEST_LENGTH];
  u8 wtxid[SHA256_DIGEST_LENGTH];
  u32 total_len;
  u32 non_swlen;
};

/**
 * output - A single output from a particular bitcoin transaction
 * 
 * @amount: The number of Satoshis for this output
 * @script_length: The length of this output's script
 * @script: This output's script
 *
 */
struct output {
	u64 amount;
	varint_t script_length;
	u8 *script;
};

/**
 * input - A single input from a particular bitcoin transaction
 * 
 * @txid: The hash of the transaction containing the output this input is spending
 * @index: The index of the output in the transaction this input is spending
 * @script_length: The length of this input's script
 * @script: This input's script
 * @sequence_number: This input's sequence number (not currently used)
 *
 */
struct input {
	u8 txid[SHA256_DIGEST_LENGTH];
	u32 index; /* output number referred to by above */
	varint_t script_length;
	u8 *script;
	varint_t num_witness;
	u8 **witness;
	u32 sequence_number;
};
 
/**
 * utxo -- A group of outputs created by a single transaction.
 *
 * @txid: hash of the transaction which created these UTXOs
 * @timestamp: UNIX timestamp of the transaction
 * @height: the block height in which the transaction was mined
 * @txnum: the index of the transaction within the block
 * @num_outputs: the number of outputs
 * @unspent_outputs: the number of unspent outputs (UTXOs)
 * @unspent: the total amount of Satoshis across all unspent outputs
 * @spent: the total amount of Satoshis across all spent outputs
 * @amount: array of output amounts
 * @output_types: array of guesses at output types
 * 
 */
struct utxo {
	u8 txid[SHA256_DIGEST_LENGTH];
	u32 timestamp;
	unsigned int height;
	unsigned int txnum;
	u32 num_outputs;
	u32 unspent_outputs;
        u64 unspent;
        u64 spent;
	u64 amount[];
	/* Followed by a char per output for UNKNOWN/PAYMENT/CHANGE */
};

#define OP_PUSHDATA1	0x4C
#define OP_PUSHDATA2	0x4D
#define OP_PUSHDATA4	0x4E
#define OP_NOP		0x61
#define OP_RETURN	0x6A
#define OP_DUP		0x76
#define OP_EQUALVERIFY	0x88
#define OP_CHECKSIG	0xAC
#define OP_HASH160	0xA9

#define UNKNOWN_OUTPUT 0
#define PAYMENT_OUTPUT 1
#define CHANGE_OUTPUT  2

#endif /* BITCOIN_PARSE_TYPES_H */
