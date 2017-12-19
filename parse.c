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
	pull_hash(f, poff, input->hash);
	input->index = pull_u32(f, poff);
	input->script_length = pull_varint(f, poff);
	input->script = space_alloc(space, input->script_length);
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
static void read_witness_stack_item(struct transaction *trans, size_t i, size_t s, struct file *f, off_t *poff)
{
  varint_t num_bytes = pull_varint(f, poff);
  u8 witness_data[num_bytes];
  pull_bytes(f, poff, &witness_data, num_bytes);
}

static void read_witness_field(struct transaction *trans, size_t i, struct file *f, off_t *poff)
{
  size_t s;
  varint_t stack_count = pull_varint(f, poff);
  if (stack_count == 0) { return; }
  for (s=0; s<stack_count;s++) {
    read_witness_stack_item(trans, i, s, f, poff);
  }
}

static void read_witness(struct transaction *trans, struct file *f, off_t *poff)
{
  size_t i;
  for (i=0; i<trans->input_count;i++) {
    read_witness_field(trans, i, f, poff);
  }
}

//
// == Parsing a transaction ==
// 

/**
 *  Appends contents of file @f between offsets @context_off and @poff
 *  to the given hash @context.
 *
 *  Increments @original_serialization_length by the number of bytes
 *  read and sets @context_off to the current position of @poff.
 *
 */
void append_to_hash_context(SHA256_CTX *context, off_t *context_off, u32 *original_serialization_length, struct file *f, off_t *poff)
{
	  if (likely(f->mmap)) {
	    SHA256_Update(context, f->mmap + *context_off, *poff - *context_off);
	  } else {
	    u8 *buf = tal_arr(NULL, u8, *poff - *context_off);
	    file_read(f, *context_off, *poff - *context_off, buf);
	    SHA256_Update(context, buf, *poff - *context_off);
	    tal_free(buf);
	  }
	  *original_serialization_length += (*poff - *context_off);
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

	// Track where the transaction started so we can use this
	// position later to calculate the total (raw) length on disk.
	off_t start = *poff;

	// Initialize a SHA256 hash context for the "original
	// serialization" above.
	SHA256_CTX context;
	SHA256_Init(&context);

	// Track the position to start adding to the hash context
	// from.  This separate offset is used to skip over segregated
	// witness data so the "original serialization" can be used to
	// calculate the TXID.
	off_t context_off  = *poff;

	// Track the size of the "original serialization".  This
	// separate counter is used to exclude segregated witness data
	// so the "original serialization" length can be tracked.  The
	// original serialization length and the actual serialization
	// length (in the new, "segwit serialization") are required to
	// calculate the virtual size of the transaction.
	u32 original_serialization_length;

	//
	// == Now start incrementally processing & hashing transaction fields ==
	// 

	// 1. Version
	trans->version = pull_u32(f, poff);
	append_to_hash_context(&context, &context_off, &original_serialization_length, f, poff);

	// 2. One of
	//
	//      input_count                        [original serialization]
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
	  // the "original serialization".
	  context_off = *poff;
	  // And now pull the transaction input count itself, leaving
	  // the file pointer poff in the same (relative) position it
	  // would have had at this point for a transaction in the
	  // "original serialization" -- just before the array of
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
	append_to_hash_context(&context, &context_off, &original_serialization_length, f, poff);
	
	// 5. witness [only segwit serialization]
	if (trans->segwit == 1) {
	  read_witness(trans, f, poff);
	  // Update the hash context to just before the lock time, as
	  // it would have been for a transaction in the "original
	  // serialization".
	  context_off = *poff;
	}

	// 6. Lock time
	trans->lock_time = pull_u32(f, poff);
	append_to_hash_context(&context, &context_off, &original_serialization_length, f, poff);

	// == Now calculate properties which depend upon serialization ==

	// Length -- the "raw" length is just the number of bytes of
	// the serialization, regardless of whether it was "original"
	// or "segwit".
	trans->len = *poff - start;

	// Virtual Length -- 
	if (trans->segwit == 1) {
	  // vsize of a transaction equals 3 times the size with
	  // original serialization, plus the size with new
	  // serialization, divide the result by 4 and round up to the
	  // next integer.
	  //
	  // For example, if a transaction is 200 bytes with new
	  // serialization, and becomes 99 bytes with marker, flag,
	  // and witness removed, the vsize is (99 * 3 + 200) / 4 =
	  // 125 with round up.
	  u32 numerator = ((3 * original_serialization_length) + trans->len);
	  if ((numerator % 4) == 0) {
	    trans->vlen = numerator / 4;
	  } else {
	    trans->vlen = (numerator / 4) + 1;
	  }
	} else {
	  trans->vlen = trans->len;
	}

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
	  // context_off and original_serialization_length but we're
	  // done with them at this point, so that's actually OK...
	  append_to_hash_context(&context, &context_off, &original_serialization_length, f, poff);
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
