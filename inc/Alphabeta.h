#include <vector>
#include "paralleluct/state/GameTree.h"
#include "paralleluct/state/PGameState.h"
using namespace std;

#ifndef ALPHABETA_H
#define ALPHABETA_H

class Alphabeta{
public:
    Alphabeta();
    virtual ~Alphabeta();
    
    float ABNegamax2(PGameState& state,float alpha, float beta,int player,int& bestMove,bool verbose);
};
#endif