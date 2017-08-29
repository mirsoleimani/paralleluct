/* 
 * File:   PinsState.h
 * Author: Alfons Laarman
 *
 */


#include "Utilities.h"
//#include "CSE.h"
#include <iostream>
#include <fstream>
#include <climits>
using namespace std;


#ifndef PINS_STATE_H
#define	PINS_STATE_H

extern "C" {
#include "mc-lib/treedbs-ll.h"
}

class PinsState {
public:
    PinsState();
    PinsState (const char *fileName, int d, int swap, bool verbose);
    PinsState (const PinsState &pins);
//    bool operator==(PinsState const& rhs){
//        return state == rhs.state;
//    }
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


    bool operator < (const PinsState & s1){ return this->state < s1.state;}
    void Stats ();
    size_t GetState() const{
        return state;
    }
private:
    bool PropertyViolated();
    //int                *current = NULL;    // state of n slots
    tree_ref_t          state;
    int                 level;
    int                 playout = 0;
    float               cached;
    int                 cache = -1;
};

namespace std
{
    template<>
    struct hash<PinsState>
    {
        auto operator()(PinsState const& state) const -> size_t
        {
            return hash<int>{} (state.GetState());
        }
    };
}
#endif	/* PINS_STATE_H */

