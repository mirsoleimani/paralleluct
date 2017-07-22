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
    GemPuzzleState(const char* str);
    GemPuzzleState(const GemPuzzleState& orig);
    virtual ~GemPuzzleState();

    void Reset();
    void SetMove(int move);
    void SetPlayoutMoves(vector<int>& moves);
    int GetMoves(vector<int>& moves);
    int GetPlayoutMoves(vector<int>& moves); 
    int GetPlyJM();
    int Evaluate();
    float GetResult(int plyjm);
    bool IsTerminal();
    void Print();
    void PrintToFile(char* fileName);
protected:
    int ParseToState(const char* str);
    void ParseFileGemPuzzle(char* fileName);
    void MakeBoard(const char* str);
    void MakeEdges(int row, int col, vector<int>& list);
private:
    std::vector<int> _srcBoard; //the original position of the game.
    std::vector<int> _board; //the current position of the game.
    std::vector<int> _dstBoard;
    std::vector<int> _fix; //keep track of the tiles that already played (+1).
    std::vector<std::vector<int>> _edges;
    int _zeroPos;
    int _srcZeroPos;
    int _dim;
    int _size;
    int _pjm;
    int _reward;
    int _srcReward;

};

#endif	/* GEMPUZZLESTATE_H */

