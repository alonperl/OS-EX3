
/*
 *       A multi threaded blockchain database manager
 *       Author: OS, os@cs.huji.ac.il
 */
#include "blockchain.h"
#include "Block.hpp"
#include "hash.h"
#include <unordered_map>
#include <list>

using namespace std;

#define GENESIS_FATHER_ID -1
#define GENESIS_ID 0
#define GENESIS_LENGTH 0
#define GENESIS_DATA NULL
#define GENESIS_PTR NULL
#define GENESIS_DEPTH 0
#define FAILURE -1

private boolean _closeChain;
private boolean _isInit;
private unsigned int _chainSize;
private Block _genesis;
private list<Block*> _waitingList;
private vector<Block*> _leafs;
private priority_queue<int, vector<int>, greater<int>> _blockNums;
private unordered_map<unsigned int, Block*> _allBlocks;


/*
 * DESCRIPTION: This function initiates the Block chain, and creates the genesis Block.  The genesis Block does not hold any transaction data 
 * or hash.
 * This function should be called prior to any other functions as a necessary precondition for their success (all other functions should return  
 * with an error otherwise).
 * RETURN VALUE: On success 0, otherwise -1.
 */
int init_blockchain() {
	_isInit = true;
	_closeChain = false;
	init_hash_generator();
	this._waitingList = new linkedList<Block*>();
	this._genesis = new Block(GENESIS_FATHER_ID, GENESIS_ID, GENESIS_DATA, GENESIS_LENGTH, GENESIS_PTR);
	this._leafs = new vector<Block>();
	_allBlocks[GENESIS_ID] = &newBlock;
	_chainSize = 1;
	_leafs.insert(&genesis);
	for(int i = 1; i < INT_MAX; i++) {
		_blockNums.push(i);
	}
}

/*
 * DESCRIPTION: Ultimately, the function adds the hash of the data to the Block chain.
 * Since this is a non-blocking package, your implemented method should return as soon as possible, even before the Block was actually attached 
 * to the chain.
 * Furthermore, the father Block should be determined before this function returns. The father Block should be the last Block of the current    
 * longest chain (arbitrary longest chain if there is more than one).
 * Notice that once this call returns, the original data may be freed by the caller.
 * RETURN VALUE: On success, the function returns the lowest available block_num (> 0),
 * which is assigned from now on to this individual piece of data.
 * On failure, -1 will be returned.
 */

int add_block(char *data , size_t length) {

	if(!_blockNums.empty() && _isInit) {
		int father_id = get_father_rand_id();
		int new_id_ = _blockNums.pop();
		Block* newBlock = new Block(father_id, new_id, data, length, _leafs[father_id]);
 		_waitingList.push_back(newBlock);
 		_allBlocks[new_id] = newBlock;
 		return new_id;
	}

 	return FAILURE;

}

//returns arbitrary longest chains leaf id (if there is more than one).
private int get_father_rand_id() {
	return _leafs[(rand()%(_leafs.size())]->get_id();
}

/*
 * DESCRIPTION: Without blocking, enforce the policy that this block_num should be attached to the longest chain at the time of attachment of 
 * the Block. For clearance, this is opposed to the original add_block that adds the Block to the longest chain during the time that add_block    
 * was called.
 * The block_num is the assigned value that was previously returned by add_block. 
 * RETURN VALUE: If block_num doesn't exist, return -2; In case of other errors, return -1; In case of success return 0; In case block_num is 
 * already attached return 1.
 */

int to_longest(int block_num) {
	if(_isInit) {
		if(_isInit && _allBlocks[block_num]) {
			if(_allBlocks[block_num]->_stat == WAITING) {
				_allBlocks[block_num]->set_to_longest();
				return 0;
			}
			else if(_allBlocks[block_num]->_stat == CHAINED) {
				return 1;
			}
		}
		else if(_allBlocks[block_num] == NULL) {
			return -2;
		}	
	}	

	return -1;
}

/*
 * DESCRIPTION: This function blocks all other Block attachments, until block_num is added to the chain. The block_num is the assigned value 
 * that was previously returned by add_block.
 * RETURN VALUE: If block_num doesn't exist, return -2;
 * In case of other errors, return -1; In case of success or if it is already attached return 0.
 */
  
int attach_now(int block_num) {

}

/*
 * DESCRIPTION: Without blocking, check whether block_num was added to the chain.
 * The block_num is the assigned value that was previously returned by add_block.
 * RETURN VALUE: 1 if true and 0 if false. If the block_num doesn't exist, return -2; In case of other errors, return -1.
 */
  
int was_added(int block_num) {
	if(_isInit) {
		if(_allBlocks[block_num]) {
			if(_allBlocks[block_num]->_stat == CHAINED) {
				return 1;
			}
			return 0;
		}
		return -2;
	}
	return -1;
}

/*
 * DESCRIPTION: Return how many Blocks were attached to the chain since init_blockchain.
 * If the chain was closed (by using close_chain) and then initialized (init_blockchain) again this function should return the new chain size.
 * RETURN VALUE: On success, the number of Blocks, otherwise -1.
 */

int chain_size() {
	if(_isInit) {
		return _chainSize;
	}
	return -1;
}

/*
 * DESCRIPTION: Search throughout the tree for sub-chains that are not the longest chain,
 * detach them from the tree, free the blocks, and reuse the block_nums.
 * RETURN VALUE: On success 0, otherwise -1.
 */
 
 int prune_chain();

/*
 * DESCRIPTION: Close the recent blockchain and reset the system, so that it is possible to call init_blockchain again. Non-blocking.
 * All pending Blocks should be hashed and printed to terminal (stdout).
 * Calls to library methods which try to alter the state of the Blockchain are prohibited while closing the Blockchain. 
 * e.g.: Calling chain_size() is ok, a call to prune_chain() should fail.
 * In case of a system error, the function should cause the process to exit.
 */
 
void close_chain();

/*
 * DESCRIPTION: The function blocks and waits for close_chain to finish.
 * RETURN VALUE: If closing was successful, it returns 0.
 * If close_chain was not called it should return -2. In case of other error, it should return -1.
 */

int return_on_close();

#endif
