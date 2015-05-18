/**
*   ChainManager.h
*   __________________________________________________________________________
*   A multi threaded blockchain database manager
*   Functions:
*       init_blockchain - This function initiates the Block chain,
*       and creates the genesis Block. The genesis Block does not
*       hold any transaction data or hash.
*
*       add_block - Ultimately, the function adds the hash of 
*       the data to the Block chain.
*
*       to_longest - enforce the policy that this block_num
*       should be attached to the longest chain at the time of attachment of
*       the Block.
*
*       attach_now - This function blocks all other Block attachments,
*       until block_num is added to the chain.
*
*       was_added - check whether block_num was added to the chain.
*    
*       chain_size - Return how many Blocks were attached to the
*       chain since init_blockchain.
*
*       prune_chain - Search throughout the tree for sub-chains
*       that are not the longest chain,
*       detach them from the tree, free the blocks, and reuse the block_nums.
*
*       close_chain - Close the recent blockchain and reset the system, so that
*       it is possible to call init_blockchain again.
*
*       return_on_close - The function blocks and waits for close_chain to finish.
*
*       chain_block - this function dose the actual chaining.
*
*       remove_leaf - removes block_num from _leafs list.
*
*       remove_deepestLeaf - removes block_num from _deepestLeafs list.
*
*       close_chain_helper - deleting all blocks from the chain
*       and destroys all used mutexs and printing all hashed blocsk in _waitingList. 
*
*       get_father_rand - returns arbitrary longest chains
*       leaf (if there is more than one).
*
*       ChainManager - constructor.
*       
*       get_new_id - the function returns the lowest available block_num (> 0) 
*       
*   Created by: Vladi Kravtsov, Alon Perelmuter.
*/


#ifndef EX3_CHAINMANAGER_H
#define EX3_CHAINMANAGER_H
#include "Block.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <queue>
#include <pthread.h>
#include <iostream>
#define PRINT(x) cout<<x<<endl;

using namespace std;
enum  Status { INITIALIZED, CLOSING, CLOSED};
class ChainManager
{

public:
    //class mutexs.
    Status _chainStatus;
    pthread_mutex_t daemonMutex;
    pthread_mutex_t generalMutex;
    pthread_mutex_t _deepestLeafsMutex;
    pthread_mutex_t _leafsMutex;
    pthread_mutex_t _waitingListMutex;
    pthread_mutex_t _allBlocksMutex;
    pthread_mutex_t _blockIdsMutex;
    //class threads
    pthread_t daemonTrd;
    pthread_t closingTrd;
    //class conditions
    pthread_cond_t waitingListCond;
    pthread_cond_t pruneCloseCond;
    
    //class data members.
    int _highestId;
    bool _closeChain;
    bool _isInit;
    int _deepestDepth;
    unsigned int _chainSize;
    list<int> _waitingList;
    vector<int> _leafs;
    vector<int> _deepestLeafs;
    priority_queue<int, vector<int>, greater<int>> _blockIds;
    unordered_map<unsigned int, Block*> _allBlocks;
    Block* _genesis;
    
    /*
    * Class constructor.
    */
    ChainManager();
    /*
    * DESCRIPTION: This function initiates the Block chain, and creates the genesis Block. The genesis Block does not hold any transaction data
    * or hash.
    * This function should be called prior to any other functions as a necessary precondition for their success (all other functions should return
    * with an error otherwise).
    * RETURN VALUE: On success 0, otherwise -1.
    */
    int init_blockchain();
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
    int add_block(char *data , size_t length);
    /*
    * DESCRIPTION: Without blocking, enforce the policy that this block_num should be attached to the longest chain at the time of attachment of
    * the Block. For clearance, this is opposed to the original add_block that adds the Block to the longest chain during the time that add_block
    * was called.
    * The block_num is the assigned value that was previously returned by add_block.
    * RETURN VALUE: If block_num doesn't exist, return -2; In case of other errors, return -1; In case of success return 0; In case block_num is
    * already attached return 1.
    */
    int to_longest(int block_num);
    /*
    * DESCRIPTION: This function blocks all other Block attachments, until block_num is added to the chain. The block_num is the assigned value
    * that was previously returned by add_block.
    * RETURN VALUE: If block_num doesn't exist, return -2;
    * In case of other errors, return -1; In case of success or if it is already attached return 0.
    */
    int attach_now(int block_num);
    /*
    * DESCRIPTION: Without blocking, check whether block_num was added to the chain.
    * The block_num is the assigned value that was previously returned by add_block.
    * RETURN VALUE: 1 if true and 0 if false. If the block_num doesn't exist, return -2; In case of other errors, return -1.
    */
    int was_added(int block_num);
    /*
    * DESCRIPTION: Return how many Blocks were attached to the chain since init_blockchain.
    * If the chain was closed (by using close_chain) and then initialized (init_blockchain) again this function should return the new chain size.
    * RETURN VALUE: On success, the number of Blocks, otherwise -1.
    */
    int chain_size();
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

    // this thread manages the asynchronous data addition operation.
    static void* daemonThread(void* ptr);

    //this function actualy dose the chaining.
    //returen value: 0 on success. on failure exit and return 1.
    int chain_block(Block* toChain);

    //removes block_num from _leafs list.
    void remove_leaf(int block_num);

    //removes block_num from _deepestLeafs list.
    void remove_deepestLeaf(int block_num);

    //deleting all blocks from the chain and destroys all used mutexs.
    //printing all hashed blocsk in _waitingList.
    static void* close_chain_helper(void* ptr);

    //returns arbitrary longest chains leaf (if there is more than one).
    Block* get_father_rand();

    //the function returns the lowest available block_num (> 0) 
    int get_new_id();

private:

};
#endif //EX3_CHAINMANAGER_H
