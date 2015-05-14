#ifndef _BLOCK_HPP
#define _BLOCK_HPP
#include <stdio.h>
#include "hash.h"

class Block {
public:

    int	_fatherId;
	int _id;
	char* _data;
	int _dataLength;
	int _depth;
	Block* _father;
	bool _to_longest;
	bool _isAttached;
	unsigned int _cntSons;
	Block() {}
	Block(int fatherId, int blockId, char* data, int dataLength, Block* father):
			_fatherId(fatherId),
			_id(blockId),
			_data(data),
			_dataLength(dataLength),
			_father(father),
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
    }
	~Block(){
		delete (_data);

	}
	void generate_data()
	{
		char* oldData = _data;
		_data = generate_hash(_data, _dataLength, generate_nonce(_id,_fatherId));
		delete oldData;
//		delete _data;
	}

	int get_id()
	{
		return _id;
	}
	void set_data(char* data)
	{
		_data = data;
	}
	int get_depth()
	{
		return _depth;
	}
	void set_to_longest()
    {
		_father = NULL;
		_fatherId = -1;
	}
	void add_son() {
		_cntSons++;
	}
	void dcr_son() {
		_cntSons--;
	}
};




#endif