#ifndef BITCOIN_ITERATE_BLOCKFILES_H
#define BITCOIN_ITERATE_BLOCKFILES_H
#include <ccan/tal/tal.h>

/* Return a tal_array of filenames. */
char **block_filenames(tal_t *ctx, const char *base, bool testnet3);

struct file *block_file(char **block_fnames, unsigned int index, bool use_mmap);

#endif /* BITCOIN_ITERATE_BLOCKFILES_H */

