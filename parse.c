#include <ccan/likely/likely.h>
#include <ccan/endian/endian.h>
#include <ccan/tal/tal.h>
#include <ccan/err/err.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "types.h"
#include "parse.h"
#include "space.h"

//
// == Parsing Data Types == 
// 

static u64 pull_varint(struct file *f, off_t *poff)
{
	u64 ret;
	u8 v[9], *p;

	if (likely(f->mmap))
		p = f->mmap + *poff;
	else {
		/* We could do a short read here, that's OK. */
		if (pread(f->fd, v, sizeof(v), *poff) < 1)
			err(1, "Pulling varint from %s offset %llu\n",
			    f->name, (long long)*poff);
		p = v;
	}

	if (*p < 0xfd) {
		ret = p[0];
		*poff += 1;
	} else if (*p == 0xfd) {
		ret = ((u64)p[2] << 8) + p[1];
		*poff += 3;
	} else if (*p == 0xfe) {
		ret = ((u64)p[4] << 24) + ((u64)p[3] << 16)
			+ ((u64)p[2] << 8) + p[1];
		*poff += 5;
	} else {
		ret = ((u64)p[8] << 56) + ((u64)p[7] << 48)
			+ ((u64)p[6] << 40) + ((u64)p[5] << 32)
			+ ((u64)p[4] << 24) + ((u64)p[3] << 16)
			+ ((u64)p[2] << 8) + p[1];
		*poff += 9;
	}
	return ret;
}

static void pull_bytes(struct file *f, off_t *poff, void *dst, size_t num)
{
	if (likely(f->mmap))
		memcpy(dst, f->mmap + *poff, num);
	else
		file_read(f, *poff, num, dst);
	*poff += num;
}

static u32 pull_u32(struct file *f, off_t *poff)
{
	le32 ret;

	pull_bytes(f, poff, &ret, sizeof(ret));
	return le32_to_cpu(ret);
}

static u64 pull_u64(struct file *f, off_t *poff)
{
	le64 ret;

	pull_bytes(f, poff, &ret, sizeof(ret));
	return le64_to_cpu(ret);
}

static void pull_hash(struct file *f, off_t *poff, u8 dst[32])
{
	pull_bytes(f, poff, dst, 32);
}

//
// == Parsing Inputs & Outputs == 
// 

static void read_input(struct transaction *t, struct space *space, struct file *f, off_t *poff,
		       struct input *input)
{
	pull_hash(f, poff, input->txid);
	input->index = pull_u32(f, poff);
	input->script_length = pull_varint(f, poff);
	input->script = space_alloc(space, input->script_length);
	input->witness = NULL;
	pull_bytes(f, poff, input->script, input->script_length);
	input->sequence_number = pull_u32(f, poff);
}

static void read_output(struct transaction *t, struct space *space, struct file *f, off_t *poff,
			struct output *output)
{
	output->amount = pull_u64(f, poff);
	output->script_length = pull_varint(f, poff);
	output->script = space_alloc(space, output->script_length);
	pull_bytes(f, poff, output->script, output->script_length);
}

//
// == Parsing a transaction witness == 
// 

// FIXME -- this function should attempt to store and/or interpret the
// witness data
static void read_witness_stack_item(u8 *witness, size_t i, size_t s, struct file *f, off_t *poff,struct space *space)
{
  varint_t num_bytes = pull_varint(f, poff);
  witness = space_alloc(space, num_bytes);
  pull_bytes(f, poff, witness, num_bytes);
}

static void read_witness_field(struct input *inp, size_t i, struct file *f, off_t *poff,struct space *space)
{
  size_t s;
  inp->num_witness = pull_varint(f, poff);
  if (inp->num_witness == 0) { return; }
  inp->witness = space_alloc_arr(space, u8 *,
				   inp->num_witness);
  for (s=0; s<inp->num_witness;s++) {
    read_witness_stack_item(inp->witness[s], i, s, f, poff, space);
  }
}

static void read_witness(struct transaction *trans, struct file *f, off_t *poff,struct space *space)
{
  size_t i;
  for (i=0; i<trans->input_count;i++) {
    read_witness_field(&trans->input[i], i, f, poff, space);
  }
}

//
// == Parsing a transaction ==
// 

/**
 *  Appends contents of file @f between offsets @context_off and @poff
 *  to the given hash @context.
 *
 *  Sets @context_off to the current position of @poff.
 *
 */
void append_to_hash_context(SHA256_CTX *context, off_t *context_off, struct file *f, off_t *poff)
{
	  if (likely(f->mmap)) {
	    SHA256_Update(context, f->mmap + *context_off, *poff - *context_off);
	  } else {
	    u8 *buf = tal_arr(NULL, u8, *poff - *context_off);
	    file_read(f, *context_off, *poff - *context_off, buf);
	    SHA256_Update(context, buf, *poff - *context_off);
	    tal_free(buf);
	  }
	  *context_off = *poff;
}

/**
 *  Parses a serialized transaction from the given @file into the
 *  given @trans object.
 *
 *  The segregated witness soft-fork of 2017-08-21 changed the
 *  serialization format for transactions on disk.  The "original
 *  serialization" was the concatenation of
 *
 *    version | transaction inputs | transaction outputs | lock time
 *
 *  The "segwit serialization" is the concatenation of
 *
 *    version | marker | flag | transaction inputs | transaction outputs | witness | lock time
 *
 *  In the above, "transaction inputs" and "transaction outputs" are
 *  both array-like data, with the first part of the data being a
 *  varint which declares the length of the corresponding array.
 *
 *  The "marker" above is a single-byte integer equal to 0 and "flag"
 *  is another single-byte integer equal to 1.
 *
 *  The first data after the "version" is therefore always an integer
 *  (varint) of some kind: either the number of transaction inputs or
 *  the marker value of 0.  Since a transaction must have at least 1
 *  input, the presence of 0 in this location can be used to
 *  distinguish a transaction using the "original" serialization from
 *  one using the newer "segwit" serialization.  This allows
 *  processing to branch and correctly parse the rest of the
 *  transaction data.
 *
 *  To make matters more complicated, the TXID of a transaction, *even
 *  a segwit transaction*, is the (double) SHA256 hash of the
 *  transaction in the "original serialization".  Segwit transactions
 *  have another identifier known as a WTXID which is the (double)
 *  SHA256 hash of the transaction in the new, "segwit serialization".
 *
 *  For more details, see:
 *
 *    - https://bitcoincore.org/en/segwit_wallet_dev/#transaction-serialization
 *    - https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
 *
 */
void read_transaction(struct space *space,
		      struct transaction *trans,
		      struct file *f, off_t *poff)
{
	size_t i;

	// Initialize a SHA256 hash context
	SHA256_CTX context;
	SHA256_Init(&context);

	// Track where the transaction started so we can use this
	// position later to calculate the total length on disk.
	off_t start = *poff;

	// Track the position to start adding to the hash context
	// from.  This separate offset is used to skip over segregated
	// witness data so the "original serialization" can be used to
	// calculate the TXID.
	off_t context_off  = *poff;

	// Track the size of the "non-segwit" or "original"
	// serialization.  This separate counter is used to exclude
	// segregated witness data.  The non-segwit serialization
	// length and the actual serialization length are required to
	// calculate the virtual length/weight of the transaction.
	trans->non_swlen = 0;

	//
	// == Now start incrementally processing & hashing transaction fields ==
	// 

	// 1. Version
	trans->version = pull_u32(f, poff);
	trans->non_swlen += (*poff - context_off);
	append_to_hash_context(&context, &context_off, f, poff);

	// 2. One of
	//
	//      input_count                        [non-segwit serialization]
	//      (marker|flag|input_count)          [segwit serialization]
	//      
	trans->input_count = pull_varint(f, poff);
	if (trans->input_count == 0) {
	  // Mark the transaction as segwit.
	  trans->segwit = 1;
	  // Validate flag value.
	  if (pull_varint(f, poff) !=1 ) {
	    errx(1, "Unexpected flag value found while parsing segwit transaction\n");
	  };
	  // Update the hash context to just before the transaction
	  // input count, as it would have been for a transaction in
	  // the non-segwit serialization.
	  context_off = *poff;
	  // And now pull the transaction input count itself, leaving
	  // the file pointer poff in the same (relative) position it
	  // would have had at this point for a transaction in the
	  // non-segwit serialization -- just before the array of
	  // transaction inputs.
	  trans->input_count = pull_varint(f, poff);
	} else {
	  // Mark the transaction as *not* segwit.
	  trans->segwit = 0;
	}

	// 3. Inputs
	trans->input = space_alloc_arr(space, struct input, trans->input_count);
	for (i = 0; i < trans->input_count; i++) {
	  read_input(trans, space, f, poff, trans->input + i);
	}

	// 4. Output count & outputs
	trans->output_count = pull_varint(f, poff);
	trans->output = space_alloc_arr(space, struct output, trans->output_count);
	for (i = 0; i < trans->output_count; i++) {
	  read_output(trans, space, f, poff, trans->output + i);
	}
	trans->non_swlen += (*poff - context_off);
	append_to_hash_context(&context, &context_off, f, poff);
	
	// 5. witness [only segwit serialization]
	if (trans->segwit == 1) {
	  read_witness(trans, f, poff, space);
	  // Update the hash context to just before the lock time, as
	  // it would have been for a transaction in the non-segwit
	  // serialization.
	  context_off = *poff;
	}

	// 6. Lock time
	trans->lock_time = pull_u32(f, poff);
	trans->non_swlen += (*poff - context_off);
	append_to_hash_context(&context, &context_off, f, poff);

	// == Now calculate properties which depend upon serialization ==

	// Length -- the total length is just the number of bytes of
	// the serialization, regardless of whether it was non-segwit
	// or segwit.
	trans->total_len = *poff - start;

	// TXID
	// 
	// Store single hash
	SHA256_Final(trans->txid, &context);
	// Now create double hash
	SHA256_Init(&context);
	SHA256_Update(&context, trans->txid, sizeof(trans->txid));
	SHA256_Final(trans->txid, &context);
	
	// WTXID
	//
	if (trans->segwit == 1) {
	  // Re-initialize the context and set the context offset to the
	  // start of the transaction so we can capture the full
	  // serialization.
	  SHA256_Init(&context);
	  context_off = start;
	  // This function will needlessly and incorrectly modify
	  // context_off but we're done with it at this point, so
	  // that's actually OK...
	  append_to_hash_context(&context, &context_off, f, poff);
	  // Store single hash
	  SHA256_Final(trans->wtxid, &context);
	  // Now create double hash
	  SHA256_Init(&context);
	  SHA256_Update(&context, trans->wtxid, sizeof(trans->wtxid));
	  SHA256_Final(trans->wtxid, &context);
	} else {
          memcpy(trans->wtxid, trans->txid, sizeof(trans->txid));
	}
}

/* Inefficient, but blk*.dat can have zero(?) padding. */
bool next_block_header_prefix(struct file *f, off_t *off, const u32 marker)
{
	while (*off + sizeof(u32) <= f->len) {
		u32 val;

		/* Inefficent, but don't expect it to be far. */
		val = pull_u32(f, off);
		*off -= 4;

		if (val == marker)
			return true;
		(*off)++;
	}
	return false;
}

bool read_block_header(struct block_header *bh,
			  struct file *f, off_t *off,
			  u8 block_md[SHA256_DIGEST_LENGTH],
			  const u32 marker)
{
	SHA256_CTX sha256;
	off_t start;

	bh->D9B4BEF9 = pull_u32(f, off);
	assert(bh->D9B4BEF9 == marker);
	bh->len = pull_u32(f, off);

	/* Hash only covers version to nonce, inclusive. */
	start = *off;
	bh->version = pull_u32(f, off);
	pull_hash(f, off, bh->prev_hash);
	pull_hash(f, off, bh->merkle_hash);
	bh->timestamp = pull_u32(f, off);
	bh->target = pull_u32(f, off);
	bh->nonce = pull_u32(f, off);

	/* Bitcoin uses double sha (it's not quite known why...) */
	SHA256_Init(&sha256);
	if (likely(f->mmap)) {
		SHA256_Update(&sha256, f->mmap + start, *off - start);
	} else {
		u8 *buf = tal_arr(NULL, u8, *off - start);
		file_read(f, start, *off - start, buf);
		SHA256_Update(&sha256, buf, *off - start);
		tal_free(buf);
	}
	SHA256_Final(block_md, &sha256);

	SHA256_Init(&sha256);
	SHA256_Update(&sha256, block_md, SHA256_DIGEST_LENGTH);
	SHA256_Final(block_md, &sha256);

	bh->transaction_count = pull_varint(f, off);

	return bh;
}

void skip_transactions(const struct block_header *bh,
			       off_t block_start,
			       off_t *off)
{
	*off = block_start + 8 + bh->len;
}
