/* 
 * File:   FifteenPuzzleState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on June 3, 2016, 12:18 PM
 */

#include <cstring>
#include <ios>
#include <vector>
#include <thread>
#include <string.h>

#include "paralleluct/state/GemPuzzleState.h"

GemPuzzleState::GemPuzzleState(const char* str):_reward(0),_pjm(WHITE) {
    MakeBoard(str);
    _board = _srcBoard;
    _zeroPos = _srcZeroPos;
    _dstBoard=_srcBoard;
    _fix.resize(_size);
    std::sort(_dstBoard.begin(), _dstBoard.end());
    std::swap(_dstBoard[0],_dstBoard[_size-1]);
    std::fill(_fix.begin(),_fix.end(),-1);
    _srcReward = Evaluate();
    
}

GemPuzzleState::GemPuzzleState(const GemPuzzleState& orig) {
    _srcBoard = orig._srcBoard;
    _dstBoard = orig._dstBoard;
    _board = orig._board;
    _fix = orig._fix;
    _edges = orig._edges;
    _zeroPos = orig._zeroPos;
    _srcZeroPos = orig._srcZeroPos;
    _dim = orig._dim;
    _size = orig._size;
    _pjm = orig._pjm;
    _reward = orig._reward;
    _srcReward = orig._srcReward;
}

GemPuzzleState::~GemPuzzleState() {
}
void GemPuzzleState::Reset(){
    _board=_srcBoard;
    _zeroPos = _srcZeroPos;
    std::fill(_fix.begin(),_fix.end(),-1);
}
/**
 * Converts @param str of the form
 * "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0"
 * to a state. Update _board and _zeroPos.
 * Return the length of @param str as the size of the _board.
 * @param str
 * @return i
 */
int GemPuzzleState::ParseToState(const char* str) {
    char* token;
    auto s = std::unique_ptr<char[]>(new char[std::strlen(str) + 1]); // +1 for the null terminator
    int i = 0;
    std::strcpy(s.get(), str);
    token = std::strtok(s.get(), ",");
    while (token) {
        assert(0 < atoi(token) && "The string is not valid!\n");
        _srcBoard.push_back(atoi(token));
        if (!std::strcmp(token,"0")) {
            _srcZeroPos = i;
        }
        token = std::strtok(NULL, ",");
        i++;
    }
    return i;

}
void GemPuzzleState::MakeBoard(const char* str){
    
    _size = ParseToState(str);
    //TODO what is the right way of using assert here
    //assert(!(size == 17 || size == 25)&&"The state is not valid!\n");
    _dim = sqrt(_size);

    for(int i=0;i<_size;i++){
        vector<int> list;
        _edges.push_back(list);
    }
        
    for (int i = 0; i < _dim; i++) {   
        for (int j = 0; j < _dim; j++) {
            MakeEdges(i,j,_edges[POS(i,j,_dim)]);
        }
    }
        
}
/**
 * Create the adjacency list for each position on the board. An example of the 
 * board for size of 15 is shown below. The edges are created for a position in
 * a clockwise manner.
 *     *---*---*---*---*
 *     |0  |1  |2  |3  |
 *     *---*---*---*---*
 *     |4  |5  |6  |7  |
 *     *---*---*---*---*
 *     |8  |9  |10 |11 |
 *     *---*---*---*---*
 *     |12 |13 |14 |15 |
 *     *---*---*---*---*
 * @param row
 * @param col
 */
void GemPuzzleState::MakeEdges(int row, int col,vector<int>& list) {
    if (row == 0) {
        if (col == 0) {
            list.push_back(POS((row), (col + 1), _dim));
            list.push_back(POS((row + 1), (col), _dim));
        } else if (col == _dim - 1) {
            list.push_back(POS((row + 1), (col), _dim));
            list.push_back(POS((row), (col - 1), _dim));
        } else {
            list.push_back(POS(row, (col + 1), _dim));
            list.push_back(POS((row + 1), col, _dim));
            list.push_back(POS(row, (col - 1), _dim));
        }
    } else if (row == _dim - 1) {
        if (col == 0) {
            list.push_back(POS((row-1), (col), _dim));
            list.push_back(POS((row), (col+1), _dim));
        } else if (col == _dim - 1) {
            list.push_back(POS((row), (col-1), _dim));
            list.push_back(POS((row-1), (col), _dim));
        } else {
            list.push_back(POS((row), (col-1), _dim));
            list.push_back(POS((row-1), (col), _dim));
            list.push_back(POS((row), (col+1), _dim));
        }
    } else {
        if (col == 0) {
            list.push_back(POS((row-1), (col), _dim));
            list.push_back(POS((row), (col+1), _dim));
            list.push_back(POS((row+1), (col), _dim));
        } else if (col == _dim - 1) {
            list.push_back(POS((row+1), (col), _dim));
            list.push_back(POS((row), (col-1), _dim));
            list.push_back(POS((row-1), (col), _dim));
        } else {
            list.push_back(POS((row-1), (col), _dim));
            list.push_back(POS((row), (col+1), _dim));
            list.push_back(POS((row+1), (col), _dim));
            list.push_back(POS((row), (col-1), _dim));
        }
    }
}
/**
 * Copy the possible moves around the _zeroPos to the input @param moves.
 * @param moves
 * @return 
 */
int GemPuzzleState::GetMoves(vector<int>& moves){
    assert(moves.empty() && "moves should be empty!\n");
    moves.insert(moves.begin(),_edges[_zeroPos].begin(),_edges[_zeroPos].end());
}
int GemPuzzleState::GetPlayoutMoves(vector<int>& moves){
    assert(moves.empty() && "moves should be empty!\n");
    for (int i = 0; i < _size; i++) {
        if(_fix[i]==-1)
            moves.push_back(i);
    }
    return moves.size();
}
/**
 * Swap the value of _zeroPos with @param move on the _board. Update the value
 * of _zeroPos.
 * @param move
 * @return 
 */
void GemPuzzleState::SetMove(int move){
    std::swap(_board[_zeroPos],_board[move]);
    _fix[_zeroPos]=1;
    _zeroPos = move;
    _pjm = WHITE;
}
void GemPuzzleState::SetPlayoutMoves(vector<int>& moves){
    std::cerr<<"GemPuzzleState::SetPlayoutMoves is not implemented!\n";
    exit(0);
//    int j=0;
//    for(int i=0;i<_fix.size();i++)
//    {
//        if(_fix[i]==-1){
//            std::swap(_board[i],_board[moves[j]]);
//            j++;
//        }
//    }
}
int GemPuzzleState::GetPlyJM(){
    return _pjm;
}
/**
 * Calculating Manhattan-distance between _board and _puzzle.
 * @return _reward 
 */
int GemPuzzleState::Evaluate(){
    for(int i=0;i<_size;i++){
        _reward += std::abs(_board[i]-_dstBoard[i]);
    }
    return _reward;
}
float GemPuzzleState::GetResult(int plyjm){
    return _srcReward/(float)_reward;
}
bool GemPuzzleState::IsTerminal(){
    int tmp=1;
    for(auto i:_board){
        if(i==tmp)
            tmp++;
    }
    if(tmp == (_size-1)){
        return true;
    }else{
        return false;
    }
}
void GemPuzzleState::Print() {
    printf("*---*---*---*---*\n");
    for (int i = 0; i < _dim; i++) {
        cout << "|" << setw(3) << _board[POS(i, 0, _dim)] <<
                "|" << setw(3) << _board[POS(i, 1, _dim)] <<
                "|" << setw(3) << _board[POS(i, 2, _dim)] <<
                "|" << setw(3) << _board[POS(i, 3, _dim)] <<
                "|" << std::endl;
        //printf("|%u  |%u  |%u  |%u  |\n", _board[POS(i,0,_dim)], _board[POS(i,1,_dim)], _board[POS(i,2,_dim)], _board[POS(i,3,_dim)]);
        printf("*---*---*---*---*\n");
    }
}

void GemPuzzleState::PrintToFile(char* FilaName){
    std::cerr<<"GemPuzzleState::PrintToFile is not implemented!\n";
    exit(0);
}
