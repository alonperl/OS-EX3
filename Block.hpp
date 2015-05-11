#ifndef _BLOCK_HPP
#define _BLOCK_HPP

Class Block {
	int _fatherId;
	int _id;
	char* _data;
	int _dataLength;
	int _depth;
	Block* _father;
	boolean _to_longest;

	Block(int fatherId, int blockId, char* data, int dataLength, Block* father):
			_fatherId = fatherId,
			_blockId = blockId,
			_data = data,
			_dataLength = dataLength,
			_father = father ; {
				if( father == NULL)
				{
					_depth = 0;
				}
				else {
					_depth = father->get_depth()++;
				}
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

}

void set_to_longest() {
	_to_longest = true;
}


#endif