from libc.stdint cimport int64_t, uint64_t
from libc.stdint cimport int32_t, uint32_t
from libc.stdint cimport int16_t, uint16_t
from libc.stdint cimport  int8_t,  uint8_t

from posix.types cimport off_t
from libcpp cimport bool

cdef extern from "openssl/sha.h":
  enum: SHA256_DIGEST_LENGTH

cdef extern from "ccan/tal/tal.h":
  ctypedef void tal_t

cdef extern from "ccan/short_types/short_types.h":

  ctypedef uint64_t u64
  ctypedef uint32_t u32
  ctypedef uint16_t u16
  ctypedef  uint8_t  u8
  
  ctypedef int64_t  s64
  ctypedef int32_t  s32
  ctypedef int16_t  s16
  ctypedef  int8_t   s8

cdef extern from "types.h":

  ctypedef u64 varint_t

  cdef struct bitcoin_block:
    u32 len
    u32 version
    u8 prev_hash[32]
    u8 merkle_hash[32]
    u32 timestamp
    u32 target
    u32 nonce
    varint_t transaction_count

  cdef struct bitcoin_transaction:
    u32 version
    varint_t input_count
    varint_t output_count
    u32 lock_time
    u8 sha256[SHA256_DIGEST_LENGTH]
    u32 len

  cdef struct bitcoin_transaction_output:
    u64 amount
    varint_t script_length
    u8 *script

  cdef struct bitcoin_transaction_input:
    u8 hash[32]
    u32 index
    varint_t script_length
    u8 *script
    u32 sequence_number

  cdef struct block:
    u8 sha[SHA256_DIGEST_LENGTH]
    s32 height
    unsigned int filenum
    off_t pos

  cdef struct utxo:
    u8 tx[SHA256_DIGEST_LENGTH]
    u32 timestamp
    unsigned int height
    u32 unspent_outputs
    u64 unspent
    u64 spent
    # u64 amount[]

  block* new_block()
  bitcoin_block* new_bitcoin_block()

  void *free_block(const tal_t *bitcoin_block)
  void *free_bitcoin_block(const tal_t *bitcoin_block)
  
cdef extern from "iterate.h":

  ctypedef void (*block_function)(block b, bitcoin_block bh)
  ctypedef void (*transaction_function)(block b, bitcoin_transaction t)
  ctypedef void (*input_function)(block b, bitcoin_transaction t, bitcoin_transaction_input i)
  ctypedef void (*output_function)(block b, bitcoin_transaction t, bitcoin_transaction_output o)
  ctypedef void (*utxo_function)(block b, utxo u)

  void iterate(char *blockdir, char *cachedir,
               bool use_testnet,
               unsigned long block_start, unsigned long block_end,
               u8 *start_hash, u8 *tip,
               unsigned int utxo_period, bool use_mmap,
               unsigned progress_marks, bool quiet,
               char *blockfmt, char *txfmt, char *inputfmt, char *outputfmt, char *utxofmt,
               void (*blockfn)(block, bitcoin_block),
               void (*txfn)(block, bitcoin_transaction),
	       void (*inputfn)(block, bitcoin_transaction, bitcoin_transaction_input),
	       void (*outputfn)(block, bitcoin_transaction, bitcoin_transaction_output),
               void (*utxofn)(block, utxo))
