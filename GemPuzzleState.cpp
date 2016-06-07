/* 
 * File:   FifteenPuzzleState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on June 3, 2016, 12:18 PM
 */

#include <ios>
#include <vector>
#include <thread>

#include "GemPuzzleState.h"

GemPuzzleState::GemPuzzleState(const char* str) {
    MakeBoard(str);
    _goal = _board;
    std::sort(_goal.begin(), _goal.end());
    std::fill(_fix.begin(),_fix.end(),-1);
}

GemPuzzleState::GemPuzzleState(const GemPuzzleState& orig) {
    _goal = orig._goal;
    _board = orig._board;
    _edges = orig._edges;
    _zeroPos = orig._zeroPos;
    _dim = orig._dim;
    _pjm = orig._pjm;
}

GemPuzzleState::~GemPuzzleState() {
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
    int size = std::strlen(str) - 1;
    assert((_size == 16 || _size == 25)&&"The state is not valid!\n");

    char* token;
    auto s = std::unique_ptr<char[]>(new char[std::strlen(str) + 1]); // +1 for the null terminator
    int i = 0;
    std::strcpy(s.get(), str);
    token = std::strtok(s.get(), ",");
    while (token) {
        assert(0 < atoi(token) < size && "The string is not valid!\n");
        _board.push_back(atoi(token));
        if (token == 0) {
            _zeroPos = i;
        }
        token = std::strtok(NULL, ",");
        i++;
    }
    return i;

}
void GemPuzzleState::MakeBoard(const char* str){
    
    _size = ParseToState(str);
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
            moves.push_back(_board[i]);
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
    _zeroPos = move;
    _pjm = WHITE;
}
void GemPuzzleState::SetPlayoutMoves(vector<int> moves){
    int j=0;
    for(int i=0;i<_fix.size();i++)
    {
        if(_fix[i]==-1){
            _fix[i]=1;
            _board[i] = moves[j];
            j++;
        }
    }
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
        _reward = std::abs(_board[i]-_goal[i]);
    }
    return _reward;
}
bool GemPuzzleState::IsTerminal(){
    
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