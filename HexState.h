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
    HexGameState(int d);
    HexGameState(const HexGameState& orig);
    HexGameState& operator=(const HexGameState& orig);
    virtual ~HexGameState();
    void NewGame();
    void DoMove(int move);
    void UndoMoves(int beg);
    int UndoMove();
    int PlyJustMoved();
    int GetMoves(vector<int>& moves);
    void DoRandGame(boost::mt19937& engine);
    float GetResult(int plyjm);
    bool GameOver();
    int CurrIndicator();
    void Print();
    void PrintDSet();
    void PrintDSet2(int pos);

    void MakeBoard();
    void MakeEdges(int row, int col, vector<int>& eList);
    float GetValue();
    int MoveValue();
    void ClearBoard();
    void ClearStone(int pos);
    int PutStone(int pos);
    
    float EvaluateBoard(int plyjm, int direction);
    float EvaluateDSet(int plyjm, int direction);
    float EvaluateDijkstra(int plyjm, int direction);
    //disjoint set
    void MakeSet(int pos);
    void ClearSet(int pos);
    void UpdateSet();
    int Find(int pos);
    void Union(int pos1,int pos2);
    
private:
    
    vector< vector<int> > edges;
    vector<BItem> dsboard;      //the board for using disjoint set data structure
    int dim;
    int size;
    int pjm;
    int moveCounter;
    //char x[56];
};

#endif	/* HEXSTATE_H */

