/* 
 * File:   PGameTree.h
 * Author: SAM
 *
 * Created on July 8, 2014, 5:26 PM
 */

#include <vector>
#include <assert.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <stdio.h>
using namespace std;

#define RandInt(n) ((int)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandChar(n) ((char)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandFloat(n) ((float)(v)*rand()/(RAND_MAX+1.0))
#define MAX(n,m) ((n)>(m)?(n):(m))
#define INF 10000

#ifndef GAMETREE_H
#define	GAMETREE_H

class GameTree {
public:
    GameTree(int b, int d, float mv);
    GameTree(const GameTree& originalTree);
    ~GameTree();
    void GenTree(int seed=-1);
    void ResetTree();
    long int NextIdx(int c);
    long int PrevIdx();
    long int CurrIdx();
    bool GameOver();
    float IsWin();
    int GenMoves(vector<int>& moves);
    int DoMove(int move);
    int UndoMove();
    int MoveValue();
    float ResultValue(int v);
    int CurrDepth();
    long int GetKey();
    int PTM();
    void PrintGame(FILE *f);
private:
    int breath;
    int depth;
    int currDepth;
    size_t size;
    long int currIdx;
    float maxMoveVal;
    int val;
    vector<char>* moveVal;
    vector<long int> path;
};
#endif	/* GAMETREE_H */

