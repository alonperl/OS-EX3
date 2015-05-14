//
// Created by alonperl on 5/15/15.
//

#ifndef OS_BLOCK_H
#define OS_BLOCK_H
class Block {

public:

    int _fatherId;
    int _id;
    char *_data;
    int _dataLength;
    int _depth;
    Block *_father;
    bool _to_longest;
    bool _isAttached;
    unsigned int _cntSons;


    Block();

    Block(int fatherId, int blockId, char *data, int dataLength, Block *father);

    ~Block();

    void generate_data();

    int get_id();

    void set_data(char *data);

    int get_depth();

    void set_to_longest();

    void add_son();

    void dcr_son();

};

#endif //OS_BLOCK_H
