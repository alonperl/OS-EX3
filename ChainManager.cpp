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


int ChainManager::init_blockchain() {
    try {
        if(!_isInit) {
            srand(time(0));
            init_hash_generator();
//            _waitingList = new list<int>();
            _genesis = new Block(GENESIS_FATHER_ID, GENESIS_ID, GENESIS_DATA, GENESIS_LENGTH, GENESIS_FPTR);
//            this._deepestLeafs = new vector<int>();
//            this->_leafs = new vector<int>();
//            this->_blockIds = new  priority_queue<int, vector<int>, greater<int>>();
            _allBlocks[GENESIS_ID] = _genesis;
            _chainSize = 0;
            _deepestDepth = 0;
            _deepestLeafs.push_back(GENESIS_ID);
            _leafs.push_back(GENESIS_ID);
            for(int i = 1; i < INT_MAX; i++) {
                _blockIds.push(i);
            }
            _closeChain = false;
            _isInit = true;
            //TODO : check if create of, else print error

            pthread_create(&deamonTrd, NULL, &daemonThread, this);
        }
        else {
            perror("init_blockchain called twice");
            return FAILURE;
        }

    }
    catch(exception& e) {
        cout<<"INIT EXCEPTION" << endl;
        return FAILURE;
    }
    return SUCCESS;
}

//returns arbitrary longest chains leaf (if there is more than one).
Block* ChainManager::get_father_rand() {
    return _allBlocks[_deepestLeafs[rand()%(_deepestLeafs.size())]];
}

int ChainManager::add_block(char *data, size_t length) {
    //TODO: check if chain was closed

    if(!_blockIds.empty() && _isInit) {
        // TODO : add mutex protection
        Block* father = get_father_rand();
        int new_id = _blockIds.top();
        _blockIds.pop();
        char* tmp = new char[length];
        memcpy(tmp, data, length);
        Block* newBlock = new Block(father->_id, new_id, tmp, length, father);
        _waitingList.push_back(newBlock->_id);
        _allBlocks[new_id] = newBlock;
        return new_id;
    }
    return FAILURE;

}

int ChainManager::to_longest(int block_num) {
    if(!_isInit) return FAILURE;
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    if(findBlock == _allBlocks.end()) return NOT_EXIST;
    // handle block exist and found
    if(!findBlock->second->_isAttached) {
        findBlock->second->set_to_longest();
        return SUCCESS;
    }
    else if(_allBlocks[block_num]->_isAttached) {
        return ALREADY_CHAINED;
    }

}

int ChainManager::was_added(int block_num) {
    if(!_isInit) return FAILURE;
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    if(findBlock == _allBlocks.end()) return NOT_EXIST;
    if(findBlock->second->_isAttached) return ALREADY_CHAINED;
    return SUCCESS;
}

int ChainManager::chain_size() {
    if(!_isInit) return FAILURE;
    return _chainSize;
}

void* ChainManager::daemonThread(void* ptr) {
    ChainManager* chain = (ChainManager*) ptr;
    while(!chain->_closeChain) {
        pthread_mutex_lock(&chain->_waitingListMutex);

        if(!chain->_waitingList.empty()) {
            Block* toAttach = chain->_allBlocks[chain->_waitingList.front()];
            chain->_waitingList.pop_front();
            chain->chain_block(toAttach);
        }
        //TODO : add condition wait
        pthread_mutex_unlock(&chain->_waitingListMutex);
    }


}
void ChainManager::remove_leaf(int block_num)
{
    _leafs.erase(remove(_leafs.begin(), _leafs.end(), block_num), _leafs.end());
}

int ChainManager::chain_block(Block* toChain) {
    // to longest or father is missing
    if(toChain->_father == NULL) {
        Block* father = get_father_rand();
        toChain->_father = father;
        toChain->_depth = father->_depth + 1;
    }
    toChain->generate_data();
    // TODO : CHECK IF NEED TO LOCK LEAFS
    pthread_mutex_lock(&_deepestLeafsMutex);
    _chainSize++;
    toChain->_father->add_son();
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
    pthread_mutex_unlock(&_deepestLeafsMutex);
    toChain->_isAttached = true;
    return SUCCESS;
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
    pthread_mutex_lock(&_deepestLeafsMutex);
    pthread_mutex_lock(&_waitingListMutex);
    unordered_map<unsigned int, Block*>::iterator findBlock = _allBlocks.find(block_num);
    if(findBlock == _allBlocks.end()) returnVal = NOT_EXIST;
    else if(findBlock->second->_isAttached) returnVal = SUCCESS;
    // not yet attached
    else {
        _waitingList.remove(block_num);
        chain_block(findBlock->second);
    }
    pthread_mutex_unlock(&_waitingListMutex);
    pthread_mutex_unlock(&_deepestLeafsMutex);

    return returnVal;
}

// TODO : TO CHECK BEFORE CHECKING FATHER IF EXIST IN MAP
int ChainManager::getUntouchable() {

    Block* findBlock;
    vector<int> allDeepests;
    for(int i = 0; i < _leafs.size(); i++) {
        findBlock = _allBlocks.find(_leafs[i])->second;
        if(findBlock->_depth == _deepestDepth) {
            allDeepests.push_back(findBlock->_id);
        }
    }
    //return the longest randomized leaf
    return allDeepests[rand()%(allDeepests.size())];

}

/*
* DESCRIPTION: Search throughout the tree for sub-chains that are not the longest chain,
* detach them from the tree, free the blocks, and reuse the block_nums.
* RETURN VALUE: On success 0, otherwise -1.
*/
int ChainManager::prune_chain() {
    Block* tmpBlock;
    Block* findBlock;
    vector<int> allMinDepths;
    int leafsSize = _leafs.size();
    int deepestId = getUntouchable();
    // to find all the leafs with min depth
    for(int i = 0; i < leafsSize; i++) {
        findBlock = _allBlocks.find(_leafs[i])->second;
        // remove all blocks till sub-root
        if(findBlock->_id != deepestId) {

            while(findBlock->_id != GENESIS_ID && findBlock->_cntSons == NO_SONS)
            {
                tmpBlock = findBlock->_father;
                tmpBlock->dcr_son();
                _blockIds.push(findBlock->_id);
                _allBlocks.erase(findBlock->_id);
                findBlock = tmpBlock;
            }
            remove_leaf(_leafs[i]);
            leafsSize--;

        }
    }
}

