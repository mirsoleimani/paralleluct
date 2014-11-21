/* 
 * File:   UCT.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 16:32
 */

#include "UCTNode.h"
#include "PGameState.h"
#include "HexState.h"
#include "Utilities.h"

#ifndef UCT_H
#define	UCT_H

template <class T>
class UCT{
public:
    UCT();
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    int UCTSearch(T& rootstate,int itr,bool verbose);
    int UCTSearchTreePar(T& rootstate,int itr,bool verbose);
    UCTNode<T>* Select(UCTNode<T>* node,T& state,float cp);
    UCTNode<T>* Expand(UCTNode<T>* node,T& state);
    void Playout(T& state);
    void Backup(UCTNode<T>* node,T& state);
    int NNodes();
    void PrintTree(int m);
private:
    int itr;
    T *state;
    double nnodes;
    UCTNode<T> *root;
};

template <class T>
UCT<T>::UCT() {
}

template <class T>
UCT<T>::UCT(const UCT<T>& orig) {
}

template <class T>
UCT<T>::~UCT() {

}

template <class T>
int UCT<T>::NNodes(){
    return nnodes;
}

template <class T>
UCTNode<T>* UCT<T>::Select(UCTNode<T>* node, T& state, float cp) {
    UCTNode<T>* n = node;
    while (n->untriedMoves.size() == 0 && n->children.size() > 0) {
        n = n->UCTSelectChild(cp);
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
UCTNode<T>* UCT<T>::Expand(UCTNode<T>* node, T& state) {
    UCTNode<T>* n = node;
    if (n->untriedMoves.size() > 0) {
        int m = n->untriedMoves[RandInt(n->untriedMoves.size()-1)];
        state.DoMove(m);
        n = n->AddChild(m, state);
        nnodes++;
    }
    return n;
}

template <class T>
void UCT<T>::Playout(T& state) {
    while (!state.GameOver()) {
        int m = state.GetRandMove();
        state.DoMove(m);
    }
}

template <class T>
void UCT<T>::Backup(UCTNode<T>* node, T& state) {
    UCTNode<T>* n = node;
    while (n != NULL) {
        n->Update(state.GetResult(n->pjm));
        n = n->parent;
    }
}

template <class T>
int UCT<T>::UCTSearch(T& rootstate, int itr, bool verbose) {
    root = new UCTNode<T>(0, NULL, rootstate);
    UCTNode<T>* n;
    T *state = &rootstate;
    int d = state->CurrIndicator();
    
    Timer tmr;
    double selectTime=0;
    double playoutTime=0;
    double expandTime=0;
    double backupTime=0;
    double time=0;
    this->nnodes=0;
    
    tmr.reset();
    for (int i = 0; i < itr; i++) {
        n = root;
        
        time = tmr.elapsed();
        n = Select(n, *state, 0.9);
        selectTime+=tmr.elapsed()-time;
        
        time=tmr.elapsed();
        n = Expand(n, *state);
        expandTime+=tmr.elapsed()-time;
        
        time = tmr.elapsed();
        Playout(*state);
        playoutTime+=tmr.elapsed()-time;
        
        time=tmr.elapsed();
        Backup(n, *state);
        backupTime+=tmr.elapsed()-time;

        int i = state->CurrIndicator();
        while (i > d) {
            state->UndoMove();
            i--;
        }
    }
    
    double t=tmr.elapsed();
    n = root->UCTSelectChild(0);
    int m = n->move;
    if (verbose) {
        //PrintTree(m);
        cout<<"Time to make move:"<<t<<" seconds"<<endl;
        cout<<"Time spends in select:"<<selectTime<<" second. %"<<(selectTime/t)*100<<endl;
        cout<<"Time spends in expand:"<<expandTime<<" second. %"<<(expandTime/t)*100<<endl;
        cout<<"Time spends in playout:"<<playoutTime<<" second. %"<<(playoutTime/t)*100<<endl;
        cout<<"Time spends in backup:"<<backupTime<<" second. %"<<(backupTime/t)*100<<endl;
        cout<<"Number of nodes in UCT tree:"<<this->nnodes<<endl;
        cout<<"Number of nodes generated per second:"<<this->nnodes/t<<endl;
        
    }

    delete(root);
    return m;
}

template <class T>
int UCT<T>::UCTSearchTreePar(T& rootstate, int itr, bool verbose) {
    root = new UCTNode(0, NULL, rootstate);
    T *state = &rootstate;
    int d = state->CurrIndicator();
    UCTNode* n;

    for (int i = 0; i < itr; i++) {
        n = root;
        n = Select(n, *state, 0.9);
        n = Expand(n, *state);
        Playout(*state);
        Backup(n, *state);

        int i = state->CurrIndicator();
        while (i > d) {
            state->UndoMove();
            i--;
        }
    }
    n = root->UCTSelectChild(0);
    int m = n->move;
    if (verbose) {
        PrintTree(m);
    }

    delete(root);
    return m;
}

template <class T>
void UCT<T>::PrintTree(int m) {
    cout << root->TreeToString(0) << endl;
}

#endif	/* UCT_H */

