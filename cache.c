#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/path/path.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/err/err.h>
#include "cache.h"

bool read_utxo_cache(const tal_t *ctx,
		     struct utxo_map *utxo_map,
		     const char *cachedir,
		     const u8 *blockid)
{
  char blockhex[hex_str_size(SHA256_DIGEST_LENGTH)];
  char *file;
  char *contents;
  size_t bytes;

  hex_encode(blockid, SHA256_DIGEST_LENGTH, blockhex, sizeof(blockhex));
  file = path_join(NULL, cachedir, blockhex);
  contents = grab_file(file, file);
  if (!contents) {
    tal_free(file);
    return false;
  }

  bytes = tal_count(contents) - 1;

  /* Size UTXO appropriately immediately (slightly oversize). */
  utxo_map_clear(utxo_map);
  utxo_map_init_sized(utxo_map, bytes / sizeof(struct utxo));

  while (bytes) {
    struct utxo *utxo;
    size_t size = sizeof(*utxo) + sizeof(utxo->amount[0])
      * ((struct utxo *)contents)->num_outputs;

    /* Truncated? */
    if (size > bytes) {
      warnx("Truncated cache file %s: deleting", file);
      unlink(file);
      tal_free(file);
      return false;
    }
    utxo = tal_alloc_(ctx, size, false, TAL_LABEL(struct utxo, ""));
    memcpy(utxo, contents, size);
    utxo_map_add(utxo_map, utxo);

    contents += size;
    bytes -= size;
  }
  tal_free(file);
  return true;
}

void write_utxo_cache(const struct utxo_map *utxo_map,
		      const char *cachedir,
		      const u8 *blockid)
{
  char *file;
  char blockhex[hex_str_size(SHA256_DIGEST_LENGTH)];
  struct utxo_map_iter it;
  struct utxo *utxo;
  int fd;

  hex_encode(blockid, SHA256_DIGEST_LENGTH, blockhex, sizeof(blockhex));
  file = path_join(NULL, cachedir, blockhex);

  fd = open(file, O_WRONLY|O_CREAT|O_EXCL, 0600);
  if (fd < 0) {
    if (errno != EEXIST) 
      err(1, "Creating '%s' for writing", file);
    tal_free(file);
    return;
  }

  for (utxo = utxo_map_first(utxo_map, &it);
       utxo;
       utxo = utxo_map_next(utxo_map, &it)) {
    size_t size = sizeof(*utxo) + sizeof(utxo->amount[0])
      * utxo->num_outputs;
    if (write(fd, utxo, size) != size)
      errx(1, "Short write to %s", file);
  }
}

void read_blockcache(const tal_t *tal_ctx,
		     bool quiet,
		     struct block_map *block_map,
		     const char *blockcache,
		     struct block **genesis,
		     char **block_fnames)
{
  size_t i, num;
  struct block *b = grab_file(tal_ctx, blockcache);

  if (!b)
    err(1, "Could not read %s", blockcache);

  num = (tal_count(b) - 1) / sizeof(*b);
  if (!quiet)
    printf("Adding %zu blocks from cache\n", num);

  block_map_init_sized(block_map, num);
  for (i = 0; i < num; i++)
    add_block(block_map, &b[i], genesis, block_fnames);
}

void write_blockcache(struct block_map *block_map,
		      const char *cachedir,
		      const char *blockcache)
{
  struct block_map_iter it;
  struct block *b;
  int fd;

  fd = open(blockcache, O_WRONLY|O_CREAT|O_TRUNC, 0600);
  if (fd < 0 && errno == ENOENT) {
    if (mkdir(cachedir, 0700) != 0)
      err(1, "Creating cachedir '%s'", cachedir);
    fd = open(blockcache, O_WRONLY|O_CREAT|O_EXCL, 0600);
  }
  if (fd < 0)
    err(1, "Creating '%s' for writing", blockcache);

  for (b = block_map_first(block_map, &it);
       b;
       b = block_map_next(block_map, &it)) {
    if (write(fd, b, sizeof(*b)) != sizeof(*b))
      err(1, "Short write to %s", blockcache);
  }
  close(fd);
}
