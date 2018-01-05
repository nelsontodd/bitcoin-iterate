#include <sys/mman.h>
#include <ccan/tal/tal.h>
#include <ccan/err/err.h>
#include <ccan/tal/path/path.h>
#include <ccan/tal/str/str.h>
#include <ccan/short_types/short_types.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <dirent.h>
#include "io.h"
#include "blockfiles.h"
#include "parse.h"

#define CHUNK (128 * 1024 * 1024)

static void add_name(char ***names_p, unsigned int num, char *name)
{
  size_t count = tal_count(*names_p);
  if (num >= count) {
    tal_resize(names_p, num + 1);
    memset(*names_p + count, 0, sizeof(char *) * (num + 1 - count));
  }
  if ((*names_p)[num])
    errx(1, "Duplicate block file for %u? '%s' and '%s'",
	 num, name, (*names_p)[num]);
  (*names_p)[num] = name;
}

char **block_filenames(tal_t *ctx, const char *path, bool testnet3)
{
  char **names = tal_arr(ctx, char *, 0);
  char *tmp_ctx = tal_arr(ctx, char, 0);
  DIR *dir;
  struct dirent *ent;

  if (!path) {
    char *base = getenv("HOME");
    if (!base) {
      struct passwd *passwd = getpwuid(getuid());
      if (!passwd)
	err(1, "Could not get home dir");
      base = passwd->pw_dir;
    }

    base = path_join(tmp_ctx, base, ".bitcoin");
    if (testnet3)
      base = path_join(tmp_ctx, base, "testnet3");

    /* First try new-style: $HOME/.bitcoin/blocks/blk[0-9]*.dat. */
    path = path_join(tmp_ctx, base, "blocks");
    dir = opendir(path);
    if (!dir) {
      /* Old-style: $HOME/.bitcoin/blk[0-9]*.dat. */
      path = base;
      dir = opendir(path);
    }
  } else
    dir = opendir(path);

  if (!dir)
    err(1, "Could not open bitcoin dir '%s'", path);

  while ((ent = readdir(dir)) != NULL) {
    char *numstr;
    int num;
    if (!tal_strreg(tmp_ctx, ent->d_name,
		    "^blk([0-9]+)\\.dat$", &numstr))
      continue;
    num = strtol(numstr, NULL, 10);
    add_name(&names, num, path_join(names, path, ent->d_name));
  }
  tal_free(tmp_ctx);
  return names;
}

/* Cache file opens. */
struct file *block_file(char **block_fnames, unsigned int index, bool use_mmap)
{
#define NUM_BLOCKFILES 2
  static struct file f[NUM_BLOCKFILES];
  static size_t next;
  size_t i;

  for (i = 0; i < NUM_BLOCKFILES; i++) {
    if (f[i].name == block_fnames[index])
      return f+i;
  }

  /* Kick one out. */
  i = next;
  if (f[i].name)
    file_close(&f[i]);

  file_open(&f[i], block_fnames[index], 0,
	    O_RDONLY | (use_mmap ? 0 : O_NO_MMAP));
  next++;
  if (next == NUM_BLOCKFILES)
    next = 0;
  return f + i;
}

size_t read_blockfiles(tal_t *tal_ctx , bool use_testnet, bool quiet, bool use_mmap, char **block_fnames, struct block_map *block_map, struct block **genesis, unsigned long block_end)
{
	/*Read in all the blockfiles, check that they are all valid, and, starting from 
	 * block 0, try to find the header of the next block. 
	 * Then, try to add the block to the block map. If these pass, block_count++
	 */
  u32 netmarker;
  if (use_testnet) {
    netmarker = 0x0709110B;
  } else {
    netmarker = 0xD9B4BEF9;
  }
	
	block_map_init(block_map);
	size_t num_misses = 0;
  off_t last_discard;
  size_t i, block_count = 0;
  struct block *b;
  for (i = 0; i < tal_count(block_fnames); i++) {
    off_t off = 0;

    /* new-style starts from 1, old-style starts from 0 */
    if (!block_fnames[i]) {
      if (i)
				warnx("Missing block info for %zu", i);
      continue;
    }

    if (!quiet)
      fprintf(stderr, "bitcoin-iterate: Processing %s (%zi/%zu files, %zi blocks)\n",
	      block_fnames[i], i+1, tal_count(block_fnames), block_count);

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
      if (!read_block_header(&b->bh, f, &off,
				     b->sha, netmarker)) {
				tal_free(b);
			break;
      }

      b->pos = off;
			//if block b exists and can be added to the block map
			if (add_block(block_map, b, genesis, block_fnames, &num_misses)) { 
				/* Go 100 past the block they asked
				* for (avoid minor forks) */
				if (block_end != -1UL && b->height > block_end + 100) {
					if (!genesis)
						errx(1, "Could not find a genesis block.");
					i = tal_count(block_fnames);
					break;
					}
			}

      skip_transactions(&b->bh, block_start, &off);
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
  return block_count;
 }
