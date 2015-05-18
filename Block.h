//
// Created by alonperl on 5/15/15.
//

#ifndef OS_BLOCK_H
#define OS_BLOCK_H

// This class raised to store all Block data.
class Block {

public:
    // The id of the block 
    int _id;
    
    // The father details 
    int _fatherId;
    Block *_father;

    // Data of the block 
    char *_data;
    int _dataLength;

    // Depth of the block - steps from the genesis block 
    int _depth;
    
    /// Flag for to_longest request
    bool _to_longest;
    bool _isAttached;

    // Counter to count the number of sons of the block -
    // raised to help with the prune algorithm.
    unsigned int _cntSons;

    // Desault Ctor 
    Block();

    // Ctor for a new normal block.
    // @param : fatherid id of the father
    // @param : if to attach to the new block.
    // @param : data to be hashed when it will chain.
    // @param : dataLength the data length
    // @param : father pointer to the block's father 
    Block(int fatherId, int blockId, char *data, int dataLength, Block *father);

    // Destructor - deleting the data that hashed. 
    ~Block();

    // This function called while chaining the block to the chain,
    // it will generate the hashed data and will store in _data. 
    void generate_data();

    // set the block to longest 
    void set_to_longest();


    int get_id();
    void set_data(char *data);
    int get_depth();

    // this function count the son for the block to _cntSons.
    void add_son();

    // this function decrease the son for the block to _cntSons.
    void dcr_son();

};

#endif //OS_BLOCK_H
