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

#ifndef UCT_H
#define	UCT_H

struct PlyOptions {
    int nthreads = 1;
    int nsims = 999999;
    float nsecs = 1.0;
    int par = 0;
    bool verbose = false;

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
    UCT(const PlyOptions opt, bool vb);
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    void UCTSearch(const T& state, int tid);
    void UCTSearchTreePar(T& rstate, int tid);

    UCTNode<T>* Select(UCTNode<T>* node, T& state, float cp);
    UCTNode<T>* Expand(UCTNode<T>* node, T& state);
    void Playout(T& state);
    void Backup(UCTNode<T>* node, T& state);

    void PrintSubTree(UCTNode<T>* root);
    void PrintTree();
    void PrintRootChildren();

    /*Multithreaing section*/
    void Run(T& state, int& m);
    void CreateNodeForState(T& state, UCTNode<T>* node);
    void Reset();

    typedef UCTNode<T>* UCTNodePointer;
    typedef typename vector<UCTNodePointer>::const_iterator const_iterator;
    typedef typename vector<UCTNodePointer>::iterator iterator;
private:
    //T *state;
    bool verbose;
    UCTNode<T>* sharedroot;
    std::vector<TimeOptions*> statistics;
    std::mutex mtx;
    std::mutex mtx2;
    PlyOptions plyOpt;
    std::vector<std::thread> threads;
};

template <class T>
UCT<T>::UCT(const PlyOptions opt, bool vb) : verbose(vb) {
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
void UCT<T>::Run(T& state, int& m) {

    sharedroot = new UCTNode<T>(0, NULL, state.PlyJustMoved());
    sharedroot->CreateChildren(state);

    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearchTreePar), std::ref(*this), std::ref(state), i));
            assert(threads[i].joinable());
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), state, i));
            assert(threads[i].joinable());
        }
    } else if (plyOpt.par == 0) {
        UCTSearch(state, 0);
    }

    /*Join the threads with the main thread*/
    if (threads.size() > 0) {
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    }

    /*Find the best node*/
    UCTNode<T>* n = sharedroot->UCTSelectChild(0);
    m = n->move;
    //PrintRootChildren();

    if (verbose) {
        cout << std::fixed << std::setprecision(2);
        cout << setw(10) << sharedroot->visits / 1.0 << "," <<
                setw(10) << statistics[0]->ttime << "," <<
                setw(10) << (statistics[0]->stime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->etime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->ptime / statistics[0]->ttime)*100 << "," <<
                setw(10) << (statistics[0]->btime / statistics[0]->ttime)*100 << "," << endl;
    }

    delete(sharedroot);
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
    threads.clear();
}

template <class T>
void UCT<T>::CreateNodeForState(T& state, UCTNode<T>* node) {
    node = new UCTNode<T>(0, NULL, state);
}

template <class T>
UCTNode<T>* UCT<T>::Select(UCTNode<T>* node, T& state, float cp) {
     //std::lock_guard<std::mutex> gaurd(mtx);
    
    UCTNode<T>* n = node;
    while (n->untriedMoves == 0 && n->children.size() > 0) {
        n = n->UCTSelectChild(cp);
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
UCTNode<T>* UCT<T>::Expand(UCTNode<T>* node, T& state) {
    // std::lock_guard<std::mutex> gaurd(mtx);
    
    UCTNode<T>* n = node->AddChild(state);
    return n;

    //    if (n->children.size() == 0) {
    //        n->CreateChildren(state);
    //    }
    //    
    //    if (n->untriedMoves > 0) {
    //        n = n->AddChild();
    //        state.DoMove(n->move);
    //    }
    //    return n;
}

template <class T>
void UCT<T>::Playout(T& state) {
    //std::lock_guard<std::mutex> gaurd(mtx);
    state.DoRandGame();
}

template <class T>
void UCT<T>::Backup(UCTNode<T>* node, T& state) {
      //  std::lock_guard<std::mutex> gaurd(mtx);
    
    UCTNode<T>* n = node;
    int winner = 0;
    if (state.GetResult(n->pjm)) {
        winner = n->pjm;
    } else if (state.GetResult(CLEAR - n->pjm)) {
        winner = CLEAR - n->pjm;
    }
    while (n != NULL) {
        //n->Update(state.GetResult(n->pjm));
        n->Update((n->pjm == winner) ? 1.0 : 0.0);
        n = n->parent;
    }
}

template <class T>
void UCT<T>::UCTSearchTreePar(T& rstate, int tid) {
    /*Create a copy of the current state for each thread*/
    T state = rstate;
    T lstate = state;
    int id = tid;
    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[id];

    double time = 0;

    int itr = 0;
    Timer tmr;

    while (itr < plyOpt.nsims && (timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs) {

        n = sharedroot;
        //n=roots[id];
        assert(n != NULL && "Root of the tree is zero!\n");

        time = tmr.elapsed();
        n = Select(n, lstate, 0.9);
        timeopt->stime += tmr.elapsed() - time;

        time = tmr.elapsed();
        n = Expand(n, lstate);
        timeopt->etime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Playout(lstate);
        timeopt->ptime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Backup(n, lstate);
        timeopt->btime += tmr.elapsed() - time;

        itr++;
        lstate = state;
    }

    if (verbose) {
        //PrintTree(m);
        //        cout << "Time to make move:" << t << " seconds" << endl;
        //        cout << "Time spends in select:" << selectTime << " second. %" << (selectTime / t)*100 << endl;
        //        cout << "Time spends in expand:" << expandTime << " second. %" << (expandTime / t)*100 << endl;
        //        cout << "Time spends in playout:" << playoutTime << " second. %" << (playoutTime / t)*100 << endl;
        //        cout << "Time spends in backup:" << backupTime << " second. %" << (backupTime / t)*100 << endl;
        //        cout << "Number of nodes generated per second:" << nnodes / time << " " << time << endl;

    }
}

template <class T>
void UCT<T>::UCTSearch(const T& state, int tid) {

    /*Create a copy of the current state for each thread*/
    //T state = rstate;
    T lstate = state;
    UCTNode<T>* myroot = new UCTNode<T>(0, NULL, lstate.PlyJustMoved());
    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[tid];

    //tmr.reset();
    double time;

    int itr = 0;
    Timer tmr;
//    double starttime=omp_get_wtime();

    while (itr < plyOpt.nsims && (timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs) {
        n = myroot;
        assert(n != NULL && "Root of the tree is zero!\n");

        time = tmr.elapsed();
        n = Select(n, lstate, 0.9);
        timeopt->stime += tmr.elapsed() - time;

        time = tmr.elapsed();
        n = Expand(n, lstate);
        timeopt->etime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Playout(lstate);
        timeopt->ptime += tmr.elapsed() - time;

        time = tmr.elapsed();
        Backup(n, lstate);
        timeopt->btime += tmr.elapsed() - time;

        itr++;
        lstate = state;
//        if(omp_get_wtime()-starttime >= plyOpt.nsecs){
//            break;
//        }
        //PrintTree();
    }

    mtx2.lock();
    //cout<<endl<<tid<<"my root "<<myroot->visits<<endl;
    sharedroot->wins += myroot->wins;
    sharedroot->visits += myroot->visits;
    for (int i = 0; i < sharedroot->children.size(); i++) {
        for (int j = 0; j < myroot->children.size(); j++) {
            if (sharedroot->children[i]->move == myroot->children[j]->move) {
                sharedroot->children[i]->wins += myroot->children[j]->wins;
                sharedroot->children[i]->visits += myroot->children[j]->visits;
            }
        }
    }
    mtx2.unlock();
    
    if (verbose) {
        //PrintTree(m);
        //PrintTree();
    }
    delete(myroot);
}
//
//undomoves{
//    //        int i = state.CurrIndicator();
////        while (i > d) {
////            state.UndoMove();
////            i--;
////        }
//}

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
    cout << endl;
    for (iterator itr = roots[0]->children.begin(); itr != roots[0]->children.end(); itr++) {
        cout << (*itr)->move << "," << (*itr)->visits << "," << (*itr)->wins / (*itr)->visits << " | ";
    }
    cout << endl;
}
#endif	/* UCT_H */

