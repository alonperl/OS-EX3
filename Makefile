all: libblockchain.a

tar: libblockchain.a
	tar -cvf ex3.tar blockchain.cpp Block.h Block.cpp \
	ChainManager.cpp ChainManager.h libblockchain.a README Makefile

libblockchain.a: ChainManager.o Block.o blockchain.o
	ar rcs libblockchain.a ChainManager.o Block.o blockchain.o libblockchain.a

ChainManager.o: ChainManager.cpp ChainManager.h Block.h
	g++ -Wall -Wextra -Wvla -std=c++11 -c ChainManager.cpp

Block.o: Block.h Block.cpp
	g++ -Wall -Wextra -Wvla -std=c++11 -c Block.cpp

blockchain.o: blockchain.h blockchain.cpp ChainManager.h Block.h
	g++ -Wall -Wextra -Wvla -std=c++11 -c blockchain.cpp -L. -lhash -lcrypto -lpthread

clean:
	rm -f *.o libblockchain.a

.PHONY: clean tar all
