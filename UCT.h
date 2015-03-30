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
#include <thread>
#include <mutex>
#include <memory>
#include <omp.h>
#include <chrono>


#ifndef UCT_H
#define	UCT_H

struct PlyOptions {
    int nthreads = 1;
    int nsims = 9999999;
    float nsecs = 1.0;
    int par = 0;
    bool verbose = false;
    float cp = 1.0;
};

struct TimeOptions {
    double ttime = 0.0;
    double stime = 0.0; //select
    double etime = 0.0; //expand
    double ptime = 0.0; //playout
    double btime = 0.0; //backup
};

template <class T>
class UCT {
public:
    UCT(const PlyOptions opt, int vb);
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    void UCTSearch(const T& state, int sid, int rid, unsigned int seed);
    void UCTSearchTreePar(T& rstate, int tid, unsigned int seed);

    UCTNode<T>* Select(UCTNode<T>* node, T& state, float cp);
    UCTNode<T>* Expand(UCTNode<T>* node, T& state, GEN& engine);
    void Playout(T& state, GEN& engine);
    void Backup(UCTNode<T>* node, T& state);

    void PrintSubTree(UCTNode<T>* root);
    void PrintTree();
    void PrintRootChildren();

    /*Multithreaing section*/
    void Run(const T& state, int& m, const vector<unsigned int>& seed, string &log1, string &log2);
    void Reset();

    typedef UCTNode<T>* UCTNodePointer;
    typedef typename vector<UCTNodePointer>::const_iterator const_iterator;
    typedef typename vector<UCTNodePointer>::iterator iterator;

    static bool SortChildern(UCTNode<T>* a, UCTNode<T>* b) {
        return (b->move > a->move);
    }
private:
    int verbose;
    UCTNode<T>* sharedroot;
    vector<UCTNode<T>*> roots;
    std::vector<TimeOptions*> statistics;
    PlyOptions plyOpt;
    std::mutex mtx;
};

template <class T>
UCT<T>::UCT(const PlyOptions opt, int vb) : verbose(vb) {
    plyOpt = opt;
}

template <class T>
UCT<T>::UCT(const UCT<T>& orig) {
}

template <class T>
UCT<T>::~UCT() {

}

template <class T>
void UCT<T>::Reset() {

}

template <class T>
void UCT<T>::Run(const T& state, int& m, const vector<unsigned int>& seed, string& log1, string& log2) {
    assert(seed[0] > 0 && "seed should be larger than 0!");
    //ENG engine(seed[0]);

    std::vector<std::thread> threads;
    //std::vector<pthread_t> myThreads;
    T lstate(state);

    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), std::ref(lstate), i, 0, seed[i]));
            assert(threads[i].joinable());
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            assert(seed[i] > 0 && "seed can not be negative!\n");
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), std::ref(lstate), i, i, seed[i]));
            assert(threads[i].joinable());
        }
    } else if (plyOpt.par == 0) {
        assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        UCTSearch(lstate, 0, 0, seed[0]);
    }

    /*Join the threads with the main thread*/
    if (threads.size() > 0) {
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    }

    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->wins += roots[j]->wins;
            roots[0]->visits += roots[j]->visits;
            for (int i = 0; i < roots[0]->children.size(); i++) {
                for (int k = 0; k < roots[j]->children.size(); k++) {
                    if (roots[0]->children[i]->move == roots[j]->children[k]->move) {
                        roots[0]->children[i]->wins += roots[j]->children[k]->wins;
                        roots[0]->children[i]->visits += roots[j]->children[k]->visits;
                    }
                }
            }
        }
    }
    /*Find the best node*/
    UCTNode<T>* n = roots[0]->UCTSelectChild(0);
    m = n->move;

    if (verbose) {
        std::stringstream buffer;
        buffer << std::fixed << std::setprecision(2);
        buffer << setw(10) << roots[0]->visits << "," <<
                setw(10) << statistics[0]->ttime << "," <<
                setw(10) << (statistics[0]->stime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->etime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->ptime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->btime / statistics[0]->ttime)*100 << ",";
        log1 = buffer.str();
    }
    if (verbose == 3) {
        std::stringstream buffer;
        std::sort(roots[0]->children.begin(), roots[0]->children.end(), SortChildern);
        for (iterator itr = roots[0]->children.begin(); itr != roots[0]->children.end(); itr++) {
            buffer << /*(*itr)->move << "," <<*/ (*itr)->visits << ",";
        }
        log2 = buffer.str();
    }

    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
    threads.clear();
}

template <class T>
UCTNode<T>* UCT<T>::Select(UCTNode<T>* node, T& state, float cp) {
    
    UCTNode<T>* n = node;
    //while (n->GetUntriedMoves() == 0 && !n->children.empty()) {
    while(n->GetUntriedMoves()== 0){
        n = n->UCTSelectChild(cp);
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
UCTNode<T>* UCT<T>::Expand(UCTNode<T>* node, T& state, GEN& engine) {

    UCTNode<T>* n = NULL;
    n = node->AddChild(state,engine);
    assert(n != NULL && "The node to be expanded is NULL");
    
    return n;
}

template <class T>
void UCT<T>::Playout(T& state, GEN& engine) {

    state.DoRandGame(engine);
}

template <class T>
void UCT<T>::Backup(UCTNode<T>* node, T& state) {

    UCTNode<T>* n = node;
    int winner = 0;
    if (state.GetResult(n->pjm)) {
        winner = n->pjm;
    } else if (state.GetResult(CLEAR - n->pjm)) {
        winner = CLEAR - n->pjm;
    }
    while (n != NULL) {
        n->Update((n->pjm == winner) ? 1.0 : 0.0);
        n = n->parent;
    }
}

template <class T>
void UCT<T>::UCTSearch(const T& rstate, int sid, int rid, unsigned int seed) {
    ENG engine(seed);
    DIST dist(0,1);
    GEN gen(engine, dist);
    
    /*Create a copy of the current state for each thread*/
    T state(rstate);
    T lstate(state);

    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[sid];
    double time;

    int itr = 0;
    int max = plyOpt.nsims;
    Timer tmr;
    double sTime = tmr.second();
    while (itr < max && (timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs) {
        //    while (itr < max) {
        n = roots[rid];
        assert(n != NULL && "Root of the tree is zero!\n");

        time = tmr.elapsed();
        n = Select(n, lstate, plyOpt.cp);
        timeopt->stime += tmr.elapsed() - time;

        time = tmr.elapsed();
        n = Expand(n, lstate, gen);
        timeopt->etime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Playout(lstate, gen);
        timeopt->ptime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Backup(n, lstate);
        timeopt->btime += tmr.elapsed() - time;

        itr++;
//        if(sid==0){
//            PrintSubTree(roots[rid]);
//            char a;
//            cin>>a;
//        }
        lstate = state;
    }
}

template <class T>
void UCT<T>::PrintSubTree(UCTNode<T>* root) {
    cout << root->TreeToString(0) << endl;
}

template <class T>
void UCT<T>::PrintTree() {
    cout << roots[0]->TreeToString(0) << endl;
}

template <class T>
void UCT<T>::PrintRootChildren() {
    for (iterator itr = roots[0]->children.begin(); itr != roots[0]->children.end(); itr++) {
        //cout << (*itr)->move << "," << (*itr)->visits << "," << (*itr)->wins / (*itr)->visits << " | ";
        cout << (*itr)->move << "," << (*itr)->visits << ",";
    }
}
#endif	/* UCT_H */
//
//undomoves{
//    //        int i = state.CurrIndicator();
////        while (i > d) {
////            state.UndoMove();
////            i--;
////        }
//}




//template <class T>
//void UCT<T>::UCTSearchTreePar(T& rstate, int tid, unsigned int seed) {
//    ENG engine(seed);
//    /*Create a copy of the current state for each thread*/
//    T state = rstate;
//    T lstate = state;
//    int id = tid;
//    UCTNode<T>* n;
//    TimeOptions* timeopt = statistics[id];
//
//    double time = 0;
//
//    int itr = 0;
//    Timer tmr;
//
//    while (itr < plyOpt.nsims && (timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs) {
//
//        n = roots[0];
//        assert(n != NULL && "Root of the tree is zero!\n");
//
//        time = tmr.elapsed();
//        mtx.lock();
//        n = Select(n, lstate, plyOpt.cp);
//        mtx.unlock();
//        timeopt->stime += tmr.elapsed() - time;
//
//        time = tmr.elapsed();
//        mtx.lock();
//        n = Expand(n, lstate, engine);
//        mtx.unlock();
//        timeopt->etime += tmr.elapsed() - time;
//
//        time = tmr.elapsed();
//        mtx.lock();
//        Playout(lstate, engine);
//        mtx.unlock();
//        timeopt->ptime += tmr.elapsed() - time;
//
//        time = tmr.elapsed();
//        mtx.lock();
//        Backup(n, lstate);
//        mtx.unlock();
//        timeopt->btime += tmr.elapsed() - time;
//
//        itr++;
//        lstate = state;
//    }
//}