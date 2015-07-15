/* 
 * File:   UCTNode.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 16:38
 */

#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <omp.h>
#include "PGameState.h"
#ifndef UCTNODE_H
#define	UCTNODE_H

template <class T>
class UCTNode {
    template <typename T>
    friend class UCT;
public:
    typedef UCTNode<T>* UCTNodePointer;
    typedef typename vector<UCTNodePointer>::const_iterator const_iterator;
    typedef typename vector<UCTNodePointer>::iterator iterator;

    UCTNode(int m, UCTNode<T>* parent, int plyjm);
    UCTNode(const UCTNode<T>& orig);
    virtual ~UCTNode();
    UCTNode<T>* UCTSelectChild(float cp, int rand);
    void CreateChildren(T& state, GEN& engine);
    void CreateChildren2(T& state, int* random, int& randIndex);
    bool ChildrenEmpty();
    int GetUntriedMoves();
    UCTNode<T>* AddChild(T& state, GEN& engine);
    UCTNode<T>* AddChild2(T& state, int* random, int& randIndex);
    void Update(float result);
    string TreeToString(int indent);
    string NodeToString();
    string IndentString(int indent);

    //UCTNode<T>* UCTSelect(T& state,float cp);
    //UCTNode<T>* AddChild();
    //UCTNode<T>* Expand(T& state, ENG& engine);
    //    static bool CompareNode(UCTNode<T>* a, UCTNode<T>* b) {
    //        return (a->val < b->val);
    //    }
private:
    int move;
    int pjm;
    std::atomic_int wins;
    std::atomic_int visits;
    int untriedMoves;
    UCTNode<T> *parent;
    vector<UCTNode<T>*> children;
    std::mutex mtx;
};

template <class T>
UCTNode<T>::UCTNode(int m, UCTNode<T>* pr, int plyjm) : move(m), visits(0), wins(0), children(NULL), parent(pr), pjm(plyjm) {
    untriedMoves = 999999;
    
}

template <class T>
UCTNode<T>::UCTNode(const UCTNode<T>& orig) {
}

template <class T>
UCTNode<T>::~UCTNode() {
    for (const_iterator itr = children.begin(); itr != children.end(); itr++)
        delete (*itr);
    children.clear();
}

template <class T>
int UCTNode<T>::GetUntriedMoves(){
    //std::lock_guard<std::mutex> lock(mtx);
    return untriedMoves;
}

template <class T>
UCTNode<T>* UCTNode<T>::AddChild(T& state, GEN& engine) {
    std::lock_guard<std::mutex> lock(mtx);
    UCTNode<T>* n = this;
    if (children.empty()&& untriedMoves==999999) {
        CreateChildren(state, engine);
    }
    if(untriedMoves>0){
    int idx = --untriedMoves;
    n = children[idx];
    state.DoMove(n->move);
    }
    return n;
}
template <class T>
UCTNode<T>* UCTNode<T>::AddChild2(T& state, int* random, int& randIndex) {
    std::lock_guard<std::mutex> lock(mtx);
    UCTNode<T>* n = this;
    if (children.empty()&& untriedMoves==999999) {
        CreateChildren2(state, random,randIndex);
    }
    if(untriedMoves>0){
    int idx = --untriedMoves;
    n = children[idx];
    state.DoMove(n->move);
    }
    return n;
}

template <class T>
void UCTNode<T>::CreateChildren(T& state, GEN& engine) {
    assert(children.size() == 0 && "Create The size of children is not zero!\n");
    vector<int> moves;
    state.GetMoves(moves, engine);
    untriedMoves = moves.size();
    //assert(untriedMoves > 0 && "there is no more moves!");
    if (untriedMoves > 0) {
        int plyToMove = CLEAR - state.PlyJustMoved();
//#pragma unroll(16)
        for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); ++itr) {
            children.push_back(new UCTNode(*itr, this, plyToMove));
        }
    } else {
        untriedMoves = -1;
    }
}
template <class T>
void UCTNode<T>::CreateChildren2(T& state, int* random,int &randIndex) {
    assert(children.size() == 0 && "Create The size of children is not zero!\n");
    vector<int> moves;
    state.GetMoves(moves);
    untriedMoves = moves.size();
    
    vector<int>::iterator __first = moves.begin();
    vector<int>::iterator __last = moves.end();
    if (__first != __last)
        for (vector<int>::iterator __i = __first + 1; __i != __last; ++__i){
            //random[randIndex] = random[randIndex]+(*__i*2);
            int rand = random[randIndex++];
            std::iter_swap(__i, __first + (rand % ((__i - __first) + 1)));
        }
    if (untriedMoves > 0) {
        int plyToMove = CLEAR - state.PlyJustMoved();

        for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); ++itr) {
            children.push_back(new UCTNode(*itr, this, plyToMove));
        }
    } else {
        untriedMoves = -1;
    }
}

template <class T>
void UCTNode<T>::Update(float result) {
    wins += result;
    visits++;
}


template <class T>
UCTNode<T>* UCTNode<T>::UCTSelectChild(float cp,int rand) {
    assert(!children.empty() && "children is empty. can not select next move!\n");

//    UCTNode<T> *n = NULL;
    UCTNode<T> *n = children[rand%children.size()];

    float l = 2.0 * logf((float) this->visits);
    
#ifdef CILKSELECT
    std::vector<float> loc_val;
    loc_val.resize(children.size());
    cilk::reducer< cilk::op_max_index<unsigned, float> > best;
    
    for (int i = 0; i < children.size(); i++) {
        float val1 = children[i]->wins / (float) (children[i]->visits + 1);
        float val2 = cp * sqrtf(l / (float) (children[i]->visits + 1));
        loc_val[i] = (val1 + val2);
    }

    cilk_for(unsigned i = 0; i < loc_val.size(); ++i) {
        best->calc_max(i, loc_val[i]);
    }
    
    n = children[best->get_index_reference()];
#else

    float max_val2 = 0.0;
    int index = -1;
    std::vector<float> loc_val;
    loc_val.resize(children.size());

    for (int i = 0; i < children.size(); i++) {
        float val1 = children[i]->wins / (float) (children[i]->visits + 1);
        float val2 = cp * sqrtf(l / (float) (children[i]->visits + 1));
        loc_val[i] = (val1 + val2);
    }

    for (int i = 0; i < loc_val.size(); i++) {
        if (max_val2 < loc_val[i]) {
            index = i;
            max_val2 = loc_val[i];
        }
    }

    if (index >= 0)
        n = children[index];

    //version 2
    //        float max_val = 0.0;
    //        for (iterator itr = children.begin(); itr != children.end(); itr++) {
    //            //assert((*itr)->visits>0 && "The children has not visited yet UCT Select Child.");
    //            int visits = (*itr)->visits + 1;
    //            float val1 = (*itr)->wins / (float) (visits);
    //            float val2 = cp * sqrtf(l / (float) (visits));
    //            float loc_val = val1 + val2;
    //            if (n == NULL || max_val < loc_val) {
    //                n = *itr;
    //                max_val = loc_val;
    //            }
    //        }
#endif

    //version 1
    //    for (iterator itr = children.begin(); itr != children.end(); itr++) {
    //        (*itr)->val = (double((*itr)->wins) / (double((*itr)->visits)) + (cp * sqrt(2.0 * log(double(visits)) / (double((*itr)->visits)))));
    //    }
    //    UCTNode<T> *n2 = *max_element(children.begin(), children.end(), this->CompareNode);

    assert(n != NULL && "UCT can not select child!");

    return n;
}

template <class T>
string UCTNode<T>::TreeToString(int indent) {
    string s = IndentString(indent) + NodeToString();
    for (iterator itr = children.begin(); itr != children.end(); itr++) {
        s += (*itr)->TreeToString(indent + 1);
    }
    return s;
}

template <class T>
string UCTNode<T>::NodeToString() {
    //double v = wins / visits;
    double v = 888;
    return "[M:" + NumToStr<int>(this->move) + " W/V:" + NumToStr<double>(wins)
            + "/" + NumToStr<double>(visits) + "=" + NumToStr<double>(v) + "U:" + NumToStr<int>(untriedMoves);
}

template <class T>
string UCTNode<T>::IndentString(int indent) {
    string s = "\n";
    for (int i = 0; i < indent; i++) {
        s += "| ";
    }
    return s;
}
#endif	/* UCTNODE_H */



//template <class T>
//UCTNode<T>* UCTNode<T>::UCTSelect(T& state, float cp) {
//    //std::lock_guard<std::mutex> lock(mtx);
//    
//    UCTNode<T> *n = this;
//    UCTNode<T> *m = NULL;
//    //while (n->untriedMoves == 0 && !n->children.empty()) {
//    while (n->untriedMoves == 0 && n->childrenCreated) {
//        assert(!children.empty() && "children is empty. can not select next move!\n");
//        float max_val = 0;
//        float loc_val = 0;
//        float val1, val2;
//        float l = 2.0 * log((float) n->visits);
//        for (iterator itr = n->children.begin(); itr != n->children.end(); itr++) {
//            assert((*itr)->visits > 0 && "The children has not visited yet.");
//
//            val1 = (*itr)->wins / (float) (*itr)->visits;
//            val2 = cp * sqrt(l / (float) (*itr)->visits);
//            loc_val = val1 + val2;
//            if (m == NULL || max_val < loc_val) {
//                m = *itr;
//                max_val = loc_val;
//            }
//        }
//        assert(m != NULL && "UCT can not select child m!");
//        n=m;
//        state.DoMove(n->move);
//    }
//    
//    assert(n != NULL && "UCT can not select child n!");
//
//    return n;
//}

//template <class T>
//UCTNode<T>* UCTNode<T>::Expand(T& state, ENG& engine) {
//    std::lock_guard<std::mutex> lock(mtx);
//    //if (children.empty()) {
//    if(!childrenCreated){
//        //CreateChildren(state, engine);
//
//        vector<int> moves;
//        state.GetMoves(moves, engine);
//        untriedMoves = moves.size();
//        int plyToMove = CLEAR - state.PlyJustMoved();
//        for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); ++itr) {
//            children.push_back(new UCTNode(*itr, this, plyToMove));
//        }
//        childrenCreated = 1;
//    }
//     UCTNode<T>* n = this;
//    if (untriedMoves > 0) {
//        n = children[--untriedMoves];
//        state.DoMove(n->move);
//    }
//
//    return n;
//}