CC=g++
RANLIB=ranlib

LIBSRC=./blockchain.cpp ./ChainBlock.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)


INCS=-I.
CFLAGS = -Wall -g -std=c++11 $(INCS)
CPPFLAGS = -Wall -g -std=c++11
LOADLIBES = -L./

COMPILE.cc=$(CC) $(CPPFLAGS) -c

OSMLIB = libblockchain.a
TARGETS = $(OSMLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) Makefile README

all: $(TARGETS) test

$(LIBOBJ): .$(LIBSRC) .$(LIBSRC:.cpp=.h) ../hash.h
	$(COMPILE.cc) $% $<

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
	
test1.o: ../blockchain.h ../hash.h ./test1.cpp
	$(CC) -c ./test1.cpp
	
test1: $(TARGETS) ./test1.o
	$(CC) ./test1.o $(TARGETS) -lhash  -lpthread -L./ -lcrypto -o blockchain
	./blockchain
	
test: test1

.PHONY: $(TARGETS) $(LIBOBJ) test test1

val: 
	valgrind --tool=memcheck --leak-check=full ./blockchain 

