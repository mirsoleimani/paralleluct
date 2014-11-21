/* 
 * File:   HexState.h
 * Author: mirsoleimanisa
 *
 * Created on 6 november 2014, 16:23
 */
#include "Utilities.h"

#ifndef HEXSTATE_H
#define	HEXSTATE_H

class HexGameState {
public:
    HexGameState(int d);
    HexGameState(const HexGameState& orig);
    virtual ~HexGameState();
    void NewGame();
    void DoMove(int move);
    int UndoMove();
    int PlyJustMoved();
    int GetMoves(vector<int>& moves);
    int GetRandMove();
    float GetResult(int plyjm);
    bool GameOver();
    int CurrIndicator();
    void Print();
    
    void MakeBoard();
    void MakeEdges(int row, int col, vector<int>& eList);
    float GetValue();
    int MoveValue();
    void ClearBoard();
    void ClearStone(int pos);
    int PutStone(int pos);
    float EvaluateBoard(int plyjm, int direction);
    
private:
    vector< vector<int> > edges;
    vector<int> board;
    vector<int> currMoves;
    vector<int> src;
    vector<int> dest;
    int dim;
    int size;
    int pjm;
    long int* path;
    int moveCounter;
};

#endif	/* HEXSTATE_H */

