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
 * transaction - A single bitcoin transaction
 * @version: Bitcoin protocol version
 * @input_count: Number of inputs
 * @input: Array of inputs
 * @output_count: Number of outputs
 * @output: Array of outputs
 * @lock_time: Lock time for this transaction
 * @segwit: Is the transaction segwit or not
 * @txid: Hash for this transaction
 * @len: Length of this transaction in bytes
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

  /* We calculate these as we read in transaction: */
  u8 txid[SHA256_DIGEST_LENGTH];
  u8 wtxid[SHA256_DIGEST_LENGTH];
  u32 len;
  u32 vlen;
};

/**
 * output - A single output from a particular bitcoin transaction
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
 * @hash: The hash of the transaction containing the output this input is spending
 * @index: The index of the output in the transaction this input is spending
 * @script_length: The length of this input's script
 * @script: This input's script
 * @sequence_number: This input's sequence number (not currently used)
 *
 */
struct input {
	u8 hash[SHA256_DIGEST_LENGTH];
	u32 index; /* output number referred to by above */
	varint_t script_length;
	u8 *script;
	u32 sequence_number;
};

struct block {
	u8 sha[SHA256_DIGEST_LENGTH];
	s32 height; /* -1 for not-yet-known */
	/* Where is it */
	unsigned int filenum;
	/* Position of first transaction */
	off_t pos;
	/* So we can iterate forwards. */
	struct block *next;
	/* Bitcoin block header. */
	struct block_header bh;
};
 
struct utxo {
	/* txid */
	u8 tx[SHA256_DIGEST_LENGTH];

	/* Timestamp. */
	u32 timestamp;

	/* Height. */
	unsigned int height;

	/* txindex within block. */
	unsigned int txnum;

	/* Number of outputs. */
	u32 num_outputs;

	/* Reference count for this tx. */
	u32 unspent_outputs;

        /* Total amount unspent. */
        u64 unspent;
  
        /* Total amount spent. */
        u64 spent;

	/* Amount for each output. */
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

struct block *new_block();
struct block_header *new_block_header();
void free_block(const tal_t *block);
void free_block_header(const tal_t *block_header);

#endif /* BITCOIN_PARSE_TYPES_H */
