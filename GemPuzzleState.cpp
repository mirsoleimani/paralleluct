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

GemPuzzleState::GemPuzzleState() {
}

GemPuzzleState::GemPuzzleState(const GemPuzzleState& orig) {
}

GemPuzzleState::~GemPuzzleState() {
}

void GemPuzzleState::MakeBoard(){
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

int GemPuzzleState::GetPlyJMd(){
    return _pjm;
}

void GemPuzzleState::Print(){
    printf("*---*---*---*---*\n");
    for (int i = 0; i < _dim; i++) {
        printf("|%u  |%u  |%u  |%u  |\n", _board[POS(i,0,_dim)], _board[POS(i,1,_dim)], _board[POS(i,2,_dim)], _board[POS(i,3,_dim)]);
        printf("*---*---*---*---*\n");
    }
}