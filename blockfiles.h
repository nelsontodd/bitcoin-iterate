#ifndef BITCOIN_ITERATE_BLOCKFILES_H
#define BITCOIN_ITERATE_BLOCKFILES_H
#include <ccan/tal/tal.h>
#include "block.h"

/* Return a tal_array of filenames. */
char **block_filenames(tal_t *ctx, const char *base, bool testnet3);

struct file *block_file(char **block_fnames, unsigned int index, bool use_mmap);

size_t read_blockfiles(tal_t *tal_ctx, bool use_testnet, bool quiet, bool use_mmap, char **block_fnames, struct block_map *block_map, struct block **genesis, unsigned long block_end);

#endif /* BITCOIN_ITERATE_BLOCKFILES_H */

