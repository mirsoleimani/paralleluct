/* 
 * File:   PGameState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on 17 september 2014, 15:42
 */

#include <bitset>

#include "PGameState.h"

PGameState::PGameState(int b,int d,float maxmv, int seed):breath(b),depth(d) {
    size = (size_t)(pow((float)breath,depth+1)-1)/(breath-1);    //calculate total number of nodes in the tree
    assert(size >0 && "Can not create a tree with size zero");
    cerr<<"Number of the nodes in p-game tree: "<<size<<endl;
    
    moveVal = new vector<char>(size);
    assert(moveVal->size() != 0 && "can not allocate tree with specified size!");
    
    path = new long int[depth];
    val=0;
    maxVal=maxmv;
    currIdx=0;
    currDepth=0;
    pjm = 2; // At the root pretend the player just moved is player 2 - player 1 has the first move
    //assert(seed != -1 && "seed can not be -1");
    //srand(seed);
    
    //Generate game tree
    for(long int i=0;i<size;i++){
        (*moveVal)[i] = RandInt(maxmv);  //Assign a random value to each node
        assert(((*moveVal)[i] < maxmv) && "value of the node is higher than maximum value!");
    }
    
}

void PGameState::NewGame()
{
    val=0;
    currIdx=0;
    currDepth=0;
    pjm=2;
    
    //Generate game tree
    for(long int i=0;i<size;i++){
        (*moveVal)[i] = RandInt(maxVal);  //Assign a random value to each node
        assert(((*moveVal)[i] < maxVal) && "value of the node is higher than maximum value!");
    }
    
}
PGameState::PGameState(const PGameState& orig) {
    breath = orig.breath;
    depth = orig.depth;
    size= orig.size;
    val=orig.val;
    currIdx=orig.currIdx;
    currDepth = orig.currDepth;
    pjm = orig.pjm;
    path = new long int[depth];
    for(int i=0;i<depth;i++)
        path[i]= orig.path[i];
    //assert(moveVal->size() == 0 && "can not copy to nonzero children vector!");
    moveVal = new vector<char>(size);
    assert(moveVal->size() != 0 && "can not allocate tree with specified size!");
    for (int i = 0; i < orig.moveVal->size(); i++) {
        (*moveVal)[i]=orig.moveVal->at(i);
    }
}

PGameState::~PGameState() {
    //cout<<"Freeing memory!\n";
    moveVal->clear();
    delete(moveVal);
    delete(path);
}

int PGameState::GetMoves(vector<int>& moves){
    if(currDepth == depth) 
        return -1;
    for(int i=0;i<breath;i++){
        moves.push_back(i);
    }
    return breath;
}

int PGameState::GetRandMove(){
     if(currDepth == depth) 
        return -1;
     return RandInt(breath-1);
}

void PGameState::DoRandGame(){
    
}
long int PGameState::NextIdx(int c){
    // the index of c-th child
    //return (currIdx+1+c*((int)pow((float)breath,depth-currDepth)-1/(breath-1)));
    return (breath*currIdx+1+c);
}

long int PGameState::PrevIdx(){
    assert(currDepth >0 && "root has no parent!");
        
    //return floor((float)(currIdx)/breath);
    return path[currDepth-1];
}

int PGameState::DoMove(int move){
    if((currDepth==depth)|| (move >= breath) || (move <0)) return 0;
    path[currDepth]=currIdx;
    currIdx = NextIdx(move);
    currDepth++;
    pjm=3-pjm;
    val += MoveValue();
    return 1;
}

int PGameState::UndoMove(){
    if(currIdx == 0) return 0;
    val -=MoveValue();
    currIdx = PrevIdx();
    currDepth--;
    pjm=3-pjm;
    return 1;
}

int PGameState::CurrIndicator(){
    return currDepth;
}

bool PGameState::GameOver(){
    return (currDepth == depth);
}

int PGameState::MoveValue(){
    assert(currIdx<size && "Index goes out of order");
    if(currDepth % 2 == 1){ 
        return (*moveVal)[currIdx] & 0x7f;
    }else
    {
        return -((*moveVal)[currIdx] & 0x7F);
    }
}

float PGameState::Result(int v){
        return (v>0)?1:(v==0?0.5f:0);
}

float PGameState::GetResult(int plyjm){
      if(plyjm == 1)
        return Result(-val);    //Player 1 is adding negative values therefore if -val is positive it means player 1 is won
      else
        return Result(val);
}
float PGameState::GetValue()
{
    return val;
}
int PGameState::PlyJustMoved(){
    return pjm;
}

void PGameState::Print(){
    cout<<GameToString(0,0)<<endl;
}

string PGameState::GameToString(int indent,int m){
    vector<int> moves;
    string s = IndentString(indent)+ StateToString(m);
    this->GetMoves(moves);
    for(int i=0;i<moves.size();i++){
    this->DoMove(moves[i]);
    s+=GameToString(indent+1,moves[i]);
    this->UndoMove();
    }
    return s;
}

string PGameState::StateToString(int m){
    
    return  "[Idx:"+ NumToStr<int>(this->currIdx)+
            " M:"+ NumToStr<int>(m)+
            " MVal:"+NumToStr<int>((this->currDepth%2==1)?((*moveVal)[currIdx]):-((*moveVal)[currIdx]))+
            " Val:"+NumToStr<int>(this->val);
}

string PGameState::IndentString(int indent){
    string s="\n";
    for(int i=0;i<indent;i++){
        s +="| ";
    }
    return s;
}

float PGameState::EvaluateBoardDSet(int plyjm, int direction){
    
}

void PGameState::UndoMoves(int beg){
    
}
