/* 
 * File:   FifteenPuzzleState.h
 * Author: mirsoleimanisa
 *
 * Created on June 3, 2016, 12:18 PM
 * Implementation of 15 or 24 puzzle. We call it Gem Puzzle. 
*/


#include "Utilities.h"

#ifndef GEMPUZZLESTATE_H
#define	GEMPUZZLESTATE_H

class GemPuzzleState {
public:

    class Item {
    };
    GemPuzzleState();
    GemPuzzleState(const GemPuzzleState& orig);
    virtual ~GemPuzzleState();

    void Reset();
    void SetMove(int move);
    int GetMoves(vector<int>& moves); 
    int GetPlyJMd();
    int Evaluate();
    float GetResult(int plyjm);
    bool IsTerminal();
    void Print();
protected:
    void MakeBoard();
    void MakeEdges(int row,int col,vector<int>& list);
private:
    std::vector<int> _board;
    std::vector<std::vector<int>> _edges;
    int _zeroPos;
    int _dim;
    int _pjm;

};

#endif	/* GEMPUZZLESTATE_H */

