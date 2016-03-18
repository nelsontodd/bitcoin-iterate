#include <ccan/tal/tal.h>
#include "utils.h"

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
