#include <ccan/err/err.h>
#include "utxo.h"
#include "utils.h"
#include "types.h"

const u8 *keyof_utxo(const struct utxo *utxo)
{
  return utxo->txid;
}

size_t hashof_utxo(const u8 *key)
{
  size_t ret;
  memcpy(&ret, key, (SHA256_DIGEST_LENGTH + 1) * sizeof(u32));
  return ret;
}

bool utxohash_eq(const struct utxo *utxo, const u8 *key)
{
  return memcmp(&utxo->txid, key, sizeof(utxo->txid) + sizeof(utxo->index)) == 0;
}

bool is_unspendable(const struct output *o)
{
  return (o->script_length > 0 && o->script[0] == OP_RETURN);
}

/* Only classify two-output txs for the moment, assuming round numbers
 * are payments.  Furthur ideas from Harold:
 *
 * 1) if the input is P2SH, and there's a P2SH and P2PKH output, then
 *    it's obvious.
 *
 * 2) lots of wallets still send change to the same public key hash as
 *    they were received originally.
 */
static void guess_output_types(const struct transaction *t, u8 *types)
{
  if (t->output_count == 2) {
    bool first_round = ((t->output[0].amount % 1000) == 0);
    bool second_round = ((t->output[1].amount % 1000) == 0);
    if (first_round != second_round) {
      if (first_round) {
	types[0] = PAYMENT_OUTPUT;
	types[1] = CHANGE_OUTPUT;
      } else {
	types[1] = PAYMENT_OUTPUT;
	types[0] = CHANGE_OUTPUT;
      }
      return;
    }
  }
  memset(types, UNKNOWN_OUTPUT, t->output_count);
}

static void add_utxo(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct transaction *t,
	      u32 txnum,
        u32 index,
        u8 *types)
{

	if (is_unspendable(&t->output[index])) {
	  return;
	}
	struct utxo *utxo;
  utxo = tal_alloc_(tal_ctx, sizeof(*utxo), false, TAL_LABEL(struct utxo, ""));
  memcpy(utxo->txid, t->txid, sizeof(utxo->txid));
	utxo->index     = index;
  utxo->height    = b->height;
  utxo->timestamp = b->bh.timestamp;
	utxo->txnum     = txnum;
	utxo->amount    = t->output[index].amount;
	utxo->type      = types[index];
	utxo_map_add(utxo_map, utxo);
}

void add_utxos(const tal_t *tal_ctx,
	       struct utxo_map *utxo_map,
	       const struct block *b,
	       const struct transaction *t,
	       u32 txnum)
{
  unsigned int i;
  u8 types[t->output_count];
  guess_output_types(t, types);
  for (i=0; i<t->output_count; i++) {
	  add_utxo(tal_ctx, utxo_map, b, t, txnum, i, types);
	}
}

void release_utxo(struct utxo_map *utxo_map, const struct input *i)
{
  struct utxo *utxo;

  utxo = utxo_map_get(utxo_map, i->txid);
  if (!utxo) {
	  errx(1, "Unknown utxo for transaction "SHA_FMT" output %i", SHA_VALS(i->txid), i->index);
	}

  utxo_map_del(utxo_map, utxo);
  tal_free(utxo);
}
