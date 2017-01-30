/* 
 * File:   PolyState.h
 * Author: mirsoleimanisa
 *
 * Created on January 8, 2016, 2:59 PM
 */

#include "Utilities.h"
#include "State.h"
#include "CSE.h"
#include <iostream>
#include <fstream>
using namespace std;

#ifndef POLYSTATE_H
#define	POLYSTATE_H

class PolyState : public State{
public:
    PolyState();
    PolyState(const polynomial poly);
    void Reset() override;
    void SetMove(int move)override;
    int GetMoves(term& moves) override; //This method returns leftover variables
    void SetPlayoutMoves(vector<int>& moves) override;
    int GetPlayoutMoves(vector<int>& moves) override;
    int GetPlyJM() override;
    float GetResult(int plyjm) override;
    void Evaluate() override;
    bool IsTerminal() override;
    int GetMoveCounter()override;
    void UndoMoves(int origMoveCounter) override;
    void Print() override;
    void PrintToFile(char* fileName) override;

protected:
    int CountMultiplications();

private:
    int _moveCounter;
    float _wReward;
    float _bReward;
    int _pjm;
    int _polyNumVars;
    int _polyNumTerms;
    int _polyNumOps;
    int _polyNumMult;
    int _polyForward;
//    vector<vector<int>> _poly;
    vector<int> _polyOrder; // This is the order of the variables   
};
#endif	/* POLYSTATE_H */

