//
// Created by alonperl on 5/14/15.
//

#include <stdio.h>
#include <iostream>
#include "blockchain.h"
using namespace std;

int main()  {
//    Block* genesis = new Block(-1,0,"aasd",4,NULL);
//    Block* a = new Block(0,1,"a",1,genesis);
//    if(a->_father != NULL) cout << "father live" << endl;
//    else cout << "father dead" << endl;
////    cout <<  1==1 << endl;
    cout<< "init blockchain" << endl;
    init_blockchain();
    return 0;

}
