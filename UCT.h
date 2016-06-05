/* 
 * File:   UCT.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 16:32
 */

#include "PGameState.h"
#include "HexState.h"
#include "Utilities.h"
#include <thread>
#include <mutex>
#include <memory>
#include <omp.h>
#include <chrono>
#include "CheckError.h"
#include "mkl_vsl.h"
#include "mkl.h"
#include <tbb/task_group.h>

#ifndef UCT_H
#define	UCT_H

struct PlyOptions {
    int nthreads = 1;
    size_t nsims = 1048576;
    float nsecs = 999999;
    int par = 0;
    bool verbose = false;
    float cp = 1.0;
    int threadruntime=0;
};

struct TimeOptions {
    double ttime = 0.0;
    double stime = 0.0; //select
    double etime = 0.0; //expand
    double ptime = 0.0; //playout
    double btime = 0.0; //backup
    size_t nrand = 0; //number of random numbers are used in simulation
};

template <class T>
class UCT {
public:
    
        // <editor-fold defaultstate="collapsed" desc="node of the tree">

    class Node {
    public:
        typedef UCT<T>::Node* NodePtr;
        typedef typename std::vector<NodePtr>::const_iterator constItr;
        typedef typename std::vector<NodePtr>::iterator Itr;

        Node(int move, UCT<T>::Node* parent, int ply) : _move(move), _parent(parent),
        _pjm(ply), _visits(0), _wins(0), _untriedMoves(-1) {

        }
        Node(const Node& orig);

        ~Node() {
            for (constItr itr = _children.begin(); itr != _children.end(); itr++)
                delete (*itr);
            _children.clear();
        }

        /**
         * Check if the children of a node is created.
         * @return
         */
        bool IsParent() {
            if (_untriedMoves >= 0)
                return true;
            return false;
        }

        /**
         * Check if all the children of a node is expanded.
         * @return 
         */
        bool IsFullExpanded() {
            if (_untriedMoves == 0)
                return true;
            return false;
        }

        /**
         * Update the values of a node with reward that
         * comes from the evaluation of a non-terminal 
         * state after the playout operation.
         * @param result
         */
        void Update(int result) {
            _visits++;
            _wins += result;
        }

        void CreatChildren(vector<int>& moves, int pjm){
            _untriedMoves = moves.size();
            for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end();itr++) {
                _children.push_back(new Node(*itr, this, pjm));
            }
        }
        /**
         * Expand a new child from _children. 
         * _untriedMoves-1 will be the index 
         * of the new child.
         * @return A pointer to the new expanded child.
         */
        NodePtr AddChild() {
            //assert(_untriedMoves > 0 && "AddChild: There is no more child to expand!\n");
            return _children[--_untriedMoves];
        }
        //private:
        int _move;
        int _pjm;
        std::atomic<unsigned long long> _wins;
        std::atomic_int _visits;
        std::atomic_int _untriedMoves;
        NodePtr _parent;
        std::vector<NodePtr> _children;
    }; // </editor-fold>
    
    typedef UCT<T>::Node* NodePointer;
    typedef typename std::vector<NodePointer>::const_iterator const_iterator;
    typedef typename std::vector<NodePointer>::iterator iterator;
    UCT(const PlyOptions opt, int vb, const std::vector<unsigned int> seed);
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    
    void UCTSearch(const T& state, int sid, int rid, Timer& tmr);
    void UCTSearchtest(const T& state, int sid, int rid, Timer& tmr, TimeOptions& statistics, UCT<T>::Node* root);
    void UCTSearchCilkFor(const T& state, int sid, int rid, Timer& tmr);
    void UCTSearchOMPFor(const T& state, int sid, int rid, Timer& tmr);
    void UCTSearchTBBPipe(const T& state, int sid, int rid, Timer& tmr);

    UCT<T>::Node* Select(UCT<T>::Node* node, T& state, float cp);
    UCT<T>::Node* Expand(UCT<T>::Node* node, T& state, GEN& engine);
    void Playout(T& state, GEN& engine);
    void Backup(UCT<T>::Node* node, T& state);

    /*Vector functions*/
    UCT<T>::Node* SelectVecRand(UCT<T>::Node* node, T& state, float cp, int *random, int& randIndex);
    UCT<T>::Node* ExpandVecRand(UCT<T>::Node* node, T& state, int* random, int& randIndex);
    void PlayoutVecRand(T& state, int* random, int& randIndex);

    /*Print Functions*/
    void PrintSubTree(UCT<T>::Node* root);
    void PrintTree();
    void PrintRootChildren();
    void PrintStats_1test(std::string& log1,double total,const std::vector<TimeOptions> statistics);
    void PrintStats_1(std::string& log1,double total);
    void PrintStats_2(std::string& log2);

    /*Multithreaing section*/
    void Runt(const T& state, int& m, std::string &log1, std::string &log2);
    void Run(const T& state, int& m, std::string &log1, std::string &log2);
    void RunThreadPool(const T& state, int& m, std::string &log1, std::string &log2,boost::threadpool::pool& thread_pool);
    void RunCilk(const T& state, int& m, std::string &log1, std::string &log2);
    void RunTBB(const T& state, int& m, std::string &log1, std::string &log2);
    void RunCilkFor(const T& state, int& m, std::string &log1, std::string &log2);


    static bool SortChildern(UCT<T>::Node* a, UCT<T>::Node* b) {
        return (b->_move > a->_move);
    }
    
private:

    int verbose;
    UCT<T>::Node* sharedroot;
    std::vector<UCT<T>::Node*> roots;
    std::vector<TimeOptions*> statistics;
    PlyOptions plyOpt;
    std::mutex mtx;
    std::vector<ENG> gengine;    
    VSLStreamStatePtr gstream[NSTREAMS];
    

};
#include "UCT.cpp"
#endif	/* UCT_H */

