/* GPLv2 or later, see LICENSE */

#include <ccan/err/err.h>
#include <ccan/tal/tal.h>
#include <ccan/tal/path/path.h>
/* #include <ccan/take/take.h> */
/* #include <ccan/short_types/short_types.h> */
/* #include <ccan/opt/opt.h> */
/* #include <ccan/htable/htable_type.h> */
/* #include <ccan/rbuf/rbuf.h> */
/* #include <ccan/tal/str/str.h> */
/* #include <ccan/str/hex/hex.h> */
/* #include <ccan/tal/grab_file/grab_file.h> */
#include <sys/types.h>
#include <sys/mman.h>
#include <stdbool.h>
/* #include <unistd.h> */
#include <sys/stat.h>
/* #include <fcntl.h> */
#include <stdio.h>
/* #include <assert.h> */
/* #include <inttypes.h> */
/* #include <errno.h> */
#include "parse.h"
#include "blockfiles.h"
/* #include "io.h" */
#include "dump.h"
#include "space.h"
#include "block.h"
#include "utxo.h"
#include "cache.h"
#include "utils.h"
#include "iterate.h"

#define CHUNK (128 * 1024 * 1024)

void iterate(char *blockdir, char *cachedir,
	     bool use_testnet,
	     unsigned long block_start, unsigned long block_end,
	     u8 *start_hash, u8 *tip,
	     unsigned int utxo_period, bool use_mmap,
	     unsigned progress_marks, bool quiet,
	     char *blockfmt, char *txfmt, char *inputfmt, char *outputfmt, char *utxofmt,
	     block_function *blockfn,
	     transaction_function *txfn,
	     input_function *inputfn,
	     output_function *outputfn,
	     utxo_function *utxofn)
{
  void *tal_ctx = tal(NULL, char);
  size_t i, block_count = 0;
  off_t last_discard;
  bool needs_utxo, needs_fee;
  struct block *b, *best, *genesis = NULL, *next, *start = NULL;
  struct block_map block_map;
  char *blockcache = NULL;
  struct block_map_iter it;
  struct utxo_map utxo_map;
  struct space space;
  u32 netmarker;
  static char **block_fnames;
	    
  if (use_testnet) {
    netmarker = 0x0709110B;
  } else {
    netmarker = 0xD9B4BEF9;
  }

  block_fnames = block_filenames(tal_ctx, blockdir, use_testnet);

  if (cachedir && tal_count(block_fnames)) {
    size_t last = tal_count(block_fnames) - 1;
    struct stat cache_st, block_st;

    /* Cache matches name of final block file */
    blockcache = path_join(tal_ctx, cachedir,
			   path_basename(tal_ctx,
					 block_fnames[last]));

    if (stat(block_fnames[last], &block_st) != 0)
      errx(1, "Could not stat %s", block_fnames[last]);
    if (stat(blockcache, &cache_st) == 0) {
      if (block_st.st_mtime >= cache_st.st_mtime) {
	if (!quiet)
	  printf("%s is newer than cache\n",
		 block_fnames[last]);
      } else {
	read_blockcache(tal_ctx, quiet,
			&block_map, blockcache,
			&genesis, block_fnames);
	goto check_genesis;
      }
    }
  }
  block_map_init(&block_map);
	
  for (i = 0; i < tal_count(block_fnames); i++) {
    off_t off = 0;

    /* new-style starts from 1, old-style starts from 0 */
    if (!block_fnames[i]) {
      if (i)
	warnx("Missing block info for %zu", i);
      continue;
    }

    if (!quiet)
      printf("bitcoin-iterate: processing %s (%zi/%zu)\n",
	     block_fnames[i], i+1, tal_count(block_fnames));

    last_discard = off = 0;
    for (;;) {
      off_t block_start;
      struct file *f = block_file(block_fnames, i, use_mmap);

      block_start = off;
      if (!next_block_header_prefix(f, &off, netmarker)) {
	if (off != block_start)
	  warnx("Skipped %lu at end of %s",
		off - block_start, block_fnames[i]);
	break;
      }
      if (off != block_start)
	warnx("Skipped %lu@%lu in %s",
	      off - block_start, block_start,
	      block_fnames[i]);

      block_start = off;
      b = tal(tal_ctx, struct block);
      b->filenum = i;
      b->height = -1;
      if (!read_bitcoin_block_header(&b->bh, f, &off,
				     b->sha, netmarker)) {
	tal_free(b);
	break;
      }

      b->pos = off;
      add_block(&block_map, b, &genesis, block_fnames);

      skip_bitcoin_transactions(&b->bh, block_start, &off);
      if (off > last_discard + CHUNK && f->mmap) {
	size_t len = CHUNK;
	if ((size_t)last_discard + len > f->len)
	  len = f->len - last_discard;
	madvise(f->mmap + last_discard, len,
		MADV_DONTNEED);
	last_discard += len;
      }
      block_count++;
    }
  }

  if (blockcache) {
    write_blockcache(&block_map, cachedir, blockcache);
    if (!quiet)
      printf("Wrote block cache to %s\n", blockcache);
  }

 check_genesis:
  if (!genesis)
    errx(1, "Could not find a genesis block.");

  /* Link up prevs. */
  best = genesis;
  for (b = block_map_first(&block_map, &it);
       b;
       b = block_map_next(&block_map, &it)) {
    set_height(&block_map, b);
    if (b->height > best->height)
      best = b;
  }

  /* If they told us a tip, that overrides. */
  if (!is_zero(tip)) {
    best = block_map_get(&block_map, tip);
    if (!best)
      errx(1, "Unknown --end block "SHA_FMT, SHA_VALS(tip));
  }
			
  /* If they told us a start, make sure it exists. */
  if (!is_zero(start_hash)) {
    start = block_map_get(&block_map, start_hash);
    if (!start)
      errx(1, "Unknown --start block "SHA_FMT,
	   SHA_VALS(start_hash));
  }

  if (!quiet)
    printf("bitcoin-iterate: best block height: %u (of %zu)\n",
	   best->height, block_count);

  /* Now iterate down from best, setting next pointers. */
  next = NULL;
  for (b = best; b; b = block_map_get(&block_map, b->bh.prev_hash)) {
    b->next = next;
    next = b;
  }

  /* If they told us to end somewhere, do that. */
  if (block_end != -1UL) {
    for (b = genesis; b->height != block_end; b = b->next) {
      if (!b->next)
	errx(1, "No block end %lu found", block_end);
    }
    best = b;
    b->next = NULL;
  }

  /* Similar with start block */
  if (block_start != 0) {
    for (b = genesis; b->height != block_start; b = b->next) {
      if (!b->next)
	errx(1, "No block start %lu found", block_start);
    }
    start = b;
  }

  utxo_map_init(&utxo_map);

  /* Optimization: figure out of we have to maintain UTXO map */
  needs_utxo = false;

  /* We need it for fee calculation, UTXO block number, or
   * bitcoin days created/destroyed.  Can be asked by tx, input,
   * output, or UTXO. */
  if (txfmt && strstr(txfmt, "%tF"))
    needs_utxo = true;
  if (txfmt && strstr(txfmt, "%tD"))
    needs_utxo = true;
  if (inputfmt && strstr(inputfmt, "%tF"))
    needs_utxo = true;
  if (inputfmt && strstr(inputfmt, "%tD"))
    needs_utxo = true;
  if (inputfmt && strstr(inputfmt, "%iB"))
    needs_utxo = true;
  if (inputfmt && strstr(inputfmt, "%ia"))
    needs_utxo = true;
  if (inputfmt && strstr(inputfmt, "%ip"))
    needs_utxo = true;
  if (outputfmt && strstr(outputfmt, "%tF"))
    needs_utxo = true;
  if (outputfmt && strstr(outputfmt, "%tD"))
    needs_utxo = true;
  if (utxofmt)
    needs_utxo = true;
  if (txfn)
    needs_utxo = true;
  if (utxofn)
    needs_utxo = true;
 	
  needs_fee = needs_utxo;

  /* Do we have cache utxo? */
  if (cachedir && start && needs_utxo) {
    if (read_utxo_cache(tal_ctx, &utxo_map, cachedir, start->sha)) {
      needs_fee = false;
      if (!quiet)
	printf("Found valid UTXO cache\n");
    } else if (!quiet)
      printf("Did not find valid UTXO cache\n");
  }

  /* Now run forwards. */
  for (b = genesis; b; b = b->next) {
    off_t off;
    struct bitcoin_transaction *tx;

    if (b == start) {
      /* Are we UTXO caching? */
      if (cachedir && needs_utxo) {
	if (needs_fee) {
	  /* Save cache for next time. */
	  write_utxo_cache(&utxo_map, cachedir,
			   b->sha);
	  if (!quiet)
	    printf("Wrote UTXO cache\n");
	} else
	  /* We loaded cache, now we calc fee. */
	  needs_fee = true;
      }
      start = NULL;
    }

    if (!start && blockfmt)
      print_format(blockfmt, &utxo_map, b, NULL, 0, NULL, NULL, NULL);

    if (!start && blockfn)
      (*blockfn)(*b, b->bh);

    if (!start && progress_marks
	&& b->height % (best->height / progress_marks)
	== (best->height / progress_marks) - 1)
      fprintf(stderr, ".");

    /* Don't read transactions if we don't have to */
    if (!txfmt && !inputfmt && !outputfmt)
      continue;

    /* If we haven't started and don't need to gather UTXO, skip */
    if (start && !needs_fee)
      continue;
		
    off = b->pos;

    space_init(&space);
    tx = space_alloc_arr(&space, struct bitcoin_transaction,
			 b->bh.transaction_count);
    for (i = 0; i < b->bh.transaction_count; i++) {
      size_t j;
      off_t txoff = off;

      read_bitcoin_transaction(&space, &tx[i],
			       block_file(block_fnames, b->filenum, use_mmap), &off);

      if (!start && txfmt)
	print_format(txfmt, &utxo_map, b, &tx[i], i,
		     NULL, NULL, NULL);
      if (!start && txfn)
	(*txfn)(*b, *tx);

      if (!start && inputfmt) {
	for (j = 0; j < tx[i].input_count; j++) {
	  print_format(inputfmt, &utxo_map, b,
		       &tx[i], i, &tx[i].input[j],
		       NULL, NULL);
	}
      }
      if (!start && inputfn) {
	for (j = 0; j < tx[i].input_count; j++) {
	  (*inputfn)(*b, *tx, tx[i].input[j]);
	}
      }
      
      if (!start && outputfmt) {
	for (j = 0; j < tx[i].output_count; j++) {
	  print_format(outputfmt, &utxo_map, b,
		       &tx[i], i, NULL,
		       &tx[i].output[j], NULL);
	}
      }
      if (!start && outputfn) {
	for (j = 0; j < tx[i].output_count; j++) {
	  (*outputfn)(*b, *tx, tx[i].output[j]);
	}
      }
      

      if (needs_fee) {
	/* Now we can release consumed utxos;
	 * before there was a possibility of %tF */
	/* Coinbase inputs are not real */
	if (i != 0) {
	  for (j = 0; j < tx[i].input_count; j++)
	    release_utxo(&utxo_map,
			 &tx[i].input[j]);
	}

	/* And add this tx's outputs to utxo */
	add_utxo(tal_ctx, &utxo_map, b, &tx[i], i, txoff);
      }
    }
    if (utxofmt && ((b->height % utxo_period) == 0)) {
      struct utxo_map_iter it;
      struct utxo *utxo;
      for (utxo = utxo_map_first(&utxo_map, &it);
	   utxo;
	   utxo = utxo_map_next(&utxo_map, &it)) {
	print_format(utxofmt, &utxo_map, b, NULL, 0, NULL, NULL, utxo);
      }
    }
    if (utxofn && ((b->height % utxo_period) == 0)) {
      struct utxo_map_iter it;
      struct utxo *utxo;
      for (utxo = utxo_map_first(&utxo_map, &it);
	   utxo;
	   utxo = utxo_map_next(&utxo_map, &it)) {
	(*utxofn)(*b, *utxo);
      }
    }
		
  }
}
