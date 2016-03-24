#ifndef BITCOIN_ITERATE_UTILS_H
#define BITCOIN_ITERATE_UTILS_H
#include <openssl/sha.h>
#include <ccan/short_types/short_types.h>

#define SHA_FMT					\
  "%02x%02x%02x%02x%02x%02x%02x%02x"		\
  "%02x%02x%02x%02x%02x%02x%02x%02x"		\
  "%02x%02x%02x%02x%02x%02x%02x%02x"		\
  "%02x%02x%02x%02x%02x%02x%02x%02x"

#define SHA_VALS(e)						\
  e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7],		\
    e[8], e[9], e[10], e[11], e[12], e[13], e[14], e[15],	\
    e[16], e[17], e[18], e[19], e[20], e[21], e[22], e[23],	\
    e[24], e[25], e[26], e[27], e[28], e[29], e[30], e[31]

bool is_zero(u8 hash[SHA256_DIGEST_LENGTH]);
size_t hash_sha(const u8 *key);
char *opt_set_hash(const char *arg, u8 *h);

#endif /* BITCOIN_ITERATE_UTILS_H */
