#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/path/path.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/err/err.h>
#include "cache.h"
#include "blockfiles.h"

bool read_utxo_cache(const tal_t *ctx,
		     bool quiet,
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
  fprintf(stderr,"Cache.c file: %s\n",file);
  if (!quiet)
    fprintf(stderr, "bitcoin-iterate: Reading UTXOs from cache at %s\n", file);
  
  contents = grab_file(file,file);
  if (!contents) {
    tal_free(file);
    fprintf(stderr,"Cache.c contents was empty %s\n", contents);
    return false;
  }
  fprintf(stderr,"Cache.c contents was not empty %s\n", contents);

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
		      bool quiet,
		      const char *cachedir,
		      const u8 *blockid)
{
  char *file;
  char blockhex[hex_str_size(SHA256_DIGEST_LENGTH)];
  struct utxo_map_iter it;
  struct utxo *utxo;
  int fd;

  fprintf(stderr, "Cache.c write_utxo_cache function before path_join: Cache value %s \n", cachedir);
  hex_encode(blockid, SHA256_DIGEST_LENGTH, blockhex, sizeof(blockhex));
  file = path_join(NULL, cachedir, blockhex);
  fprintf(stderr, "Cache.c write_utxo_cache function post path_join: Cache value %s \n", cachedir);
  if (!quiet)
    fprintf(stderr, "bitcoin-iterate: Writing UTXOs to cache at %s\n", file);
  
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

char  * set_blockcache_path(char *blockcache, tal_t *tal_ctx, char *cachedir, char *last_block_fname)
{
  fprintf(stderr, "In set_blockcache_path \n");
  fprintf(stderr, "blockcache: %s \n",blockcache);
  fprintf(stderr, "cachedir: %s \n",cachedir);
  fprintf(stderr, "last block file name: %s \n",last_block_fname);
  blockcache = path_join(tal_ctx,
			 cachedir,
			 path_basename(tal_ctx, last_block_fname));
  fprintf(stderr, "blockcache: %s \n",blockcache);
  return blockcache;
}

static bool blockcache_is_valid(bool quiet, char *blockcache, char *last_block_fname)
{
  struct stat cache_st, block_st;
  
  if (stat(last_block_fname, &block_st) != 0)
    errx(1, "Could not stat %s", last_block_fname);
  fprintf(stderr, "Testing if blockcache is valid, blockcache: %s \n",blockcache);
  if (stat(blockcache, &cache_st) == 0) {
    if (block_st.st_mtime >= cache_st.st_mtime) {
      /* Cache file is older than (or as old as) last block file */
      if (!quiet)
	fprintf(stderr, "bitcoin-iterate: %s is newer than cache\n", last_block_fname);
      return false;
    } else {
      /* Cache file is newer than last block file */
      return true;
    }
  } else {
    /* No cache file */
    return false;
  }
}

static size_t read_blockcache(const tal_t *tal_ctx,
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
    fprintf(stderr, "bitcoin-iterate: Adding %zu blocks from cache at %s\n", num, blockcache);

  block_map_init_sized(block_map, num);
  for (i = 0; i < num; i++)
    add_block(block_map, &b[i], genesis, block_fnames);

  return num;
}


size_t read_blockchain(tal_t *tal_ctx,
		       bool quiet, bool use_mmap,
		       bool use_testnet, char *cachedir, char *blockcache,
		       char **block_fnames,
		       struct block_map *block_map, struct block **genesis)
{
  size_t block_count = 0;
  if (cachedir && tal_count(block_fnames)) {
    size_t last = tal_count(block_fnames) - 1;
    char *last_block_fname = block_fnames[last];
		blockcache = set_blockcache_path(blockcache, tal_ctx, cachedir, last_block_fname);
    fprintf(stderr,"In cache.c, after running set_blockcache, blockcache: %s \n", blockcache);
    if (blockcache_is_valid(quiet, blockcache, last_block_fname)) {
		fprintf(stderr,"In cache.c, blockcache was valid \n");
      block_count = read_blockcache(tal_ctx, quiet,
				    block_map, blockcache,
				    genesis, block_fnames);
    }
	else {
      block_map_init(block_map);
      block_count = read_blockfiles(tal_ctx,
				    use_testnet, quiet, use_mmap,
				    block_fnames,
				    block_map, genesis);
    }
    if (!*genesis)
      errx(1, "Could not find a genesis block.");
  } else{
      block_map_init(block_map);
      block_count = read_blockfiles(tal_ctx,
				    use_testnet, quiet, use_mmap,
				    block_fnames,
				    block_map, genesis);
  }
  fprintf(stderr,"In cache.c outside blockcache if, blockcache: %s \n",blockcache);
  if (blockcache) {
     fprintf(stderr,"In cache.c Blockcache value: %s", blockcache);
     write_blockcache(block_map, quiet, cachedir, blockcache);
  }
  return block_count;
}

void write_blockcache(struct block_map *block_map,
		      bool quiet,
		      const char *cachedir,
		      const char *blockcache)
{
  struct block_map_iter it;
  struct block *b;
  int fd;

  if (!quiet)
    fprintf(stderr, "bitcoin-iterate: Writing blocks to cache at %s\n", blockcache);

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
