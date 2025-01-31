CFLAGS  = -I ../lib $(CFLAGSIN)

PROGS = hash_bu hash_marc

all: $(PROGS)

hash_marc: hash_marc.o ../lib/hash.o uintlist.o
	$(CC) $(CFLAGS) -o $@ $^

hash_bu: hash_bu.o ../lib/hash.o uintlist.o
	$(CC) $(CFLAGS) -o $@ $^

clean: 
	rm -f *.o core

realclean:
	rm -f *.o *.a core $(PROGS)

install:

test:
