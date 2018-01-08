#include <ccan/str/hex/hex.h>
#include <ccan/endian/endian.h>
#include <ccan/err/err.h>
#include <stdio.h>
#include <inttypes.h>
#include "format.h"
#include "calculations.h"
#include "utxo.h"

static void print_hash(const u8 *hash)
{
	char str[hex_str_size(SHA256_DIGEST_LENGTH)];

	hex_encode(hash, SHA256_DIGEST_LENGTH, str, sizeof(str));
	fputs(str, stdout);
}

void print_reversed_hash(const u8 *hash)
{
	u8 reversed[32];
	int i; 
	for(i=0;i<32;++i) {
		reversed[i]=hash[32-i-1];
	}
	print_hash(reversed);
}

static void print_hex(const void *data, size_t len)
{
	char str[len * 2 + 1];

	hex_encode(data, len, str, sizeof(str));
	fputs(str, stdout);
}

static void print_varint(varint_t v)
{
	u8 buf[9], *p = buf;

	if (v < 0xfd) {
		*(p++) = v;
	} else if (v <= 0xffff) {
		(*p++) = 0xfd;
		(*p++) = v;
		(*p++) = v >> 8;
	} else if (v <= 0xffffffff) {
		(*p++) = 0xfe;
		(*p++) = v;
		(*p++) = v >> 8;
		(*p++) = v >> 16;
		(*p++) = v >> 24;
	} else {
		(*p++) = 0xff;
		(*p++) = v;
		(*p++) = v >> 8;
		(*p++) = v >> 16;
		(*p++) = v >> 24;
		(*p++) = v >> 32;
		(*p++) = v >> 40;
		(*p++) = v >> 48;
		(*p++) = v >> 56;
	}
	print_hex(buf, p - buf);
}

static void print_le32(u32 v)
{
	le32 l = cpu_to_le32(v);
	print_hex(&l, sizeof(l));
}

static void print_le64(u64 v)
{
	le64 l = cpu_to_le64(v);
	print_hex(&l, sizeof(l));
}

static void dump_tx_input(const struct input *input)
{
	print_hash(input->txid);
	print_le32(input->index);
	print_varint(input->script_length);
	print_hex(input->script, input->script_length);
	print_le32(input->sequence_number);
}

static void dump_tx_output(const struct output *output)
{
	print_le64(output->amount);
	print_varint(output->script_length);
	print_hex(output->script, output->script_length);
}

static void dump_tx(const struct transaction *tx)
{
	varint_t i;

	print_le32(tx->version);
	print_varint(tx->input_count);
	for (i = 0; i < tx->input_count; i++)
		dump_tx_input(&tx->input[i]);
	print_varint(tx->output_count);
	for (i = 0; i < tx->output_count; i++)
		dump_tx_output(&tx->output[i]);
	print_le32(tx->lock_time);
}

static void dump_block_header(const struct block_header *bh)
{
	print_le32(bh->version);
	print_hash(bh->prev_hash);
	print_hash(bh->merkle_hash);
	print_le32(bh->timestamp);
	print_le32(bh->target);
	print_le32(bh->nonce);
}

/* FIXME: Speed up! */
void print_format(const char *format,
		  const struct utxo_map *utxo_map,
		  struct block *b,
		  struct transaction *t,
		  size_t txnum,
		  struct input *i,
		  struct output *o,
		  struct utxo *u,
		  struct block *last_utxo_block)
{
  const char *c;
  for (c = format; *c; c++) {
    if (*c != '%') {
      fputc(*c, stdout);
      continue;
    }

    switch (c[1]) {
    case 'b':
      switch (c[2]) {
      case 'l':
	printf("%u", b->bh.len);
	break;
      case 'v':
	printf("%u", b->bh.version);
	break;
      case 'p':
	print_reversed_hash(b->bh.prev_hash);
	break;
      case 'm':
	print_hash(b->bh.merkle_hash);
	break;
      case 's':
	printf("%u", b->bh.timestamp);
	break;
      case 't':
	printf("%u", b->bh.target);
	break;
      case 'n':
	printf("%u", b->bh.nonce);
	break;
      case 'c':
	printf("%"PRIu64, b->bh.transaction_count);
	break;
      case 'h':
	print_reversed_hash(b->id);
	break;
      case 'N':
	printf("%u", b->height);
	break;
      case 'H':
	dump_block_header(&b->bh);
	break;
      default:
	goto bad_fmt;
      }
      break;
    case 't':
      if (!t)
	goto bad_fmt;
      switch (c[2]) {
      case 'h':
	print_reversed_hash(t->txid);
	break;
      case 'H':
	print_reversed_hash(t->wtxid);
	break;
      case 'v':
	printf("%u", t->version);
	break;
      case 'i':
	printf("%"PRIu64, t->input_count);
	break;
      case 'o':
	printf("%"PRIu64, t->output_count);
	break;
      case 't':
	printf("%u", t->lock_time);
	break;
      case 'L':
	printf("%u", t->total_len);
	break;
      case 'n':
	printf("%u", t->non_swlen);
	break;
      case 'l':
	printf("%u", segwit_length(t));
	break;
      case 'w':
	printf("%u", segwit_weight(t));
	break;
      case 'N':
	printf("%zu", txnum);
	break;
      case 'F':
	printf("%"PRIi64,
	       calculate_fees(utxo_map, t, txnum == 0));
	break;
      case 'D':
	printf("%"PRIi64,
	       calculate_bdd(utxo_map, t, txnum == 0, b->bh.timestamp));
	break;
      case 'X':
	dump_tx(t);
	break;
      case 'S':
	printf("%u", t->segwit);
	break;

      default:
	goto bad_fmt;
      }
      break;
    case 'i':
      if (!i)
	goto bad_fmt;
      switch (c[2]) {
      case 'h':
	print_reversed_hash(i->txid);
	break;
      case 'i':
	printf("%u", i->index);
	break;
      case 'l':
	printf("%"PRIu64, i->script_length);
	break;
      case 's':
	print_hex(i->script, i->script_length);
	break;
      case 'N':
	printf("%zu", i - t->input);
	break;
      case 'X':
	dump_tx_input(i);
	break;
      case 'a':
	/* Coinbase doesn't have valid input. */
	if (txnum != 0) {
	  struct utxo *utxo = utxo_map_get(utxo_map, i->txid);
	  printf("%"PRIu64, utxo->amount[i->index]);
	} else
	  printf("0");
	break;
      case 'B':
	/* Coinbase doesn't have valid input. */
	if (txnum != 0) {
	  struct utxo *utxo = utxo_map_get(utxo_map, i->txid);
	  printf("%u", utxo->height);
	} else
	  printf("0");
	break;
      case 'T':
	/* Coinbase doesn't have valid input. */
	if (txnum != 0) {
	  struct utxo *utxo = utxo_map_get(utxo_map, i->txid);
	  printf("%u", utxo->txnum);
	} else
	  printf("-1");
	break;
      case 'p':
	/* Coinbase doesn't have valid input. */
	if (txnum != 0) {
	  struct utxo *utxo = utxo_map_get(utxo_map, i->txid);
	  printf("%u", output_types(utxo)[i->index]);
	} else
	  printf("%u", UNKNOWN_OUTPUT);
	break;
      default:
	goto bad_fmt;
      }
      break;
    case 'o':
      if (!o)
	goto bad_fmt;
      switch (c[2]) {
      case 'a':
	printf("%"PRIu64, o->amount);
	break;
      case 'l':
	printf("%"PRIu64, o->script_length);
	break;
      case 's':
	print_hex(o->script, o->script_length);
	break;
      case 'q':
	printf("%u", i->sequence_number);
	break;
      case 'N':
	printf("%zu", o - t->output);
	break;
      case 'U':
	printf("%u", is_unspendable(o));
	break;
      case 'X':
	dump_tx_output(o);
	break;
      default:
	goto bad_fmt;
      }
      break;
    case 'u':
      if (!u)
	goto bad_fmt;
      switch (c[2]) {
      case 'h':
    print_reversed_hash(u->txid);
	break;
      case 's':
	printf("%u", u->timestamp);
	break;
      case 'N':
	printf("%u", u->height);
	break;
      case 'c':
	printf("%u", u->num_outputs);
	break;
      case 'u':
	printf("%u", u->unspent_outputs);
	break;
      case 'd':
	printf("%u", u->num_outputs - u->unspent_outputs);
	break;
      case 'U':
	printf("%"PRIu64, u->unspent);
	break;
      case 'D':
	printf("%"PRIu64, u->spent);
	break;
      case 'C':
	printf("%"PRIi64,
		 calculate_bdc(u, b->bh.timestamp, last_utxo_block->bh.timestamp));
	break;
      default:
	goto bad_fmt;
      }
      break;
    }

    /* Skip first two escape letters; loop will skip next */
    c += 2;
  }
  fputc('\n', stdout);
  return;
	
 bad_fmt:
  errx(1, "Bad %s format %.3s",
       i ? "input" : o ? "output" : t ? "transaction" : "block",
       c);
}
