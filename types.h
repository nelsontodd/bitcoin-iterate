#ifndef BITCOIN_PARSE_TYPES_H
#define BITCOIN_PARSE_TYPES_H
#include <ccan/short_types/short_types.h>
#include <openssl/sha.h>

/* We unpack varints for our in-memory representation */
#define varint_t u64

/* These are little endian on disk. */
struct bitcoin_block {
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

struct bitcoin_transaction {
	u32 version;
	varint_t input_count;
	struct bitcoin_transaction_input *input;
	varint_t output_count;
	struct bitcoin_transaction_output *output;
	u32 lock_time;

	/* We calculate these as we read in transaction: */
	u8 sha256[SHA256_DIGEST_LENGTH];
	u32 len;
};

struct bitcoin_transaction_output {
	u64 amount;
	varint_t script_length;
	u8 *script;
};

struct bitcoin_transaction_input {
	u8 hash[32];
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
	struct bitcoin_block bh;
};

struct utxo {
	/* txid */
	u8 tx[SHA256_DIGEST_LENGTH];

	/* Timestamp. */
	u32 timestamp;

	/* Height. */
	unsigned int height;

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

#endif /* BITCOIN_PARSE_TYPES_H */
