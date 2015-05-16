//
// Created by alonperl on 5/13/15.
//

#include "ChainManager.h"
#include "hash.h"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <climits>
#include <string.h>
//#include <pthread.h>
#define GENESIS_FATHER_ID -1
#define GENESIS_ID 0
#define GENESIS_LENGTH 0
#define GENESIS_DATA NULL
#define GENESIS_FPTR NULL
#define GENESIS_DEPTH 0
#define FAILURE -1
#define SUCCESS 0
#define NOT_EXIST -2
#define ALREADY_CHAINED 1
#define NO_SONS 0
#define LOCK(x)  if(pthread_mutex_lock(x) == EINVAL){cerr << "system error: Lock failed"<<endl; exit(1); }
#define UNLOCK(x) if(pthread_mutex_unlock(x) != 0){cerr << "system error: UnLock failed" <<endl; exit(1);}

ChainManager::ChainManager() {




}
int ChainManager::init_blockchain() {
    generalMutex = PTHREAD_MUTEX_INITIALIZER;
    try {
        LOCK(&generalMutex);
//        cout << "init block chain manager" << endl;
        if(!_isInit) {

            srand(time(0));
            init_hash_generator();

            pthread_mutex_init(&daemonMutex, NULL);
            pthread_cond_init(&waitingListCond, NULL);

            pthread_mutex_init(&_waitingListMutex, NULL);
            pthread_mutex_init(&_leafsMutex, NULL);
            pthread_mutex_init(&_deepestLeafsMutex, NULL);
            pthread_mutex_init(&_allBlocksMutex, NULL);
            pthread_mutex_init(&_blockIdsMutex, NULL);

//            _waitingList = new list<int>();

            _genesis = new Block(GENESIS_FATHER_ID, GENESIS_ID, GENESIS_DATA, GENESIS_LENGTH, GENESIS_FPTR);

//            this._deepestLeafs = new vector<int>();
//            this->_leafs = new vector<int>();
//            this->_blockIds = new  priority_queue<int, vector<int>, greater<int>>();

            _allBlocks[GENESIS_ID] = _genesis;
            _chainSize = 0;
            _deepestDepth = 0;
            _highestId = 0;
            _deepestLeafs.push_back(GENESIS_ID);
            _leafs.push_back(GENESIS_ID);
//            PRINT("before for");

//            for(int i = 1; i < INT_; i++) {
//                _blockIds.push(i);
//            }
            _closeChain = false;

            //TODO : check if create of, else print error
//            cout << "starting daemon" << endl;

            _isInit = true;
            UNLOCK(&generalMutex);
            pthread_create(&daemonTrd, NULL, daemonThread, this);

        }
        else {
            perror("init_blockchain called twice");
            UNLOCK(&generalMutex);
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
    int randIndex = rand()%(_deepestLeafs.size());
//    PRINT(randIndex);
//    for(int i = 0; i< _deepestLeafs.size(); i++)
//        cout << _deepestLeafs[i] <<" --> ";
//    cout << endl;
    return _allBlocks[_deepestLeafs[randIndex]];
}
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

int ChainManager::add_block(char *data, size_t length) {
    //TODO: check if chain was closed

    if(_highestId < INT_MAX && _isInit) {
        LOCK(&generalMutex);
//        PRINT("sdasd");
        // TODO : add mutex protection


//        if(father == NULL) PRINT("FATHER IS NULL")
        LOCK(&_waitingListMutex);
        LOCK(&_allBlocksMutex);
        int new_id = get_new_id();

//        LOCK(&generalMutex);
        Block* father = get_father_rand();
//        UNLOCK(&generalMutex);

        char* tmp = new char[length];
        memcpy(tmp, data, length);
        Block* newBlock = new Block(father->_id, new_id, tmp, length, father);


        _waitingList.push_back(newBlock->_id);
        _allBlocks[new_id] = newBlock;
        UNLOCK(&_allBlocksMutex);
        UNLOCK(&_waitingListMutex);
        if(_waitingList.size() == 1) {
            LOCK(&daemonMutex);
            pthread_cond_signal(&waitingListCond);
            UNLOCK(&daemonMutex);
        }



//      PRINT("add block " << new_id)
        UNLOCK(&generalMutex);
        return new_id;
    }
    return FAILURE;

}

int ChainManager::to_longest(int block_num) {
    int retVal;
    if(!_isInit) return FAILURE;
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    if(findBlock == _allBlocks.end()) retVal = NOT_EXIST;
    // handle block exist and found
    if(!findBlock->second->_isAttached) {
        findBlock->second->set_to_longest();
        retVal= SUCCESS;
    }
    else if(_allBlocks[block_num]->_isAttached) {
        retVal= ALREADY_CHAINED;
    }
    UNLOCK(&_allBlocksMutex);
    return retVal;

}
/*
 * DESCRIPTION: This function blocks all other Block attachments, until block_num is added to the chain. The block_num is the assigned value
 * that was previously returned by add_block.
 * RETURN VALUE: If block_num doesn't exist, return -2;
 * In case of other errors, return -1; In case of success or if it is already attached return 0.
 */
int ChainManager::attach_now(int block_num) {
    int returnVal = SUCCESS;
    if(!_isInit) return FAILURE;
    // TODO : CHECK IF NEED TO LOCK LEAFS
    LOCK(&generalMutex);
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    UNLOCK(&_allBlocksMutex);
    if(findBlock == _allBlocks.end()) returnVal = NOT_EXIST;
    else if(findBlock->second->_isAttached) returnVal = SUCCESS;
    // not yet attached
    else {
        LOCK(&_waitingListMutex);
        _waitingList.remove(block_num);
        UNLOCK(&_waitingListMutex);

        chain_block(findBlock->second);
    }
    UNLOCK(&generalMutex);
    return returnVal;
}

int ChainManager::was_added(int block_num) {


    int retVal = SUCCESS;
    if(!_isInit) return FAILURE;

//    LOCK(&_allBlocksMutex);

    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    if(findBlock == _allBlocks.end()) retVal =  NOT_EXIST;
    if(findBlock->second->_isAttached) retVal =  ALREADY_CHAINED;
//    UNLOCK(&_allBlocksMutex);
    return retVal;
}

int ChainManager::chain_size() {

    if(!_isInit) return FAILURE;
//    PRINT(_chainSize)
    return _chainSize;
}

void ChainManager::remove_leaf(int block_num)
{
    _leafs.erase(remove(_leafs.begin(), _leafs.end(), block_num), _leafs.end());
}

void ChainManager::remove_deepestLeaf(int block_num)
{
    _deepestLeafs.erase(remove(_deepestLeafs.begin(), _deepestLeafs.end(), block_num), _deepestLeafs.end());
}

void* ChainManager::daemonThread(void* ptr) {
//    PRINT("DAEMON STARTED")
    ChainManager* chain = (ChainManager*) ptr;
    while(!chain->_closeChain) {

//        PRINT("waiting size " << chain->_waitingList.size())
        if(!chain->_waitingList.empty()) {

            LOCK(&chain->_waitingListMutex);
            LOCK(&chain->_allBlocksMutex);

//            PRINT("waiting List front " << chain->_waitingList.front())
            Block* toAttach = chain->_allBlocks[chain->_waitingList.front()];
            if(toAttach == NULL) PRINT("TO ATTACH NULL")
            UNLOCK(&chain->_allBlocksMutex);
            chain->_waitingList.pop_front();
            UNLOCK(&chain->_waitingListMutex);
//            PRINT("before daemon general lock");
            LOCK(&chain->generalMutex);
            chain->chain_block(toAttach);
            UNLOCK(&chain->generalMutex);
        }
//        PRINT("after if !!!")
        else {
//            LOCK(&chain->daemonMutex);
            pthread_cond_wait(&chain->waitingListCond, &chain->daemonMutex);
//            UNLOCK(&chain->daemonMutex);
        }
        //TODO : add condition wait

    }
    return NULL;
}


int ChainManager::chain_block(Block* toChain) {
    // to longest or father is missing
//    PRINT("TO CHAIN" << toCHAIN)
//    PRINT("CHAIN BLOCK" << toChain->_father)
    if(toChain->_father == NULL) {
        Block* father = get_father_rand();
        toChain->_father = father;
        toChain->_depth = father->_depth + 1;
    }
    toChain->generate_data();
    // TODO : CHECK IF NEED TO LOCK LEAFS

    toChain->_father->add_son();
    LOCK(&_leafsMutex);
    LOCK(&_deepestLeafsMutex);

    if(toChain->_depth > _deepestDepth) { // deeper depth
        _deepestLeafs.clear();
        _deepestLeafs.push_back(toChain->_id);
        _leafs.push_back(toChain->_id);
        _deepestDepth = toChain->_depth;
    }
    else if (toChain->_depth == _deepestDepth){ // new leaf but not deepest
        _leafs.push_back(toChain->_id);
        _deepestLeafs.push_back(toChain->_id);
    }
    else {
        _leafs.push_back(toChain->_id);
    }

    if(toChain->_father->_cntSons == 1) {//removes the father only if he have one son.
        remove_leaf(toChain->_fatherId);
    }
    _chainSize++;
    toChain->_isAttached = true;
    UNLOCK(&_deepestLeafsMutex);
    UNLOCK(&_leafsMutex);


    return SUCCESS;
}

// TODO : TO CHECK BEFORE CHECKING FATHER IF EXIST IN MAP
int ChainManager::getUntouchable() {

//    Block* findBlock;
//    vector<int> allDeepests;
//    LOCK(&_leafsMutex);
//    for(unsigned int i = 0; i < _deepestLeafs.size(); i++) {
//        findBlock = _allBlocks.find(_leafs[i])->second;
//        if(findBlock->_depth == _deepestDepth) {
//            allDeepests.push_back(findBlock->_id);
//        }
//    }
//    UNLOCK(&_leafsMutex);
//    //return the longest randomized leaf
//    //PRINT("longest randomized leaf " << allDeepests[rand()%(allDeepests.size())]);
    return _deepestLeafs[rand()%(_deepestLeafs.size())];

}

/*
* DESCRIPTION: Search throughout the tree for sub-chains that are not the longest chain,
* detach them from the tree, free the blocks, and reuse the block_nums.
* RETURN VALUE: On success 0, otherwise -1.
*/
int ChainManager::prune_chain() {
    // TODO :: to check when return -1
    Block* tmpBlock;
    Block* findBlock;
    vector<int> allMinDepths;
    LOCK(&generalMutex);
    LOCK(&_allBlocksMutex);
    int leafsSize = _leafs.size();
    LOCK(&_deepestLeafsMutex);
    int deepestId = _deepestLeafs[rand()%(_deepestLeafs.size())];
    UNLOCK(&_deepestLeafsMutex);
    // to find all the leafs with min depth
    for(int i = 0; i < leafsSize; i++) {

        findBlock = _allBlocks.find(_leafs[i])->second;

        // remove all blocks till sub-root
        if(findBlock->_id != deepestId) {

            while(findBlock->_id != GENESIS_ID && findBlock->_cntSons == NO_SONS)
            {
                //print block id of block that erasing now.
//                PRINT("prune: erasing block id  " << findBlock->_id);
//                PRINT("fathers block id  " << findBlock->_fatherId);
                tmpBlock = findBlock->_father;
//                //sons before dcr_son()
//                PRINT(findBlock->_father->_cntSons);
                tmpBlock->dcr_son();
//                //sons after dcr_son()
//                PRINT(findBlock->_father->_cntSons);
                LOCK(&_blockIdsMutex);
                _blockIds.push(findBlock->_id);
                UNLOCK(&_blockIdsMutex);
                _allBlocks.erase(findBlock->_id);
                findBlock = tmpBlock;
            }
            remove_deepestLeaf(_leafs[i]);
            remove_leaf(_leafs[i]);
            leafsSize--;

        }

    }
    UNLOCK(&_allBlocksMutex);
    UNLOCK(&generalMutex);
    return SUCCESS;
}

/*
 * DESCRIPTION: Close the recent blockchain and reset the system, so that it is possible to call init_blockchain again. Non-blocking.
 * All pending Blocks should be hashed and printed to terminal (stdout).
 * Calls to library methods which try to alter the state of the Blockchain are prohibited while closing the Blockchain.
 * e.g.: Calling chain_size() is ok, a call to prune_chain() should fail.
 * In case of a system error, the function should cause the process to exit.
 */
void ChainManager::close_chain() {
    LOCK(&generalMutex);

    _closeChain = true;
    for(int block:_waitingList) {
        _allBlocks[block]->generate_data();
        PRINT(_allBlocks[block]->_data);
    }

    _allBlocks.erase(_allBlocks.begin());
    _waitingList.clear();
    _leafs.clear();
    _deepestLeafs.clear();
    while(!_blockIds.empty()) {
        _blockIds.pop();
    }
//    pthread_mutex_destroy(&daemonMutex);
//    pthread_cond_destroy(&waitingListCond);
    pthread_mutex_destroy(&_waitingListMutex);
    pthread_mutex_destroy(&_leafsMutex);
    pthread_mutex_destroy(&_deepestLeafsMutex);
    pthread_mutex_destroy(&_allBlocksMutex);
    pthread_mutex_destroy(&_blockIdsMutex);

    _isInit = false;
    close_hash_generator();
    _closeChain = false;
    UNLOCK(&generalMutex);

    pthread_cond_signal(&waitingListCond);

}

/*
* DESCRIPTION: The function blocks and waits for close_chain to finish.
* RETURN VALUE: If closing was successful, it returns 0.
* If close_chain was not called it should return -2. In case of other error, it should return -1.
*/
int ChainManager::return_on_close() {
//    pthread_cond_wait(&waitingListCond, &daemonMutex);
//    while(_closeChain);

    if(_closeChain) return pthread_join(daemonTrd, NULL);
    return -2;
}



//void ChainManager::printCurChain(){
////    for(int i = 0; i < _leafs.size(); i++) {
////        Block*  curBlock = _allBlocks.find(_leafs[i])->second;
////        while(curBlock->_id != 0) {
////            cout << curBlock->_id << " --> " ;
////            curBlock = curBlock->_father;
////        }
////        cout << endl;
////    }
//}

