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

    UCTNode(int m, UCTNode<T>* parent,int plyjm);
    UCTNode(const UCTNode<T>& orig);
    virtual ~UCTNode();
    UCTNode<T>* UCTSelectChild(float cp);
    void CreateChildren(T& state,boost::mt19937& engine);
    UCTNode<T>* AddChild();
    UCTNode<T>* AddChild(T& state,boost::mt19937& engine);
    void Update(float result);
    string TreeToString(int indent);
    string NodeToString();
    string IndentString(int indent);

    static bool CompareNode(UCTNode<T>* a, UCTNode<T>* b) {
        return (a->val < b->val);
    }
private:
    int move;
    double val;
    double wins;
    double visits;
    int pjm;
    UCTNode<T> *parent;
    vector<UCTNode<T>*> children;
    int untriedMoves;

};

template <class T>
UCTNode<T>::UCTNode(int m, UCTNode<T>* pr, int plyjm) : move(m), visits(0), wins(0), children(NULL), parent(pr), pjm(plyjm) {
    untriedMoves = 99999;
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
void UCTNode<T>::CreateChildren(T& state, boost::mt19937& engine) {
    if (children.size() == 0) {
        vector<int> moves;
        state.GetMoves(moves);
      
        boost::uniform_int<int> uni_dist(0, moves.size() - 1);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<int> > rand(engine, uni_dist);
        boost::random_shuffle(moves,rand);
     
        untriedMoves = moves.size();
        for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); ++itr) {
            children.push_back(new UCTNode(*itr, this, CLEAR - state.PlyJustMoved()));
        }
        
//                        boost::uniform_int<int> uni_dist(0, children.size() - 1);
//        boost::variate_generator<boost::mt19937&, boost::uniform_int<int> > rand(engine, uni_dist);
//        std::random_shuffle(children.begin(), children.end(),rand);
        
        //boost::random_shuffle(children, rand);
                        
        //PRINT_ELEMENTS(moves,"original: ");


        //std::random_shuffle(children.begin(), children.end(),rand);
        //std::random_shuffle(children.begin(), children.end());
    }
}

//template <class T>
//UCTNode<T>* UCTNode<T>::AddChild() {    
//    UCTNode<T>* n = children[untriedMoves - 1];
//    untriedMoves--;
//    return n;
//}

template <class T>
UCTNode<T>* UCTNode<T>::UCTSelectChild(float cp) {
    assert(!children.empty() && "can not select next move!");
    for (iterator itr = children.begin(); itr != children.end(); itr++) {
        (*itr)->val = (double((*itr)->wins) / (double((*itr)->visits)) + (cp * sqrt(2.0 * log(double(visits)) / (double((*itr)->visits)))));
    }
    UCTNode<T> *n = *max_element(children.begin(), children.end(), this->CompareNode);
    return n;   
}

template <class T>
UCTNode<T>* UCTNode<T>::AddChild(T& state,boost::mt19937& engine) {
    UCTNode<T>* n = this;
    if (children.size() == 0) {
        vector<int> moves;
        state.GetMoves(moves);
        untriedMoves = moves.size();
        
//        boost::uniform_int<> uni_dist;
//        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > generator(engine, uni_dist);
//        std::random_shuffle(moves.begin(), moves.end(), generator);

                                boost::uniform_int<int> uni_dist(0, moves.size() - 1);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<int> > rand(engine, uni_dist);
        boost::random_shuffle(moves,rand);
        
        for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); ++itr) {
            children.push_back(new UCTNode(*itr, this, CLEAR - state.PlyJustMoved()));
        }
//                        boost::uniform_int<int> uni_dist(0, children.size() - 1);
//        boost::variate_generator<boost::mt19937&, boost::uniform_int<int> > rand(engine, uni_dist);
//        boost::random_shuffle(children, rand);
        
    } else if (n->untriedMoves > 0) {
        n = children[untriedMoves - 1];
        untriedMoves--;
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
void UCTNode<T>::Update(float result) {
    this->wins += result;
    this->visits ++;
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
    double v = wins / visits;
   // assert(untriedMoves <= 36);
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

