/* 
 * File:   SampleGameState.h
 * Author: mirsoleimanisa
 *
 * Created on February 2, 2017, 6:28 PM
 */

#pragma offload_attribute (push, _Cilk_shared)
#include <vector>
#include "offload.h"
#pragma offload_attribute (pop)
using namespace std;
#define SHAREDVEC(T) _Cilk_shared vector<T, __offload::shared_allocator<T> > * _Cilk_shared

#ifndef SAMPLEGAMESTATE_H
#define	SAMPLEGAMESTATE_H

class SampleGameState {
public:
    SampleGameState();
    SampleGameState(const SampleGameState& orig);
    virtual ~SampleGameState();

    void Reset(){}
    void SetMove(int move){}
    int GetPlyJM(){return 1;}
    int GetMoves(SHAREDVEC(int)& moves){moves=_moves;}
    void SetPlayoutMoves(SHAREDVEC(int)& moves){_moves=moves;}
    int GetPlayoutMoves(SHAREDVEC(int)& moves){moves=_moves;}

    float GetResult(int plyjm){return _reward;}
    bool IsTerminal(){return 1;}
    void Print(){;}
    void PrintToFile(char* fileName){;}
    void Evaluate(){_reward=1;}

    void UndoMoves(int beg){;}
    int GetMoveCounter(){;}

private:
    SHAREDVEC(int) _moves;
    int _reward;
};

#endif	/* SAMPLEGAMESTATE_H */

