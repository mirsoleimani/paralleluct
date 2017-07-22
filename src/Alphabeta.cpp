/* 
 * File:   MCTS.cpp
 * Author: SAM
 * 
 * Created on July 10, 2014, 10:15 AM
 */

#include <bitset>
#include "Utilities.h"
#include "Alphabeta.h"
#include "paralleluct/state/PGameState.h"

Alphabeta::Alphabeta() {

}

Alphabeta::~Alphabeta(){
    
}

float Alphabeta::ABNegamax2(PGameState& state,float alpha,float beta,int player,int& bestMove,bool verbose){
//    vector<int> moves;
//    int n;
//    int bestVal=-INF;
//    int move,idx;
//    float val;
//    
//    if(state.GameOver()){
//        return player*state.GetValue();
//    }
//    n=state.GetMoves(moves);
//    
//    for(int i=0;i<n&&(alpha<beta);i++){
//        move=moves[i];
//        state.DoMove(move);
//        idx=state.currIdx;
//        val=-ABNegamax2(state,-beta,-alpha,-player,bestMove,verbose);
//        state.UndoMove();
//       
//        if(bestVal<val){
//            bestVal=val;
//            bestMove=move;
//        }
//        if(val > alpha) alpha=val;   
//        if(verbose){
//        cout<<"[Idx:"<<idx<<" M:"<<move<<" NVal:"<<val<<" A:"<<alpha<<" B:"<<beta<<endl;
//        }
//    }
//  
//    return bestVal;
}
