#include <ccan/err/err.h>
#include "utxo.h"
#include "utils.h"

const u8 *keyof_utxo(const struct utxo *utxo)
{
  return utxo->txid;
}

bool utxohash_eq(const struct utxo *utxo, const u8 *key)
{
  return memcmp(&utxo->txid, key, sizeof(utxo->txid)) == 0;
}

bool is_unspendable(const struct output *o)
{
  return (o->script_length > 0 && o->script[0] == OP_RETURN);
}

u8 *output_types(struct utxo *utxo)
{
  return (u8 *)&utxo->amount[utxo->num_outputs];
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

void add_utxo(const tal_t *tal_ctx,
	      struct utxo_map *utxo_map,
	      const struct block *b,
	      const struct transaction *t,
	      u32 txnum, off_t off)
{
  struct utxo *utxo;
  unsigned int i;
  unsigned int spend_count = 0;
  u64 initial_spent = 0, initial_unspent = 0;

  for (i = 0; i < t->output_count; i++) {
    if (!is_unspendable(&t->output[i])) {
      spend_count++;
      initial_unspent += t->output[i].amount;
    } else {
      initial_spent += t->output[i].amount;
    }
  }

  if (spend_count == 0)
    return;

  utxo = tal_alloc_(tal_ctx, sizeof(*utxo) + (sizeof(utxo->amount[0]) + 1)
		    * t->output_count, false, TAL_LABEL(struct utxo, ""));

  memcpy(utxo->txid, t->txid, sizeof(utxo->txid));
  utxo->num_outputs = t->output_count;
  utxo->unspent_outputs = spend_count;
  utxo->height = b->height;
  utxo->timestamp = b->bh.timestamp;
  utxo->spent   = initial_spent;
  utxo->unspent = initial_unspent;
  utxo->txnum   = txnum;
  for (i = 0; i < utxo->num_outputs; i++) {
    utxo->amount[i] = t->output[i].amount;
  }
  guess_output_types(t, output_types(utxo));

  utxo_map_add(utxo_map, utxo);
}

void release_utxo(struct utxo_map *utxo_map,
		  const struct input *i)
{
  struct utxo *utxo;

  utxo = utxo_map_get(utxo_map, i->txid);
  if (!utxo)
    errx(1, "Unknown utxo for "SHA_FMT, SHA_VALS(i->txid));

  utxo->spent   += utxo->amount[i->index];
  utxo->unspent -= utxo->amount[i->index];

  if (--utxo->unspent_outputs == 0) {
    utxo_map_del(utxo_map, utxo);
    tal_free(utxo);
  }
}
