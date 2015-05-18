/**
*   ChainManager.cpp
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

#include "ChainManager.h"
#include "hash.h"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <climits>
#include <string.h>

//class consts.
#define GENESIS_FATHER_ID -1
#define CLOSE_CHAIN_WAS_NOT_CALLED -2
#define GENESIS_ID 0
#define GENESIS_LENGTH 8
#define GENESIS_DATA (char*)"genesis"
#define GENESIS_FPTR NULL
#define GENESIS_DEPTH 0
#define FAILURE -1
#define SUCCESS 0
#define NOT_EXIST -2
#define ALREADY_CHAINED 1
#define NO_SONS 0
#define LOCK(x)  if(pthread_mutex_lock(x) == EINVAL){cerr << "system error: Lock failed"<<endl; exit(1); }
#define UNLOCK(x) if(pthread_mutex_unlock(x) != 0){cerr << "system error: UnLock failed" <<endl; exit(1);}

/*
* Constructor.
*/
ChainManager::ChainManager() {
    _chainStatus = CLOSED;
}

/*
* DESCRIPTION: This function initiates the Block chain,
* and creates the genesis Block. The genesis Block does not
* hold any transaction data or hash.
* This function should be called prior to any other functions
* as a necessary precondition for their success (all other functions
* should return with an error otherwise).
* RETURN VALUE: On success 0, otherwise -1.
*/
int ChainManager::init_blockchain() {
    generalMutex = PTHREAD_MUTEX_INITIALIZER;
    try {      
        if(_chainStatus == CLOSED) {
            LOCK(&generalMutex);
            srand(time(0));
            init_hash_generator();
            //inits all mutexs.
            pthread_mutex_init(&daemonMutex, NULL);
            pthread_cond_init(&waitingListCond, NULL);
            pthread_cond_init(&pruneCloseCond, NULL);
            pthread_mutex_init(&_waitingListMutex, NULL);
            pthread_mutex_init(&_leafsMutex, NULL);
            pthread_mutex_init(&_deepestLeafsMutex, NULL);
            pthread_mutex_init(&_allBlocksMutex, NULL);
            pthread_mutex_init(&_blockIdsMutex, NULL);
            _genesis = new Block(GENESIS_FATHER_ID, GENESIS_ID, GENESIS_DATA, GENESIS_LENGTH, GENESIS_FPTR);
            LOCK(&_allBlocksMutex);
            _allBlocks[GENESIS_ID] = _genesis;
            UNLOCK(&_allBlocksMutex);
            _chainSize = 0;
            _deepestDepth = 0;
            _highestId = 0;
            LOCK(&_deepestLeafsMutex);
            _deepestLeafs.push_back(GENESIS_ID);
            UNLOCK(&_deepestLeafsMutex);
            LOCK(&_leafsMutex);
            _leafs.push_back(GENESIS_ID);
            UNLOCK(&_leafsMutex);
            _chainStatus = INITIALIZED;
            UNLOCK(&generalMutex);
            pthread_create(&daemonTrd, NULL, daemonThread, this);
        }
        else {
            perror("init_blockchain called twice");
            return FAILURE;
        }

    }
    catch(exception& e) {
        cout<<"INIT EXCEPTION" << endl;
        UNLOCK(&generalMutex);
        return FAILURE;
    }

    return SUCCESS;
}

//returns arbitrary longest chains leaf (if there is more than one).
Block* ChainManager::get_father_rand() {
    LOCK(&_deepestLeafsMutex);
    int randIndex = rand()%(_deepestLeafs.size());
    UNLOCK(&_deepestLeafsMutex);
    LOCK(&_allBlocksMutex);
    Block* tmp = _allBlocks[_deepestLeafs[randIndex]];
    UNLOCK(&_allBlocksMutex);
    return tmp;

}
//the function returns the lowest available block_num (> 0) 
int ChainManager::get_new_id(){
    LOCK(&_blockIdsMutex);
    int newId = _highestId + 1;
    if(!_blockIds.empty()) {
        newId = _blockIds.top();
        _blockIds.pop();
    }
    else {
        _highestId = newId;
    }
    UNLOCK(&_blockIdsMutex);
    return newId;
}

/*
* DESCRIPTION: Ultimately, the function adds the hash of the data to the Block chain.
* Since this is a non-blocking package, your implemented method should return as
* soon as possible, even before the Block was actually attached
* to the chain.
* Furthermore, the father Block should be determined before this function returns.
* The father Block should be the last Block of the current
* longest chain (arbitrary longest chain if there is more than one).
* Notice that once this call returns, the original data may be freed by the caller.
* RETURN VALUE: On success, the function returns the lowest available block_num (> 0),
* which is assigned from now on to this individual piece of data.
* On failure, -1 will be returned.
*/
int ChainManager::add_block(char *data, size_t length) {
    if(_highestId < INT_MAX && _chainStatus == INITIALIZED) {
        int new_id = get_new_id();
        Block* father = get_father_rand();
        Block* newBlock = new Block(father->_id, new_id, data, length, father);
        LOCK(&_waitingListMutex);
        _waitingList.push_back(newBlock->_id);
        UNLOCK(&_waitingListMutex);
        LOCK(&_allBlocksMutex);
        _allBlocks[new_id] = newBlock;
        UNLOCK(&_allBlocksMutex);
        LOCK(&daemonMutex);
        //signal to daemon to get out from the waiting status.
        pthread_cond_signal(&waitingListCond);
        UNLOCK(&daemonMutex);
        return new_id;
    }
    return FAILURE;
}

 /*
* DESCRIPTION: Without blocking, enforce the policy that this block_num
* should be attached to the longest chain at the time of attachment of
* the Block. For clearance, this is opposed to the original add_block 
* that adds the Block to the longest chain during the time that add_block
* was called.
* The block_num is the assigned value that was previously returned by add_block.
* RETURN VALUE: If block_num doesn't exist, return -2; In case of other errors,
* return -1; In case of success return 0; In case block_num is
* already attached return 1.
*/
int ChainManager::to_longest(int block_num) {
    int retVal;
    if(_chainStatus != INITIALIZED) return FAILURE;
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    //in case block_num doesn't exist.
    if(findBlock == _allBlocks.end()) retVal = NOT_EXIST;
    // handle block exist and found
    if(!findBlock->second->_isAttached) {
        findBlock->second->set_to_longest();
        retVal= SUCCESS;
    }
    // in case of already attached.
    else if(_allBlocks[block_num]->_isAttached) {
        retVal= ALREADY_CHAINED;
    }
    UNLOCK(&_allBlocksMutex);
    return retVal;

}
/*
 * DESCRIPTION: This function blocks all other Block attachments,
 * until block_num is added to the chain. The block_num is the assigned value
 * that was previously returned by add_block.
 * RETURN VALUE: If block_num doesn't exist, return -2;
 * In case of other errors, return -1; In case of success or if
 * it is already attached return 0.
 */
int ChainManager::attach_now(int block_num) {
    int returnVal = SUCCESS;
    if(_chainStatus !=  INITIALIZED) return FAILURE;
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    UNLOCK(&_allBlocksMutex);
    // block_num doesn't exist
    if(findBlock == _allBlocks.end()) returnVal = NOT_EXIST;
    else if(findBlock->second->_isAttached) returnVal = SUCCESS;
    // not yet attached
    else {
        LOCK(&_waitingListMutex);
        _waitingList.remove(block_num);
        UNLOCK(&_waitingListMutex);
        LOCK(&generalMutex);
        // chaining the block. 
        chain_block(findBlock->second);
        UNLOCK(&generalMutex);
    }
    return returnVal;
}

 /*
* DESCRIPTION: Without blocking, check whether block_num was added to the chain.
* The block_num is the assigned value that was previously returned by add_block.
* RETURN VALUE: 1 if true and 0 if false. If the block_num doesn't exist, return -2;
* In case of other errors, return -1.
*/
int ChainManager::was_added(int block_num) {
    int retVal = SUCCESS;
    if(_chainStatus != INITIALIZED) return FAILURE;
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    UNLOCK(&_allBlocksMutex);
    // block_num doesn't exist
    if(findBlock == _allBlocks.end()) retVal =  NOT_EXIST;
    // in case of already attached.
    if(findBlock->second->_isAttached) retVal =  ALREADY_CHAINED;
    return retVal;
}

/*
* DESCRIPTION: Return how many Blocks were attached to the chain since init_blockchain.
* If the chain was closed (by using close_chain) and then initialized (init_blockchain)
* again this function should return the new chain size.
* RETURN VALUE: On success, the number of Blocks, otherwise -1.
*/
int ChainManager::chain_size() {
    if(_chainStatus == CLOSED ) return FAILURE;
    return _chainSize;
}

//removes block_num from _leafs list.
void ChainManager::remove_leaf(int block_num)
{
    _leafs.erase(remove(_leafs.begin(), _leafs.end(), block_num), _leafs.end());
}

//removes block_num from _deepestLeafs list.
void ChainManager::remove_deepestLeaf(int block_num)
{
    LOCK(&_deepestLeafsMutex);
    _deepestLeafs.erase(remove(_deepestLeafs.begin(), _deepestLeafs.end(), block_num), _deepestLeafs.end());
    UNLOCK(&_deepestLeafsMutex);
}

// this thread manages the asynchronous data addition operation.
void* ChainManager::daemonThread(void* ptr) {
    ChainManager* chain = (ChainManager*) ptr;
    //running while _chainStatus flag is INITIALIZED
    while(chain->_chainStatus == INITIALIZED) {
        LOCK(&chain->daemonMutex);
        //in case _waitingList is empty the daemon thread will wait
        //untile add_block will add new block to _waitingList.
        if(chain->_waitingList.empty()) {
            if(pthread_cond_wait(&chain->waitingListCond, &chain->daemonMutex)) {
                exit(FAILURE);
            }
        }
        // in case there are some elements in _waitingList.
        else {
            LOCK(&chain->_waitingListMutex);
            LOCK(&chain->_allBlocksMutex);
            Block* toAttach = chain->_allBlocks[chain->_waitingList.front()];
            UNLOCK(&chain->_allBlocksMutex);
            chain->_waitingList.pop_front();
            UNLOCK(&chain->_waitingListMutex);
            LOCK(&chain->generalMutex);
            // chaining the block. 
            chain->chain_block(toAttach);
            UNLOCK(&chain->generalMutex);
        }
        UNLOCK(&chain->daemonMutex);
    }
    pthread_exit(NULL);
    return NULL;
}

//this function actualy dose the chaining.
//returen value: 0 on success. on failure exit and return 1.
int ChainManager::chain_block(Block* toChain) {
    toChain->_isAttached = true;
    //in case to_longest was called.
    if(toChain->_father == NULL) {
        Block* father = get_father_rand();
        toChain->_father = father;
        toChain->_depth = father->_depth + 1;
    }
    if(toChain->_father == NULL) {
        exit(1);
    }
    toChain->generate_data();
    toChain->_father->add_son();
    // deeper depth
    if(toChain->_depth > _deepestDepth) { 
        LOCK(&_deepestLeafsMutex);
        _deepestLeafs.clear();
        _deepestLeafs.push_back(toChain->_id);
        UNLOCK(&_deepestLeafsMutex);
        LOCK(&_leafsMutex);
        _leafs.push_back(toChain->_id);
        UNLOCK(&_leafsMutex);
        _deepestDepth = toChain->_depth;
    }
    // new leaf but not deepest
    else if (toChain->_depth == _deepestDepth){ 
        LOCK(&_leafsMutex);
        _leafs.push_back(toChain->_id);
        UNLOCK(&_leafsMutex);
        LOCK(&_deepestLeafsMutex);
        _deepestLeafs.push_back(toChain->_id);
        UNLOCK(&_deepestLeafsMutex);
    }
    else {
        LOCK(&_leafsMutex);
        _leafs.push_back(toChain->_id);
        UNLOCK(&_leafsMutex);
    }
    // removes the father only if he have one son.
    if(toChain->_father->_cntSons == 1) {
        LOCK(&_leafsMutex);
        remove_leaf(toChain->_fatherId);
        UNLOCK(&_leafsMutex);
    }
    _chainSize++;
    return SUCCESS;
}

/*
* DESCRIPTION: Search throughout the tree for sub-chains
* that are not the longest chain,
* detach them from the tree, free the blocks, and reuse the block_nums.
* RETURN VALUE: On success 0, otherwise -1.
*/
int ChainManager::prune_chain() {

    LOCK(&generalMutex);
    Block* tmpBlock;
    Block* findBlock;
    vector<int> allMinDepths;
    //in case init_blockchain was not called.
    if(_chainStatus != INITIALIZED) return FAILURE;
    LOCK(&_leafsMutex);
    int leafsSize = _leafs.size();
    UNLOCK(&_leafsMutex);
    LOCK(&_deepestLeafsMutex);
    int deepestId = _deepestLeafs[rand()%(_deepestLeafs.size())];
    UNLOCK(&_deepestLeafsMutex);
    // to find all the leafs with min depth
    for(int i = 0; i < leafsSize; i++) {
        LOCK(&_allBlocksMutex);
        findBlock = _allBlocks.find(_leafs[i])->second;
        UNLOCK(&_allBlocksMutex);
        // remove all blocks till sub-root
        if(findBlock->_id != deepestId)
         {

            while(findBlock->_id != GENESIS_ID && findBlock->_cntSons == NO_SONS)
            {
                tmpBlock = findBlock->_father;
                tmpBlock->dcr_son();
                LOCK(&_allBlocksMutex);
                int id = findBlock->_id;
                _allBlocks.erase(findBlock->_id);
                delete (findBlock);
                UNLOCK(&_allBlocksMutex);
                LOCK(&_blockIdsMutex);
                //relesing unused id;
                _blockIds.push(id);
                UNLOCK(&_blockIdsMutex);
                findBlock = tmpBlock;
            }
            //removes from _deepestLeaf list. 
            remove_deepestLeaf(_leafs[i]);
            LOCK(&_leafsMutex);
            //removes from _leafs list.
            remove_leaf(_leafs[i]);
            UNLOCK(&_leafsMutex);
            leafsSize--;
        }
    }
    delete(tmpBlock);
    UNLOCK(&generalMutex);
    return SUCCESS;
}

/*
 * DESCRIPTION: Close the recent blockchain and reset the system, so that
 * it is possible to call init_blockchain again. Non-blocking.
 * All pending Blocks should be hashed and printed to terminal (stdout).
 * Calls to library methods which try to alter the state of the Blockchain
 * are prohibited while closing the Blockchain.
 * e.g.: Calling chain_size() is ok, a call to prune_chain() should fail.
 * In case of a system error, the function should cause the process to exit.
 */
void ChainManager::close_chain() {
    _chainStatus = CLOSING;
    pthread_create(&closingTrd, NULL, close_chain_helper, this);
}

//deleting all blocks from the chain and destroys all used mutexs.
//printing all hashed blocsk in _waitingList. 
void* ChainManager::close_chain_helper(void* ptr) {
    ChainManager* chain = (ChainManager*) ptr;
    LOCK(&chain->generalMutex);
    LOCK(&chain->_allBlocksMutex);
    LOCK(&chain->_waitingListMutex);
    //hashed blocsk in _waitingList and printing the hash code.
    for(int block:chain->_waitingList) {
        chain->_allBlocks[block]->generate_data();
        PRINT(chain->_allBlocks[block]->_data);
    }
    //deleting all block in chain.
    for(auto it = chain->_allBlocks.begin(); it != chain->_allBlocks.end(); ++it){
        if(it->second->_id != GENESIS_ID) {
          delete (it->second);
        }
    }

    UNLOCK(&chain->_waitingListMutex);
    chain->_allBlocks.erase(chain->_allBlocks.begin());
    UNLOCK(&chain->_allBlocksMutex);

    LOCK(&chain->_waitingListMutex);
    chain->_waitingList.clear();
    UNLOCK(&chain->_waitingListMutex);

    LOCK(&chain->_leafsMutex);
    chain->_leafs.clear();
    UNLOCK(&chain->_leafsMutex);

    LOCK(&chain->_deepestLeafsMutex);
    chain->_deepestLeafs.clear();
    UNLOCK(&chain->_deepestLeafsMutex);

    LOCK(&chain->_blockIdsMutex);
    //relesing unused id;
    while(!chain->_blockIds.empty()) {
        chain->_blockIds.pop();
    }
    UNLOCK(&chain->_blockIdsMutex);

    //destroying all mutexs and conditions
    close_hash_generator();
    UNLOCK(&chain->generalMutex);
    pthread_mutex_destroy(&chain->_waitingListMutex);
    pthread_mutex_destroy(&chain->_leafsMutex);
    pthread_mutex_destroy(&chain->_deepestLeafsMutex);
    pthread_mutex_destroy(&chain->_allBlocksMutex);
    pthread_mutex_destroy(&chain->_blockIdsMutex);
    pthread_mutex_destroy(&chain->daemonMutex);
    pthread_cond_destroy(&chain->waitingListCond);
    chain->_chainStatus = CLOSED;
    pthread_exit(NULL);
}

/*
* DESCRIPTION: The function blocks and waits for close_chain to finish.
* RETURN VALUE: If closing was successful, it returns 0.
* If close_chain was not called it should return -2. In case of other error, it should return -1.
*/
int ChainManager::return_on_close() {
    //in case init_blockchain was not called.
    if(_chainStatus != CLOSING) return CLOSE_CHAIN_WAS_NOT_CALLED;
    pthread_join(closingTrd, NULL);
    return SUCCESS;
}