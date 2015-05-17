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

ChainManager::ChainManager() {
    _chainStatus = CLOSED;
}
int ChainManager::init_blockchain() {
    generalMutex = PTHREAD_MUTEX_INITIALIZER;
    try {
//        cout << "before init block chain manager" << endl;

//        cout << "init block chain manager" << endl;
        if(_chainStatus == CLOSED) {
            LOCK(&generalMutex);
            srand(time(0));
            init_hash_generator();

            pthread_mutex_init(&daemonMutex, NULL);
            pthread_cond_init(&waitingListCond, NULL);
            pthread_cond_init(&pruneCloseCond, NULL);
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
//    PRINT(randIndex);
//    for(int i = 0; i< _deepestLeafs.size(); i++)
//        cout << _deepestLeafs[i] <<" --> ";
//    cout << endl;
    LOCK(&_allBlocksMutex);
    Block* tmp = _allBlocks[_deepestLeafs[randIndex]];
    UNLOCK(&_allBlocksMutex);
    return tmp;

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
    if(_highestId < INT_MAX && _chainStatus == INITIALIZED) {
//        LOCK(&generalMutex);
//        PRINT("sdasd");
        // TODO : add mutex protection


//        if(father == NULL) PRINT("FATHER IS NULL")

        int new_id = get_new_id();
//        LOCK(&generalMutex);
//        LOCK(&_deepestLeafsMutex);
        Block* father = get_father_rand();
//        UNLOCK(&_deepestLeafsMutex);
//        UNLOCK(&generalMutex);
        Block* newBlock = new Block(father->_id, new_id, data, length, father);
        LOCK(&_waitingListMutex);
        _waitingList.push_back(newBlock->_id);
        UNLOCK(&_waitingListMutex);
        LOCK(&_allBlocksMutex);
        _allBlocks[new_id] = newBlock;
        UNLOCK(&_allBlocksMutex);

        LOCK(&daemonMutex);
        pthread_cond_signal(&waitingListCond);
        UNLOCK(&daemonMutex);
//      PRINT("add block " << new_id)
//        UNLOCK(&generalMutex);
        return new_id;
    }
//    UNLOCK(&generalMutex);
    return FAILURE;

}

int ChainManager::to_longest(int block_num) {
    int retVal;
    if(_chainStatus != INITIALIZED) return FAILURE;
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
    if(_chainStatus !=  INITIALIZED) return FAILURE;
    // TODO : CHECK IF NEED TO LOCK LEAFS

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
        LOCK(&generalMutex);
        chain_block(findBlock->second);
        UNLOCK(&generalMutex);
    }

    return returnVal;
}

int ChainManager::was_added(int block_num) {


    int retVal = SUCCESS;
    if(_chainStatus != INITIALIZED) return FAILURE;

//    LOCK(&_allBlocksMutex);
    LOCK(&_allBlocksMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    UNLOCK(&_allBlocksMutex);
    if(findBlock == _allBlocks.end()) retVal =  NOT_EXIST;
    if(findBlock->second->_isAttached) retVal =  ALREADY_CHAINED;
//    UNLOCK(&_allBlocksMutex);
    return retVal;
}

int ChainManager::chain_size() {

    if(_chainStatus == CLOSED ) return FAILURE;
//    PRINT(_chainSize)
    return _chainSize;
}

void ChainManager::remove_leaf(int block_num)
{
//    LOCK(&_leafsMutex);
    _leafs.erase(remove(_leafs.begin(), _leafs.end(), block_num), _leafs.end());
//    UNLOCK(&_leafsMutex);
}

void ChainManager::remove_deepestLeaf(int block_num)
{
    LOCK(&_deepestLeafsMutex);
    _deepestLeafs.erase(remove(_deepestLeafs.begin(), _deepestLeafs.end(), block_num), _deepestLeafs.end());
    UNLOCK(&_deepestLeafsMutex);
}

void* ChainManager::daemonThread(void* ptr) {
    PRINT("DAEMON STARTED")
    ChainManager* chain = (ChainManager*) ptr;

    while(chain->_chainStatus == INITIALIZED) {

//        PRINT("waiting size " << chain->_waitingList.size())
        LOCK(&chain->daemonMutex);
        if(chain->_waitingList.empty()) {
//            LOCK(&chain->daemonMutex);
            if(pthread_cond_wait(&chain->waitingListCond, &chain->daemonMutex))
            {PRINT("ERROR WAIT");
                exit(FAILURE);   }//            UNLOCK(&chain->daemonMutex);
        }
//        PRINT("after if !!!")
        else {

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
        UNLOCK(&chain->daemonMutex);
        //TODO : add condition wait

    }

    pthread_exit(NULL);
    return NULL;
}


int ChainManager::chain_block(Block* toChain) {
    toChain->_isAttached = true;
    if(toChain->_father == NULL) {
        Block* father = get_father_rand();
        toChain->_father = father;
        toChain->_depth = father->_depth + 1;
    }
    if(toChain->_father == NULL) {
        PRINT("FATHER NULL" << toChain->_id)
        exit(1);
    }
    toChain->generate_data();
    toChain->_father->add_son();
    if(toChain->_depth > _deepestDepth) { // deeper depth
        LOCK(&_deepestLeafsMutex);
        _deepestLeafs.clear();
        _deepestLeafs.push_back(toChain->_id);
        UNLOCK(&_deepestLeafsMutex);
        LOCK(&_leafsMutex);
        _leafs.push_back(toChain->_id);
        UNLOCK(&_leafsMutex);
        _deepestDepth = toChain->_depth;
    }
    else if (toChain->_depth == _deepestDepth){ // new leaf but not deepest
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

    if(toChain->_father->_cntSons == 1) {//removes the father only if he have one son.
        LOCK(&_leafsMutex);
        remove_leaf(toChain->_fatherId);
        UNLOCK(&_leafsMutex);
    }
    _chainSize++;





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

    LOCK(&generalMutex);
    // TODO :: to check when return -1
    Block* tmpBlock;
    Block* findBlock;
    vector<int> allMinDepths;
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



                LOCK(&_allBlocksMutex);
                int id = findBlock->_id;
                _allBlocks.erase(findBlock->_id);
                delete (findBlock);
                UNLOCK(&_allBlocksMutex);
                LOCK(&_blockIdsMutex);
                _blockIds.push(id);
                UNLOCK(&_blockIdsMutex);

                findBlock = tmpBlock;
            }

//            LOCK(&_deepestLeafsMutex);
            remove_deepestLeaf(_leafs[i]);
//            UNLOCK(&_deepestLeafsMutex);

            LOCK(&_leafsMutex);
            remove_leaf(_leafs[i]);
            UNLOCK(&_leafsMutex);

            leafsSize--;

        }

    }

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

//    pthread_cond_wait(&pruneCloseCond, &generalMutex);
    _chainStatus = CLOSING;
    pthread_create(&closingTrd, NULL, close_chain_helper, this);

}

void* ChainManager::close_chain_helper(void* ptr) {

    ChainManager* chain = (ChainManager*) ptr;
    LOCK(&chain->generalMutex);
    LOCK(&chain->_allBlocksMutex);
    LOCK(&chain->_waitingListMutex);
    for(int block:chain->_waitingList) {

        chain->_allBlocks[block]->generate_data();

        PRINT(chain->_allBlocks[block]->_data);
    }
    for(auto it = chain->_allBlocks.begin(); it != chain->_allBlocks.end(); ++it){
        if(it->second)
          delete (it->second);
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
    while(!chain->_blockIds.empty()) {
        chain->_blockIds.pop();
    }
    UNLOCK(&chain->_blockIdsMutex);



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
//    pthread_cond_wait(&waitingListCond, &daemonMutex);
//    while(_closeChain);
    if(_chainStatus != CLOSING) return FAILURE;
    pthread_join(closingTrd, NULL);
    return SUCCESS;
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

