/* 
 * File:   PolyState.h
 * Author: mirsoleimanisa
 *
 * Created on January 8, 2016, 2:59 PM
 */

#include "Utilities.h"
#include "CSE.h"
#include <iostream>
#include <fstream>
using namespace std;

#ifndef POLYSTATE_H
#define	POLYSTATE_H

class PolyState{
public:
    PolyState(const polynomial poly);
    void Reset();
    void SetMove(int move);
    int GetMoves(term& moves);     //This method returns leftover variables
    void SetPlayoutMoves(vector<int>& moves);
    int GetPlayoutMoves(vector<int>& moves);    
    int GetPlyJM();
    float GetResult(int plyjm);
    int Evaluate();
    bool IsTerminal();
    void Print();
    void PrintPoly();
    
protected:
    int CountMultiplications();  

private:
    float _wReward;
    float _bReward;
    int _pjm;
    int _polyNumVars;
    int _polyNumTerms;
    int _polyNumOps;
    int _polyNumMult;
    int _polyForward;
    polynomial _poly;
    term _polyOrder;    // This is the order of the variables   
};
#endif	/* POLYSTATE_H */

