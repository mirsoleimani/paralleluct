/* 
 * File:   PolyState.h
 * Author: mirsoleimanisa
 *
 * Created on January 18, 2016, 6:09 PM
 */

#include "PolyState.h"

PolyState::PolyState(const polynomial poly) {
    _wReward = 0;
    _bReward = 0;
    _pjm = WHITE;
    _poly = poly;
    _polyNumTerms = _poly.size();
    _polyNumMult= CountMultiplications();
    _polyNumOps = _poly.size() + _polyNumMult;
    _polyNumVars = _poly.begin()->size() - 1; // number of vars. first is constant so -1
    _polyForward = 1;
}

int PolyState::CountMultiplications() {
    int mCount = 0;
    for (int i = 0; i < _polyNumTerms; i++) {
        int c = 1;
        for(int j=1;j<_poly[i].size();j++){
            c+=_poly[i][j];
        }
        if(c>0){
            mCount +=c-1;
        }
    }
    return mCount;
}

/**
 * Find the untried moves in the current state.
 * @param moves an empty vector
 * @return number of untried moves
 */
int PolyState::GetMoves(term& moves){
    assert(moves.size()==0&&"The moves vector should be empty!\n");
    std::vector<char> visited(_polyNumVars);
    
    for(int i=0;i<_polyOrder.size();i++){
        visited[_polyOrder[i]]=1;    //Mark the visited variables
    }
    
    for(int i=0;i<_polyNumVars;i++){
        if(!visited[i]){
            moves.push_back(i);
        }
    }
    
    assert(moves.size()<=size&&"The number of untried moves is out of bound!\n");
    return moves.size();
}

float PolyState::GetResult(int plyjm){
    if(plyjm==WHITE)
        return _wReward;
    else
        return _bReward;
}

int PolyState::Evaluate(){
        //Tree building assumes that var 0 is constant
    term order = _polyOrder;
    for(int i=0;i<_polyOrder.size();i++)
        order[i]++;
    
    if(!_polyForward)
        std::reverse(_polyOrder.begin(),_polyOrder.end());
    
    CSE cse;
    CSE::Node* root = cse.buildTree(_poly,order);
    
    unsigned int count = cse.countSimpleCSE(root);
    
    delete root;
    
    _wReward = count; // //(float)_polyNumOps;
    _bReward = count; // //(float)_polyNumOps;
    
    return count;
}

bool PolyState::IsTerminal(){
    if(_polyOrder.size()== _polyNumVars){
        return true;
    }else{
        return false;
    }       
}

int PolyState::GetPlyJM(){
    return _pjm;
}

void PolyState::SetMove(int move){
    //TODO: _polyOrder.Insert all moves at once is more efficient method for horner.
    _polyOrder.push_back(move);
}

void PolyState::Reset(){
    _wReward = 0;
    _bReward = 0;
    _pjm = WHITE;
    _polyOrder.clear();
    _polyNumTerms = _poly.size();
    _polyNumMult= CountMultiplications();
    _polyNumOps = _poly.size() + _polyNumMult;
    _polyNumVars = _poly.begin()->size() - 1; // number of vars. first is constant so -1
    _polyForward = 1;
}

void PolyState::Print()
{
    
}

void PolyState::PrintPoly(){
    std::string varnames = "abcdefghijklmnopqrstuvwxyz";
    BOOST_ASSERT(_poly[0].size() - 1 <= varnames.size());

    for (int i = 0; i < _poly.size(); i++) {
        std::cout << _poly[i][0]; // constant
        for (int j = 1; j < _poly[i].size(); j++) {
            if (_poly[i][j] > 0) {
                std::cout << "*";
                std::cout << varnames[j - 1];

                if (_poly[i][j] > 1) {
                    std::cout << "^" << _poly[i][j];
                }
            }
        }

        if (i < _poly.size() - 1) {
            std::cout << " + ";
        }
    }

    std::cout << std::endl;
}
