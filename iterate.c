#include <ccan/err/err.h>
#include <stdio.h>
#include "parse.h"
#include "block.h"
#include "blockfiles.h"
#include "cache.h"
#include "iterate.h"

#define BLOCK_PROGRESS_PERIOD 10000

static void set_heights_and_best(struct block **best, struct block *genesis, struct block_map *block_map)
{
  struct block *b;
  struct block_map_iter it;
  for (b = block_map_first(block_map, &it);
       b;
       b = block_map_next(block_map, &it)) {
    set_height(block_map, b);
    if (b->height > (*best)->height)
      *best = b;
  }
}

static void set_blockchain_end(u8 *tip, struct block **best, struct block_map *block_map)
{
  if (!is_zero(tip)) {
    *best = block_map_get(block_map, tip);
    if (!*best)
      errx(1, "Unknown --end-hash block "SHA_FMT, SHA_VALS(tip));
  }
}

static void set_blockchain_start(u8 *start_hash, struct block **start, struct block_map *block_map)
{
  if (!is_zero(start_hash)) {
    *start = block_map_get(block_map, start_hash);
    if (!*start)
      errx(1, "Unknown --start-hash block "SHA_FMT, SHA_VALS(start_hash));
  }
}

static void link_blocks(struct block *best, struct block_map *block_map)
{
  struct block *b, *next = NULL;
  for (b = best; b; b = block_map_get(block_map, b->bh.prev_hash)) {
    b->next = next;
    next = b;
  }
}  

static void set_iteration_end(unsigned long block_end, struct block **best, struct block *genesis)
{
  if (block_end != -1UL) {
    struct block *b;
    for (b = genesis; b->height != block_end; b = b->next) {
      if (!b->next)
	errx(1, "No block end %lu found", block_end);
    }
    *best = b;
    b->next = NULL;
  }
}

static void set_iteration_start(unsigned long block_start, struct block **start, struct block *genesis)
{
  if (block_start != 0) {
    struct block *b;
    for (b = genesis; b->height != block_start; b = b->next) {
      if (!b->next)
		errx(1, "No block start %lu found", block_start);
    }
    *start = b;
  }
}

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
	     utxo_function utxofn)
{
  void *tal_ctx = tal(NULL, char);
  size_t i, block_count = 0;
  bool needs_fee;
  struct block *b, *best = NULL, *genesis = NULL, *start = NULL, *last_utxo_block = NULL;
  struct block_map block_map;
  struct utxo_map utxo_map;
  struct space space;
  static char **block_fnames;

  block_fnames = block_filenames(tal_ctx, blockdir, use_testnet);

  block_count = read_blockchain(tal_ctx,
		  quiet, use_mmap,
		  use_testnet, cachedir,
		  block_fnames,
		  &block_map, &genesis, block_end);

  best  = genesis;
  start = genesis;
  set_heights_and_best(&best, genesis, &block_map);
  set_blockchain_end(tip, &best, &block_map);
  set_blockchain_start(start_hash, &start, &block_map);
  link_blocks(best, &block_map);

  set_iteration_end(block_end, &best, genesis);
  set_iteration_start(block_start, &start, genesis);
  if (!quiet) {
    fprintf(stderr, "bitcoin-iterate: Iterating between block heights %u and %u (of %zu total blocks)\n",
	   start->height, best->height, block_count);
    if (utxofn) {
      fprintf(stderr, "bitcoin-iterate: Iterating over UTXOs every %u blocks\n", utxo_period);
    } else {
      fprintf(stderr, "bitcoin-iterate: Not iterating over UTXOs\n");
    }
  }
  
  utxo_map_init(&utxo_map);
 	
  needs_fee = needs_utxo;
  /* Do we have cache utxo? */
  if (cachedir && start && needs_utxo) {
    if (read_utxo_cache(tal_ctx, quiet, &utxo_map, cachedir, start->id)) {
      needs_fee = false;
    } else if (!quiet)
      fprintf(stderr, "bitcoin-iterate: Did not find valid UTXO cache\n");
  }

  int blocks_iterated = 0;
  /* Now run forwards. */
  for (b = genesis; b; b = b->next) {
    off_t off;
    struct transaction *tx;

    if (!quiet && (b->height > 0) && (b->height % BLOCK_PROGRESS_PERIOD) == 0) {
      fprintf(stderr,"bitcoin-iterate: Iterating over block number %i\n",b->height);
    }

    if (b == start) { 
      /* Are we UTXO caching? */
      if (cachedir && needs_utxo) {
        if (needs_fee) {
		  /* Save cache for next time. */
		  write_utxo_cache(&utxo_map, quiet, cachedir, b->id);
        } else {
		  /* We loaded cache, now we calc fee. */
		  needs_fee = true;
		}
      }
      start = NULL; 
    }

    if (!start && blockfn){
      blockfn(&utxo_map, b);
    }

    if (!start && progress_marks && b->height % (best->height / progress_marks) == (best->height / progress_marks) - 1)
      fprintf(stderr, ".");

    /* Don't read transactions if we don't have to */
    if (!txfn && !inputfn && !outputfn && !utxofn)
      continue;

    /* If we haven't started and don't need to gather UTXO, skip */
    if (start && !needs_fee)
      continue;
		
    off = b->pos;

    space_init(&space);
    tx = space_alloc_arr(&space, struct transaction,
			 b->bh.transaction_count);
    for (i = 0; i < b->bh.transaction_count; i++) {
      size_t j;

      read_transaction(&space, &tx[i],
			       block_file(block_fnames, b->filenum, use_mmap), &off);
      if (!start && txfn)
	txfn(&utxo_map, b, &tx[i], i);

      if (!start && inputfn) {
	for (j = 0; j < tx[i].input_count; j++) {
	  inputfn(&utxo_map, b, &tx[i], i, &tx[i].input[j]);
	}
      }
      
      if (!start && outputfn) {
	for (j = 0; j < tx[i].output_count; j++) {
	  outputfn(&utxo_map, b, &tx[i], i, &tx[i].output[j]);
	}
      }

      if (needs_fee) {
	/* Now we can release consumed utxos;
	 * before there was a possibility of %tF */
	/* Coinbase inputs are not real */
	if (i != 0) {
	  for (j = 0; j < tx[i].input_count; j++)
	    release_utxo(&utxo_map, &tx[i].input[j]);
	}

	/* And add this tx's outputs to utxo */
	add_utxos(tal_ctx, &utxo_map, b, &tx[i], i);
      }
    }
    
    if (!start) {
      blocks_iterated += 1;
    }

    if (!start && utxofn && ((blocks_iterated % utxo_period) == 0)) {
      struct utxo_map_iter it;
      struct utxo *utxo;
      for (utxo = utxo_map_first(&utxo_map, &it);
				 utxo;
				 utxo = utxo_map_next(&utxo_map, &it)) {
		 		 		utxofn(&utxo_map, b, last_utxo_block, utxo);
	  		}
    }
    if (!start && utxofn) {
        last_utxo_block = b;
    }
		
  }
}
