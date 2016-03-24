#include "types.h"

struct block* new_block() {
  struct block *b;
  b = tal(NULL, struct block);
  return b;
}

void free_block(const tal_t *block) {
  tal_free(block);
}

struct bitcoin_block* new_bitcoin_block() {
  struct bitcoin_block *bh;
  bh = tal(NULL, struct bitcoin_block);
  return bh;
}

void free_bitcoin_block(const tal_t *bitcoin_block) {
  tal_free(bitcoin_block);
}
