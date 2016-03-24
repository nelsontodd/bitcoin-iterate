print "Importing * from 'iterate'..."
from iterate import *
print "Current namespace:"
things = dir()
things.sort()
print things

def print_block(block, header):
    print "BLOCK %i %i" % (block.height, header.length)

iterate(blockdir="/data/bitcoind/blocks", cachedir="/data/bitcoind/iterate-cache",
        end=10,
        blockfn=print_block)
