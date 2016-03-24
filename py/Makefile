PYTHON_INCLUDES := -I /usr/include/python2.7
CCAN_INCLUDES := -I ../ccan
BITCOIN_ITERATE_INCLUDES := -I ..
CFLAGS := -shared -pthread -fPIC -fwrapv -O2 -Wall -fno-strict-aliasing
CYTHON := cython

all: iterate.so

iterate.so: iterate.c
	$(CC) $(CFLAGS) $(PYTHON_INCLUDES) $(CCAN_INCLUDES) $(BITCOIN_ITERATE_INCLUDES) -o iterate.so iterate.c

iterate.c: iterate.pyx
	$(CYTHON) -a iterate.pyx

clean:
	$(RM) iterate.c iterate.html iterate.so
	$(RM) -r build
