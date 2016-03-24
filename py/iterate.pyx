cimport citerate

OP_PUSHDATA1   = 0x4C
OP_PUSHDATA2   = 0x4D
OP_PUSHDATA4   = 0x4E
OP_NOP         = 0x61
OP_RETURN      = 0x6A
OP_DUP         = 0x76
OP_EQUALVERIFY = 0x88
OP_CHECKSIG    = 0xAC
OP_HASH160     = 0xA9

UNKNOWN_OUTPUT = 0
PAYMENT_OUTPUT = 1
CHANGE_OUTPUT  = 2

cdef class Block:
  cdef citerate.block* _block

  def __cinit__(self):
    self._block = citerate.new_block()

  def __dealloc__(self):
    citerate.free_block(self._block)

  cdef int height(self):
      return <int>self._block.height

cdef class BlockHeader:
  cdef citerate.bitcoin_block* _bitcoin_block

  def __cinit__(self):
    self._bitcoin_block = citerate.new_bitcoin_block()

  def __dealloc__(self):
    citerate.free_bitcoin_block(self._bitcoin_block)

  cdef int length(self):
      return <int>self._bitcoin_block.len
