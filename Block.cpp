#include "Block.h"
#include <stdio.h>
#include "hash.h"
#include <string.h>
#include <iostream>
using namespace std;
    Block::Block() {}
    Block::Block(int fatherId, int blockId, char* data, int dataLength, Block* father):
            _id(blockId),
            _fatherId(fatherId),
            _father(father),
            _dataLength(dataLength),
            
            _isAttached(false),
            _cntSons(0)
    {

        if( father == NULL)
        {
            _depth = 0;
        }
        else {
            _depth = father->get_depth()+1;
        }
        char* tmp = new char[_dataLength];
        memcpy(tmp, data, _dataLength);
        _data = tmp;
    }
    Block::~Block(){
        if(_data) free (_data);


    }
    void Block::generate_data()
    {
        char* oldData = _data;
        _data = generate_hash(oldData, _dataLength, generate_nonce(_id,_fatherId));
        delete[] (oldData);
    }

    int Block::get_id()
    {
        return _id;
    }
    void Block::set_data(char* data)
    {
        _data = data;
    }
    int Block::get_depth()
    {
        return _depth;
    }
    void Block::set_to_longest()
    {
        _father = NULL;
        _fatherId = -1;
    }
    void Block::add_son() {
        _cntSons++;
    }
    void Block::dcr_son() {
        _cntSons--;
    }
