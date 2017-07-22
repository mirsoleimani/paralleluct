/* 
 * File:   PGameTree.cpp
 * Author: SAM
 * 
 * Created on July 8, 2014, 5:26 PM
 */

#include <memory>

#include "GameTree.h"


GameTree::GameTree(int b, int d, float mv) : breath(b),depth(d), maxMoveVal(mv) {
    size = (int)(pow((float)breath,depth+1)-1)/(breath-1);    //calculate total number of nodes in the tree
    assert(size >0 && "Can not create a tree with size zero");
    cerr<<"Size of the tree= "<<size<<endl;
    
    moveVal = new vector<char>(size);
    //nodeVal = new int[size];    //allocate tree
    assert(moveVal->size() != 0 && "can not allocate tree with specified size!");
    
    path.resize(size);
    //path = new int[depth]; //allocate a path in the tree
    assert(path.size() != 0 && "can not allocate path in tree!");
    
    val=0;
    currIdx=0;
    currDepth=-1;
    
}

int GameTree::GenMoves(vector<int>& moves){
    if(currDepth == depth) 
        return 0;
    for(int i=0;i<breath;i++){
        moves.push_back(i);
    }
    return breath;
}
GameTree::~GameTree() {
    moveVal->clear();
}

void GameTree::GenTree(int seed){
    assert(seed != -1 && "seed can not be -1");
    srand(seed);
    
    for(long int i=0;i<size;i++){
        (*moveVal)[i] = RandInt(maxMoveVal);  //Assign a random value to each node
        assert(((*moveVal)[i] < maxMoveVal) && "value of the node is higher than maximum value!");
    }
}

void GameTree::ResetTree(){
    val=0;
    currIdx=0;
    currDepth=0;
    moveVal->clear();
}

long int GameTree::NextIdx(int c){
    // the index of c-th child
    return breath*currIdx+1+c;
}

long int GameTree::PrevIdx(){
    assert(currIdx >0 && "root has no parent!");
    return floor((float)(currIdx-1)/breath);
}

long int GameTree::CurrIdx(){
    return currIdx;
}

bool GameTree::GameOver(){
    return (currDepth == depth);
}

int GameTree::PTM(){
    return currDepth%2;
}

int GameTree::DoMove(int move){
    if((currDepth==depth) || (move >= breath) || (move <0)) return 0;
    currIdx = NextIdx(move);
    currDepth++;
    val += MoveValue();
    return 1;
}

int GameTree::UndoMove() {
    if(currIdx == 0) return 0;
    val -=MoveValue();
    currIdx = PrevIdx();
    currDepth--;
    return 1;
}

int GameTree::MoveValue(){
    int m,r;
    if(currDepth % 2 == 1){
        m=(*moveVal)[currIdx];
        r= m & 0x7f;
return r;        
        //return moveVal[currIdx] & 0x7f;
    }else{
        m=(*moveVal)[currIdx];
        r= m & 0x7f;
return -r; 
        //return -(moveVal[currIdx] & 0x7F);
    }
}

float GameTree::ResultValue(int v){
    return (v>0)?1:(v==0?0.5f:0);
}

/**
 * This method assign a negative value to each terminal position that yields a 
 * win for the MIN player, ad a positive value for the MAX player
 * @return 
 */
float GameTree::IsWin(){
    //if(ptm == 0)
        return ResultValue(val);
    //else
        //return ResultValue(-val);
}
int GameTree::CurrDepth(){
    return currDepth;
}

long int GameTree::GetKey(){
    return currIdx;
}

void GameTree::PrintGame(FILE* f){
    int i;
    char pre[100];
    for(i=0;i<currDepth;i++) pre[i]=' ';
    sprintf(pre+i, "%3ld %2d", currIdx, currDepth);
    
    if(currIdx==0){
        fprintf(f,"Size=%d\n",size);
        fprintf(f,"%s %d %d %0.1f\n",pre,MoveValue(),val,IsWin());
    }else{
        fprintf(f,"%s %d %d %0.1f\n",pre,MoveValue(),val,IsWin());
    }
    if(currDepth<depth)
        for(int i=0;i<breath;i++){
            DoMove(i);
            PrintGame(f);
            UndoMove();
        }
}