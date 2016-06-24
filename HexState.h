/* 
 * File:   HexState.h
 * Author: mirsoleimanisa
 *
 * Created on 6 november 2014, 16:23
 */
#include "Utilities.h"


#ifndef HEXSTATE_H
#define	HEXSTATE_H

struct BItem{
    int val;
    int rank;
    int parent;
    bool top;
    bool bottom;
    bool left;
    bool right;
};

class HexGameState {
public:
    HexGameState();
    HexGameState(int d);
    HexGameState(const HexGameState& orig);
    HexGameState& operator=(const HexGameState& orig);
    HexGameState& operator =(HexGameState&& orig);
    virtual ~HexGameState();
    void Reset();
    void SetMove(int move);
    int GetPlyJM();
    int GetMoves(vector<int>& moves);
    void SetPlayoutMoves(vector<int>& moves);
    int GetPlayoutMoves(vector<int>& moves);   
    
    float GetResult(int plyjm);
    bool IsTerminal();
    void Print();
    void PrintToFile(char* fileName);
    void PrintDSet();
    void PrintDSet2(int pos);

    void MakeBoard();
    void MakeEdges(int row, int col, vector<int>& eList);
    float GetValue();
    int MoveValue();
    void ClearBoard();
    void ClearStone(int pos);
    int PutStone(int pos);
    
    void Evaluate();
    float EvaluateBoard(int plyjm, int direction);
    
    //dijkstra method
    float EvaluateDSet(int plyjm, int direction);
    float EvaluateDijkstra(int plyjm, int direction);
    
    //disjoint set method
    void MakeSet(int pos);
    void ClearSet(int pos);
    void UpdateSet();
    int Find(int pos);
    void Union(int pos1, int pos2);

        void UndoMoves(int beg);
        int GetMoveCounter();
    //    int UndoMove();
    //int CurrIndicator();
    
private:
    
    vector< vector<int> > edges;
    vector<BItem> dsboard;      //the board for using disjoint set data structure
    vector<int> lefPos;
    int dim;
    int size;
    int pjm;
    int moveCounter;
    float _bReward;
    float _wReward;

};

#endif	/* HEXSTATE_H */

