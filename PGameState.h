/* 
 * File:   PGameState.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 15:42
 */

#include <vector>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include <iostream>
#include "Utilities.h"
using namespace std;
#ifndef PGAMESTATE_H
#define	PGAMESTATE_H

class PGameState {
public:
    PGameState(int b, int d, float maxmv, int seed);
    PGameState(const PGameState& orig);
    virtual ~PGameState();
    int DoMove(int move);
    int UndoMove();
    void UndoMoves(int beg);
    int GetMoves(vector<int>& moves, GEN& engine);
    int GetMoves(vector<int>& moves);
    int GetMoves();
    float GetResult(int pjm);
    bool GameOver();
    int PlyJustMoved();
    int CurrIndicator();
    float EvaluateBoardDSet(int plyjm, int direction);
    long int NextIdx(int c);
    long int PrevIdx();
    int GetRandMove();
    void DoRandGame(GEN& engine);
    float Result(int v);
    float GetValue();
    int MoveValue();
    void Print();
    string GameToString(int indent, int m);
    string StateToString(int m);
    string IndentString(int indent);
    void NewGame();
    long int currIdx;
    int val;
    int currDepth;
private:
    int breath;
    int depth;
    int pjm;
    float maxVal;

    size_t size;
    vector<char>* moveVal;
    long int* path;

};

#endif	/* PGAMESTATE_H */

