/* 
 * File:   UCTNode.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 16:38
 */

#include <vector>
#include <string>
#include <sstream>
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
    
    UCTNode(int m, UCTNode<T>* parent, T& state);
    UCTNode(const UCTNode<T>& orig);
    virtual ~UCTNode();
    UCTNode<T>* UCTSelectChild(float cp);
    UCTNode<T>* AddChild(int m,T& state);
    void Update(float result);
    string TreeToString(int indent);
    string NodeToString();
    string IndentString(int indent);   
    static bool CompareNode(UCTNode<T>* a,UCTNode<T>* b){
    return (a->val<b->val);
}
private:
    int move;
    double val;
    double wins;
    double visits;
    int pjm;
    UCTNode<T> *parent;
    vector<UCTNode<T>*> children;
    vector<int> untriedMoves;
};

template <class T>
UCTNode<T>::UCTNode(int m, UCTNode<T>* pr, T& state):move(m),visits(0),wins(0),children(NULL),parent(pr),pjm(state.PlyJustMoved()) {
    state.GetMoves(untriedMoves);
}

template <class T>
UCTNode<T>::UCTNode(const UCTNode<T>& orig) {
}

template <class T>
UCTNode<T>::~UCTNode() {
    for(const_iterator itr= children.begin();itr!= children.end();itr++)
        delete (*itr);
    children.clear();
    untriedMoves.clear();
}

template <class T>
UCTNode<T>* UCTNode<T>::UCTSelectChild(float cp){
    assert(!children.empty() && "can not select next move!");
    
    for (iterator itr = children.begin(); itr != children.end(); itr++) {
        (*itr)->val = (double((*itr)->wins) / (double((*itr)->visits)) + (cp*sqrt(2.0 * log(double(visits )) / (double((*itr)->visits)))));
    }
        return *max_element(children.begin(), children.end(), this->CompareNode);
}

template <class T>
UCTNode<T>* UCTNode<T>::AddChild(int m, T& state){
    untriedMoves.erase(std::remove(untriedMoves.begin(),untriedMoves.end(),m),untriedMoves.end());
    children.push_back(new UCTNode(m,this,state));
    return children.back();
}

template <class T>
void UCTNode<T>::Update(float result){
    this->wins+=result;
    this->visits+=1;
}

template <class T>
string UCTNode<T>::TreeToString(int indent){
    string s = IndentString(indent)+ NodeToString();
    for(iterator itr=children.begin();itr!=children.end();itr++){
        s+= (*itr)->TreeToString(indent+1);
    }
    return s;
}

template <class T>
string UCTNode<T>::NodeToString(){
    double v=wins/visits;
    return "[M:"+ NumToStr<int>(this->move)+" W/V:"+NumToStr<double>(wins)
            +"/"+NumToStr<double>(visits)+ "="+NumToStr<double>(v)+ "U:"+NumToStr<int>(untriedMoves.size());
}

template <class T>
string UCTNode<T>::IndentString(int indent){
    string s="\n";
    for(int i=0;i<indent;i++){
        s +="| ";
    }
    return s;
}
#endif	/* UCTNODE_H */

