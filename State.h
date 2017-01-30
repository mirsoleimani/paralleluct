/* 
 * File:   State.h
 * Author: mirsoleimanisa
 *
 * Created on February 22, 2016, 4:20 PM
 */

#ifndef STATE_H
#define	STATE_H

#include "Utilities.h"
#include <cstdio>
#include <vector>
using namespace std;
#define MoveType int

class State {
public:

    State() {
    }
    virtual void Reset() = 0;

    virtual void SetMove(MoveType move) = 0;
    virtual void SetPlayoutsMoves(vector<MoveType>& moves) = 0;

    virtual int GetMoves(vector<MoveType>& moves) = 0;
    virtual int GetPlayoutMoves(vector<MoveType>& moves) = 0;

    virtual void Evaluate() = 0;
    virtual float GetResult(int plyjm) = 0;

    virtual int PlyJM() = 0;
    virtual bool IsTerminal() = 0;
    virtual int GerMoveCounter() = 0;
    virtual void UndoMoves(int origMoveCounter) = 0;
    
    virtual void Print();
    virtual void PrintToFile(char* fileName);

};

#endif	/* STATE_H */

