#include <ccan/tal/tal.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/str/str.h>
#include "utils.h"

char *opt_set_hash(const char *arg, u8 *h)
{
  if (!hex_decode(arg, strlen(arg), h, SHA256_DIGEST_LENGTH))
    return "Bad hex string (needs 64 hex chars)";
  return NULL;
}

bool is_zero(u8 hash[SHA256_DIGEST_LENGTH])
{
  unsigned int i;

  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    if (hash[i] != 0)
      return false;
  }
  return true;
}

size_t hash_sha(const u8 *key)
{
  size_t ret;

  memcpy(&ret, key, sizeof(ret));
  return ret;
}
