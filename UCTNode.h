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
    UCTNode<T>* UCTSelectChild(float cp);
    void CreateChildren(T& state, GEN& engine);
    bool ChildrenEmpty();
    int GetUntriedMoves();
    UCTNode<T>* AddChild(T& state, GEN& engine);
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
    //double wins;
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
void UCTNode<T>::CreateChildren(T& state, GEN& engine) {
    assert(children.size() == 0 && "Create The size of children is not zero!\n");
    vector<int> moves;
    state.GetMoves(moves, engine);
    untriedMoves = moves.size();
    //assert(untriedMoves > 0 && "there is no more moves!");
    if (untriedMoves > 0) {
        int plyToMove = CLEAR - state.PlyJustMoved();
#pragma unroll(16)
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
UCTNode<T>* UCTNode<T>::UCTSelectChild(float cp) {
    assert(!children.empty() && "children is empty. can not select next move!\n");
    float max_val = 0;
    float loc_val = 0;
    UCTNode<T> *n = NULL;
    float val1, val2;
    float l = 2.0 * log((float) this->visits);
#pragma unroll(16)
    for (iterator itr = children.begin(); itr != children.end(); itr++) {
        //assert((*itr)->visits>0 && "The children has not visited yet UCT Select Child.");
        val1 = (*itr)->wins / (float) ((*itr)->visits + 1);
        val2 = cp * sqrt(l / (float) ((*itr)->visits + 1));
        loc_val = val1 + val2;
        if (n == NULL || max_val < loc_val) {
            n = *itr;
            max_val = loc_val;
        }
    }

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