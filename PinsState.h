/* 
 * File:   PinsState.h
 * Author: Alfons Laarman
 *
 */


#include "Utilities.h"
#include "CSE.h"
#include <iostream>
#include <fstream>
using namespace std;

#ifndef PINS_STATE_H
#define	PINS_STATE_H

class PinsState {
public:
    PinsState();
    PinsState(const char *fileName);
    PinsState(const PinsState &pins);
    void Reset();
    void SetMove(int move);
    int GetMoves(term& moves); //This method returns leftover variables
    void SetPlayoutMoves(vector<int>& moves);
    int GetPlayoutMoves(vector<int>& moves);
    int GetPlyJM();
    float GetResult(int plyjm);
    void Evaluate();
    bool IsTerminal();
    int GetMoveCounter();
    void UndoMoves(int origMoveCounter);
    void Print();
    void PrintToFile(char* fileName);

private:
    bool PropertyViolated();
    int                *current;    // state of n slots
    int                 depth;
};
#endif	/* PINS_STATE_H */

