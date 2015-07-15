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
    int threadruntime=2;
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
    UCT(const PlyOptions opt, int vb, const vector<unsigned int> seed);
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    void UCTSearch(const T& state, int sid, int rid, Timer& tmr);
    void UCTSearchtest(const T& state, int sid, int rid, Timer& tmr,TimeOptions& statistics, UCTNode<T>* root);
    void UCTSearchCilkFor(const T& state, int sid, int rid, Timer& tmr);
    void UCTSearchOMPFor(const T& state, int sid, int rid, Timer& tmr);
    
    UCTNode<T>* Select(UCTNode<T>* node, T& state, float cp);
    UCTNode<T>* SelectVecRand(UCTNode<T>* node, T& state, float cp,int *random, int& randIndex);
    UCTNode<T>* Expand(UCTNode<T>* node, T& state, GEN& engine);
    UCTNode<T>* ExpandVecRand(UCTNode<T>* node, T& state, int* random, int& randIndex);
    void Playout(T& state, GEN& engine);
    void PlayoutVecRand(T& state, int* random, int& randIndex);
    void Backup(UCTNode<T>* node, T& state);

    /*Print Functions*/
    void PrintSubTree(UCTNode<T>* root);
    void PrintTree();
    void PrintRootChildren();
    void PrintStats_1test(string& log1,double total,const std::vector<TimeOptions> statistics);
    void PrintStats_1(string& log1,double total);
    void PrintStats_2(string& log2);

    /*Multithreaing section*/
    void Runt(const T& state, int& m, string &log1, string &log2);
    void Run(const T& state, int& m, string &log1, string &log2);
    void RunThreadPool(const T& state, int& m, string &log1, string &log2,boost::threadpool::pool& thread_pool);
    void RunCilk(const T& state, int& m, string &log1, string &log2);
    void RunTBB(const T& state, int& m, string &log1, string &log2);
    void RunCilkFor(const T& state, int& m, string &log1, string &log2);
    
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
    vector<ENG> gengine;    
    VSLStreamStatePtr gstream[NSTREAMS];
    

};

template <class T>
UCT<T>::UCT(const PlyOptions opt, int vb, vector<unsigned int> seed) : verbose(vb) {
    plyOpt = opt;
#ifdef VECRAND
    int errcode;
    for (int i = 0; i < NSTREAMS /*plyOpt.nthreads/*opt.nthreads*/; i++) {
        //errcode = vslNewStream(&stream, VSL_BRNG_NONDETERM, VSL_BRNG_RDRAND);
        errcode = vslNewStream(&gstream[i], VSL_BRNG_MT2203 + i, seed[0]);
        //errcode = vslNewStream(&gstream[i], VSL_BRNG_SFMT19937, seed[i]);
        CheckVslError(errcode);
    }
#else
    for(int i=0; i<plyOpt.nthreads;i++)
        gengine.push_back(ENG(seed[i]));
#endif
}

template <class T>
UCT<T>::UCT(const UCT<T>& orig) {

}

template <class T>
UCT<T>::~UCT() {
#ifdef VECRAND
    mkl_free_buffers();  
    int errcode;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        errcode = vslDeleteStream(&gstream[i]);
        CheckVslError(errcode);
    }
#endif
}

template <class T>
void UCT<T>::Reset() {
}

//template <class T>
//void UCT<T>::RunCilkFor(const T& state, int& m, string& log1, string& log2) {
//    T lstate(state);
//    double total=0;
//    for (int i = 0; i < plyOpt.nthreads; i++) {
//        statistics.push_back(new TimeOptions());
//    }
//
//    if (plyOpt.par == 1) {
//        /*tree parallelization*/
//        /*create the root*/
//        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
//
//        /*create threads*/
//        Timer tmr;
////        for (int i = 0; i < plyOpt.nthreads; i++) {
////            cilk_spawn UCTSearch(lstate,i,0,tmr);
////        }
//        UCTSearchCilkFor(lstate,0,0,tmr);
//
//    } else if (plyOpt.par == 2) {
//        /*root parallelization*/
//        /*create the roots among threads*/
//
////        for (int i = 0; i < plyOpt.nthreads; i++) {
////            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
////        }
////        /*create threads*/
////        Timer tmr;
////        for (int i = 0; i < plyOpt.nthreads; i++) {
////            cilk_spawn UCTSearch(lstate,i,i,tmr);
////        }
//        exit(0);
//    } else if (plyOpt.par == 0) {
//        //assert(seed[0] > 0 && "seed can not be negative!\n");
//        /*create the root*/
//        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
//        Timer tmr;
//        UCTSearch(lstate,0,0,tmr);
//    }
//
//    /*Join the threads with the main thread*/
//    //cilk_sync;
//
//    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
//        for (int j = 1; j < plyOpt.nthreads; j++) {
//            roots[0]->wins += roots[j]->wins;
//            roots[0]->visits += roots[j]->visits;
//            for (int i = 0; i < roots[0]->children.size(); i++) {
//                for (int k = 0; k < roots[j]->children.size(); k++) {
//                    if (roots[0]->children[i]->move == roots[j]->children[k]->move) {
//                        roots[0]->children[i]->wins += roots[j]->children[k]->wins;
//                        roots[0]->children[i]->visits += roots[j]->children[k]->visits;
//                    }
//                }
//            }
//        }
//    }
//
//    /*Find the best node*/
//    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
//    m = n->move;
//
//    if (verbose) {
//        PrintStats_1(log1,total);
//    }
//    if (verbose == 3) {
//        PrintStats_2(log2);
//    }
//
//    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
//        delete (*itr);
//    roots.clear();
//    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
//        delete (*itr);
//    statistics.clear();
//}

template <class T>
void UCT<T>::RunCilkFor(const T& state, int& m, string& log1, string& log2) {
    T lstate(state);
    double ttime =0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
        tmr.reset();
        cilk_for (int i = 0; i < plyOpt.nthreads; i++) {
             UCTSearch(lstate,i,0,tmr);
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        tmr.reset();
        cilk_for (int i = 0; i < plyOpt.nthreads; i++) {
            UCTSearch(lstate,i,i,tmr);
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        tmr.reset();
        UCTSearch(lstate,0,0,tmr);
    }

    /*Join the threads with the main thread*/
    ttime = tmr.elapsed();
    
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
    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
    m = n->move;


    if (verbose) {
        PrintStats_1(log1,ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
    }

    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
}

template <class T>
void UCT<T>::RunTBB(const T& state, int& m, string& log1, string& log2) {
    tbb::task_group g;
    T lstate(state);
    double ttime =0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            g.run(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));
        }
        
    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            g.run(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        tmr.reset();
        g.run(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));
    }

    /*Join the threads with the main thread*/
    g.wait();
    ttime=tmr.elapsed();
    
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
    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
    m = n->move;

    if (verbose) {
        PrintStats_1(log1,ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
    }

    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
}

template <class T>
void UCT<T>::RunCilk(const T& state, int& m, string& log1, string& log2) {
    T lstate(state);
    double ttime =0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            cilk_spawn UCTSearch(lstate,i,0,tmr);
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            cilk_spawn UCTSearch(lstate,i,i,tmr);
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        tmr.reset();
        cilk_spawn UCTSearch(lstate,0,0,tmr);
    }

    /*Join the threads with the main thread*/
    cilk_sync;
    ttime = tmr.elapsed();
    
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
    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
    m = n->move;


    if (verbose) {
        PrintStats_1(log1,ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
    }

    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
}

template <class T>
void UCT<T>::RunThreadPool(const T& state, int& m, string& log1, string& log2,boost::threadpool::pool& thread_pool) {
    T lstate(state);
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
       // Timer tmr;
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            thread_pool.schedule(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));
              

      }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        //Timer tmr;
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            thread_pool.schedule(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));
              

        }
    } else if (plyOpt.par == 0) {
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        //Timer tmr;
        tmr.reset();
        thread_pool.schedule(std::bind(std::mem_fn(&UCT::UCTSearch),std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));
    


    }

    /*Join the threads with the main thread*/
    thread_pool.wait(0);
    ttime = tmr.elapsed();

//    Timer tmr1;
    if (plyOpt.par == 2 /*&& plyOpt.nthreads > 1*/) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->wins += roots[j]->wins;
            roots[0]->visits += roots[j]->visits;
            int i=0;
            while(i<roots[0]->children.size()){
            //for (int i = 0; i < roots[0]->children.size(); i++) {
                for (int k = 0; k < roots[j]->children.size(); k++) {
                    if (roots[0]->children[i]->move == roots[j]->children[k]->move) {
                        roots[0]->children[i]->wins += roots[j]->children[k]->wins;
                        roots[0]->children[i]->visits += roots[j]->children[k]->visits;
                        i++;
                        break;
                    }
                }
            }
        }
    }
//        printf("\n***%f***\n",tmr1.elapsed());
    /*Find the best node*/
    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
    m = n->move;


    if (verbose) {
        PrintStats_1(log1,ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
    }
    
    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear();
}

template <class T>
void UCT<T>::Run(const T& state, int& m, string& log1, string& log2) {
    std::vector<std::thread> threads;
    double ttime=0;
    Timer tmr;
    T lstate(state);
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));
            assert(threads[i].joinable());
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));
            assert(threads[i].joinable());
        }
    } else if (plyOpt.par == 0) {
        /*create the root*/
        roots.push_back(new UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
        tmr.reset();
        threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearch), std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));
        assert(threads[0].joinable());
        //UCTSearch(lstate, 0, 0, seed[0],tmr);
    }

    /*Join the threads with the main thread*/
    if (threads.size() > 0) {
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    }
    ttime = tmr.elapsed();

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
    UCTNode<T>* n = roots[0]->UCTSelectChild(0,0);
    m = n->move;


    if (verbose) {
        PrintStats_1(log1,ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
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
UCTNode<T>* UCT<T>::Select(UCTNode<T>* node, T& state,float cp) {

    UCTNode<T>* n = node;
    //while (n->GetUntriedMoves() == 0 && !n->children.empty()) {
    while (n->GetUntriedMoves() == 0) {
        n = n->UCTSelectChild(cp,0);
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
UCTNode<T>* UCT<T>::SelectVecRand(UCTNode<T>* node, T& state, float cp,int* random, int &randIndex) {

    UCTNode<T>* n = node;
    //while (n->GetUntriedMoves() == 0 && !n->children.empty()) {
    while (n->GetUntriedMoves() == 0) {
        n = n->UCTSelectChild(cp,random[randIndex++]);
        state.DoMove(n->move);
    }
    return n;
}

template <class T>
UCTNode<T>* UCT<T>::Expand(UCTNode<T>* node, T& state, GEN& engine) {

    UCTNode<T>* n = NULL;
    n = node->AddChild(state, engine);
    assert(n != NULL && "The node to be expanded is NULL");

    return n;
}

template <class T>
UCTNode<T>* UCT<T>::ExpandVecRand(UCTNode<T>* node, T& state, int* random, int &randIndex) {

    UCTNode<T>* n = NULL;
    n = node->AddChild2(state, random, randIndex);
    assert(n != NULL && "The node to be expanded is NULL");

    return n;
}

template <class T>
void UCT<T>::Playout(T& state, GEN& engine) {

    state.DoRandGame(engine);
}

template <class T>
void UCT<T>::PlayoutVecRand(T& state, int* random, int& randIndex) {
    vector<int> moves;
    state.GetMoves(moves);

    vector<int>::iterator __first = moves.begin();
    vector<int>::iterator __last = moves.end();

    if (__first != __last)
        for (vector<int>::iterator __i = __first + 1; __i != __last; ++__i) {
            int rand = random[randIndex++];
            assert(rand >= 0 && "random number is negative.");
            std::iter_swap(__i, __first + (rand % ((__i - __first) + 1)));
        }

    int m = 0;
    while (!state.GameOver()) {
        state.DoMove(moves[m]);
        m++;
    }
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
void UCT<T>::UCTSearch(const T& rstate, int sid, int rid, Timer& tmr) {
    /*Create a copy of the current state for each thread*/
    T state(rstate);
    T lstate(state);

    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[sid];

    timeopt->nrand = 0;

#ifdef TIMING
    timeopt->stime = 0;
    timeopt->btime = 0;
    timeopt->ptime = 0;
    timeopt->etime = 0;
    double time = 0.0;
#endif

    int itr = 0;
    int max = ceil(plyOpt.nsims/(float)plyOpt.nthreads);

#ifdef VECRAND
    int umoves = lstate.GetMoves();
    int nRandItr = umoves * 2;
    int RAND_N = max*2*umoves;
    if(RAND_N > 16384*4)
        RAND_N = 16384*4;
    int random[RAND_N] __attribute__((aligned(SIMDALIGN))) ;
    //int * random = (int *)_mm_malloc(RAND_N*sizeof(int), SIMDALIGN);
    
    int errCode;
  
    int limit = RAND_N - nRandItr;
    int randIndex = 0;

    timeopt->nrand++;
    errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 0, RAND_MAX);
    CheckVslError(errCode);
#else
    DIST dist(0, 1);
    GEN gen(gengine[sid], dist);
#endif

    while ((timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs && itr < max) {

#ifdef VECRAND
        if (randIndex > limit) {
            timeopt->nrand++;
            errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 1, RAND_MAX);
            CheckVslError(errCode);
            randIndex = 0;
        }
#endif
        n = roots[rid];
        assert(n != NULL && "Root of the tree is zero!\n");

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = SelectVecRand(n, lstate, plyOpt.cp, random, randIndex);
#else
        n = Select(n, lstate, plyOpt.cp);
#endif        
#ifdef TIMING
        timeopt->stime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = ExpandVecRand(n, lstate, random, randIndex);
#else
        n = Expand(n, lstate, gen);
#endif
#ifdef TIMING
        timeopt->etime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        PlayoutVecRand(lstate, random, randIndex);
#else
        Playout(lstate, gen);
#endif
#ifdef TIMING
        timeopt->ptime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
        Backup(n, lstate);
#ifdef TIMING
        timeopt->btime += tmr.elapsed() - time;
#endif

        itr++;
#ifdef VECRAND
        assert(randIndex < RAND_N && "not enough random numbers");
#endif
        lstate = state;

    }
#ifdef VECRAND
    //_mm_free(random);
#endif
}

//template <class T>
//void UCT<T>::UCTSearchCilkFor(const T& rstate, int sid, int rid, Timer& tmr) {
//    /*Create a copy of the current state for each thread*/
//    T state(rstate);
////    T lstate(state);
//
//    //UCTNode<T>* n;
//    TimeOptions* timeopt = statistics[sid];
//    timeopt->nrand = 0;
//
//#ifdef TIMING
//    timeopt->stime = 0;
//    timeopt->btime = 0;
//    timeopt->ptime = 0;
//    timeopt->etime = 0;
//    double time = 0.0;
//#endif  
//    int max = plyOpt.nsims;
//    int umoves = state.GetMoves();
//    int nRandItr = umoves * 2;
//
//    int nblocks = 16384*8;
//    static const int blockSize = nblocks*nRandItr;//16384*4096;
//    
//    //int random[blockSize] __attribute__((aligned(SIMDALIGN))) ;
//    int * random = (int *)_mm_malloc(blockSize*sizeof(int), SIMDALIGN);
//    int errCode;
//
//    int itr = 0;
//    while (itr < ceil(max /(float) nblocks) && (timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs) {
//
//#ifdef VECRAND
//        
//        timeopt->nrand++;
//        int randstream = 4096;
//        int randblocksize = ceil(blockSize/randstream);
//        cilk_for(int i=0;i<randstream;i++){
//            errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[i], randblocksize, random+randblocksize*i, 0, RAND_MAX);
//            CheckVslError(errCode);
//        }
////        errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], blockSize, random, 0, RAND_MAX);
////        CheckVslError(errCode);
//#else
//    DIST dist(0, 1);
//    GEN gen(gengine[sid], dist);
//#endif
//
//        cilk_for (int i=0; i < nblocks; i++) {
//
//            T lstate=state;
//            int randIndex = i*nRandItr;
//            UCTNode<T>* n = roots[rid];
//            assert(n != NULL && "Root of the tree is zero!\n");
//
//#ifdef TIMING
//            time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//            n = SelectVecRand(n, lstate, plyOpt.cp, random, randIndex);
//#else
//            n = Select(n, lstate, plyOpt.cp);
//#endif        
//#ifdef TIMING
//            timeopt->stime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//            time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//            n = ExpandVecRand(n, lstate, random, randIndex);
//#else
//            n = Expand(n, lstate, gen);
//#endif
//#ifdef TIMING
//            timeopt->etime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//            time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//            PlayoutVecRand(lstate, random, randIndex);
//#else
//            Playout(lstate, gen);
//#endif
//#ifdef TIMING
//            timeopt->ptime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//            time = tmr.elapsed();
//#endif
//            Backup(n, lstate);
//#ifdef TIMING
//            timeopt->btime += tmr.elapsed() - time;
//#endif
//
//#ifdef VECRAND
//            assert(randIndex < blockSize && "not enough random numbers");
//            assert(randIndex < ((i+1)*nRandItr) && "not enough random numbers");
//#endif
////            T lstate(state);
//
//        }
//        itr++;
//    }
//     //timeopt->ttime = tmr.elapsed();
//    _mm_free(random);
//}

template <class T>
void UCT<T>::UCTSearchCilkFor(const T& rstate, int sid, int rid, Timer& tmr) {
    /*Create a copy of the current state for each thread*/
    T state(rstate);
    T lstate(state);

    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[sid];

    timeopt->nrand = 0;

#ifdef TIMING
    timeopt->stime = 0;
    timeopt->btime = 0;
    timeopt->ptime = 0;
    timeopt->etime = 0;
    double time = 0.0;
#endif

    int itr = 0;
    int max = ceil(plyOpt.nsims/(float)plyOpt.nthreads);

#ifdef VECRAND
    int umoves = lstate.GetMoves();
    int nRandItr = umoves * 2;
    int RAND_N = max*2*umoves;
    if(RAND_N > 16384*4)
        RAND_N = 16384*4;
    int random[RAND_N] __attribute__((aligned(SIMDALIGN))) ;
    //int * random = (int *)_mm_malloc(RAND_N*sizeof(int), SIMDALIGN);
    
    int errCode;
  
    int limit = RAND_N - nRandItr;
    int randIndex = 0;

    timeopt->nrand++;
    errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 0, RAND_MAX);
    CheckVslError(errCode);
#else
    DIST dist(0, 1);
    GEN gen(gengine[sid], dist);
#endif

    while ((timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs && itr < max) {

#ifdef VECRAND
        if (randIndex > limit) {
            timeopt->nrand++;
            errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 1, RAND_MAX);
            CheckVslError(errCode);
            randIndex = 0;
        }
#endif
        n = roots[rid];
        assert(n != NULL && "Root of the tree is zero!\n");

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = SelectVecRand(n, lstate, plyOpt.cp, random, randIndex);
#else
        n = Select(n, lstate, plyOpt.cp);
#endif        
#ifdef TIMING
        timeopt->stime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = ExpandVecRand(n, lstate, random, randIndex);
#else
        n = Expand(n, lstate, gen);
#endif
#ifdef TIMING
        timeopt->etime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        PlayoutVecRand(lstate, random, randIndex);
#else
        Playout(lstate, gen);
#endif
#ifdef TIMING
        timeopt->ptime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
        Backup(n, lstate);
#ifdef TIMING
        timeopt->btime += tmr.elapsed() - time;
#endif

        itr++;
#ifdef VECRAND
        assert(randIndex < RAND_N && "not enough random numbers");
#endif
        lstate = state;

    }
#ifdef VECRAND
    //_mm_free(random);
#endif
}

template <class T>
void UCT<T>::UCTSearchOMPFor(const T& rstate, int sid, int rid, Timer& tmr) {
    /*Create a copy of the current state for each thread*/
    T state(rstate);
    T lstate(state);

    UCTNode<T>* n;
    TimeOptions* timeopt = statistics[sid];
timeopt->nrand = 0;

#ifdef TIMING
    timeopt->stime = 0;
    timeopt->btime = 0;
    timeopt->ptime = 0;
    timeopt->etime = 0;    
    double time = 0.0;
#endif

    int itr = 0;
    int max = plyOpt.nsims;

#ifdef VECRAND
    __attribute__((aligned(64))) int random1[RAND_N];
    ;
    int errCode;

    int umoves = lstate.GetMoves();
    int limit = RAND_N - umoves * 20;
    int randIndex = 0;

    errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random1, 0, RAND_MAX);
    CheckVslError(errCode);
#else
    DIST dist(0, 1);
    GEN gen(gengine[sid], dist);
#endif

   for (itr=0;itr < max;itr++) {

#ifdef VECRAND
        if (randIndex >limit) {
            timeopt->nrand++;
            errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random1, 1, RAND_MAX);
            //printf("Generated %ld random numbers\nA[0]=%ld\n", RAND_N, random1[1]);
            CheckVslError(errCode);
            randIndex = 0;
        }
#endif
        n = roots[rid];
        assert(n != NULL && "Root of the tree is zero!\n"); 

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = SelectVecRand(n, lstate, plyOpt.cp,random1,randIndex);
#else
        n = Select(n, lstate, plyOpt.cp);
#endif        
#ifdef TIMING
        timeopt->stime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        n = ExpandVecRand(n, lstate, random1, randIndex);
#else
        n = Expand(n, lstate, gen);
#endif
#ifdef TIMING
        timeopt->etime += tmr.elapsed() - time;
#endif
        
#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef VECRAND
        PlayoutVecRand(lstate, random1, randIndex);
#else
        Playout(lstate, gen);
#endif
#ifdef TIMING
        timeopt->ptime += tmr.elapsed() - time;
#endif
        
#ifdef TIMING
        time = tmr.elapsed();
#endif
        Backup(n, lstate);
#ifdef TIMING
        timeopt->btime += tmr.elapsed() - time;
#endif

        itr++;
#ifdef VECRAND
        assert(randIndex < RAND_N && "not enough random numbers");
#endif
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

template <class T>
void UCT<T>::PrintStats_1(string& log1,double ttime){
    //double ttime = ttime2;
    double stime = 0;
    double etime = 0;
    double ptime = 0;
    double btime = 0;
    size_t nrand = 0;
    for (int t = 0; t < plyOpt.nthreads; t++) {
//    for (int t = 0; t < 1; t++) {
        //ttime += statistics[t]->ttime;
        assert(statistics[t]->stime >= 0.0 && "select time is negative.");
        //                        if(statistics[t]->stime==0.0)
        //                            cout<<t<<" "<<endl;
        stime += statistics[t]->stime;
        etime += statistics[t]->etime;
        ptime += statistics[t]->ptime;
        assert(statistics[t]->btime >= 0.0 && "select time is negative.");
        btime += statistics[t]->btime;
        nrand += statistics[t]->nrand;

    }
    stime/=plyOpt.nthreads;
    etime/=plyOpt.nthreads;
    ptime/=plyOpt.nthreads;
    btime/=plyOpt.nthreads;
    
    //ttime/=plyOpt.nthreads;
    std::stringstream buffer;
    buffer << std::fixed << std::setprecision(2);
    buffer << setw(10) << roots[0]->visits << "," <<
            //setw(10) << ttime / plyOpt.nthreads << "," <<
            setw(10) << ttime << "," <<
            setw(10) << (stime/(ttime))*100 << "," <<
            setw(10) << (etime/(ttime))*100 << "," <<
            setw(10) << (ptime/(ttime))*100 << "," <<
            setw(10) << (btime/(ttime))*100 << "," <<
            setw(10) << nrand << ",";
    log1 = buffer.str();
}

template <class T>
void UCT<T>::PrintStats_2(string& log2){
            std::stringstream buffer;
        std::sort(roots[0]->children.begin(), roots[0]->children.end(), SortChildern);
        for (iterator itr = roots[0]->children.begin(); itr != roots[0]->children.end(); itr++) {
            buffer << /*(*itr)->move << "," <<*/ (*itr)->visits << ",";
        }
        log2 = buffer.str();
}


//template <class T>
//void UCT<T>::Runt(const T& state, int& m, string& log1, string& log2) {
//    std::vector<std::thread> threads;
//    std::vector<TimeOptions> statisticsl;
//    vector<UCTNode<T>*> rootsl;
//    double ttime=0;
//    Timer tmr;
//    T lstate(state);
//    for (int i = 0; i < plyOpt.nthreads; i++) {
//        statisticsl.push_back(TimeOptions());
//    }
//
//    if (plyOpt.par == 1) {
//        /*tree parallelization*/
//        /*create the root*/
//        rootsl.push_back(UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
//
//        /*create threads*/
//        tmr.reset();
//        for (int i = 0; i < plyOpt.nthreads; i++) {
//            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearchtest), std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr),std::ref(statisticsl[i])));
//            assert(threads[i].joinable());
//        }
//
//    } else if (plyOpt.par == 2) {
//        /*root parallelization*/
//        /*create the rootsl among threads*/
//
//        for (int i = 0; i < plyOpt.nthreads; i++) {
//            rootsl.push_back(UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
//        }
//        /*create threads*/
//        tmr.reset();
//        for (int i = 0; i < plyOpt.nthreads; i++) {
//            threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearchtest), std::ref(*this), std::ref(lstate), i, i, std::ref(tmr),std::ref(statisticsl[i])));
//            assert(threads[i].joinable());
//        }
//    } else if (plyOpt.par == 0) {
//        /*create the root*/
//        rootsl.push_back(UCTNode<T>(0, NULL, lstate.PlyJustMoved()));
//        tmr.reset();
//        threads.push_back(std::thread(std::mem_fn(&UCT::UCTSearchtest), std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr),std::ref(statisticsl[0])));
//        assert(threads[0].joinable());
//        //UCTSearch(lstate, 0, 0, seed[0],tmr);
//    }
//
//    /*Join the threads with the main thread*/
//    if (threads.size() > 0) {
//        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
//    }
//    ttime = tmr.elapsed();
//
//    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
//        for (int j = 1; j < plyOpt.nthreads; j++) {
//            rootsl[0]->wins += rootsl[j]->wins;
//            rootsl[0]->visits += rootsl[j]->visits;
//            for (int i = 0; i < rootsl[0]->children.size(); i++) {
//                for (int k = 0; k < rootsl[j]->children.size(); k++) {
//                    if (rootsl[0]->children[i]->move == rootsl[j]->children[k]->move) {
//                        rootsl[0]->children[i]->wins += rootsl[j]->children[k]->wins;
//                        rootsl[0]->children[i]->visits += rootsl[j]->children[k]->visits;
//                    }
//                }
//            }
//        }
//    }
//
//    /*Find the best node*/
//    UCTNode<T>* n = rootsl[0]->UCTSelectChild(0,0);
//    m = n->move;
//
//
//    if (verbose) {
//        PrintStats_1test(log1,ttime,statisticsl);
//    }
//    if (verbose == 3) {
//        PrintStats_2(log2);
//    }
//
//    for (iterator itr = rootsl.begin(); itr != rootsl.end(); itr++)
//        delete (*itr);
//    rootsl.clear();
////    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
////        delete (*itr);
////    statistics.clear();
//    statistics.clear();
//    threads.clear();
//}
//
//template <class T>
//void UCT<T>::UCTSearchtest(const T& rstate, int sid, int rid, Timer& tmr, TimeOptions& timeopt,UCTNode<T>* root) {
//    /*Create a copy of the current state for each thread*/
//    T state(rstate);
//    T lstate(state);
//
//    UCTNode<T>* n;
////    TimeOptions* timeopt = statistics[sid];
//
//    timeopt.nrand = 0;
//
//#ifdef TIMING
//    timeopt.stime = 0;
//    timeopt.btime = 0;
//    timeopt.ptime = 0;
//    timeopt.etime = 0;
//    double time = 0.0;
//#endif
//
//    int itr = 0;
//    int max = ceil(plyOpt.nsims/(float)plyOpt.nthreads);
//
//#ifdef VECRAND
//    int umoves = lstate.GetMoves();
//   int nRandItr = umoves * 2;
//    //const int RAND_N = max*2*umoves;
//    //int * random = (int *)_mm_malloc(RAND_N*sizeof(int), SIMDALIGN);
//    
//    const int RAND_N = nRandItr*max;
//    //int random[RAND_N] __attribute__((aligned(SIMDALIGN))) ;
//    int * random = (int *)_mm_malloc(RAND_N*sizeof(int), SIMDALIGN);
//    
//    int errCode;
//  
//    int limit = RAND_N - umoves * 2;
//    int randIndex = 0;
//
//    timeopt.nrand++;
//    errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 0, RAND_MAX);
//    CheckVslError(errCode);
//#else
//    DIST dist(0, 1);
//    GEN gen(gengine[sid], dist);
//#endif
//
//    while ((timeopt.ttime = tmr.elapsed()) < plyOpt.nsecs && itr < max) {
//
//#ifdef VECRAND
//        if (randIndex > limit) {
//            timeopt.nrand++;
//            errCode = viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, gstream[sid], RAND_N, random, 1, RAND_MAX);
//            //printf("Generated %ld random numbers\nA[0]=%ld\n", RAND_N, random1[1]);
//            CheckVslError(errCode);
//            randIndex = 0;
//        }
//#endif
//        n = root;//roots[rid];
//        assert(n != NULL && "Root of the tree is zero!\n");
//
//#ifdef TIMING
//        time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//        n = SelectVecRand(n, lstate, plyOpt.cp, random, randIndex);
//#else
//        n = Select(n, lstate, plyOpt.cp);
//#endif        
//#ifdef TIMING
//        timeopt.stime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//        time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//        n = ExpandVecRand(n, lstate, random, randIndex);
//#else
//        n = Expand(n, lstate, gen);
//#endif
//#ifdef TIMING
//        timeopt.etime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//        time = tmr.elapsed();
//#endif
//#ifdef VECRAND
//        PlayoutVecRand(lstate, random, randIndex);
//#else
//        Playout(lstate, gen);
//#endif
//#ifdef TIMING
//        timeopt.ptime += tmr.elapsed() - time;
//#endif
//
//#ifdef TIMING
//        time = tmr.elapsed();
//#endif
//        Backup(n, lstate);
//#ifdef TIMING
//        timeopt.btime += tmr.elapsed() - time;
//#endif
//
//        itr++;
//#ifdef VECRAND
//        assert(randIndex < RAND_N && "not enough random numbers");
//#endif
//        lstate = state;
//
//    }
//#ifdef VECRAND
//    _mm_free(random);
//#endif
//}
//
//template <class T>
//void UCT<T>::PrintStats_1test(string& log1,double ttime,const std::vector<TimeOptions> statistics){
//    double ttime2=0;
//    double stime = 0;
//    double etime = 0;
//    double ptime = 0;
//    double btime = 0;
//    size_t nrand = 0;
//    for (int t = 0; t < plyOpt.nthreads; t++) {
////    for (int t = 0; t < 1; t++) {
//        ttime2 += statistics[t].ttime;
//        assert(statistics[t].stime >= 0.0 && "select time is negative.");
//        //                        if(statistics[t]->stime==0.0)
//        //                            cout<<t<<" "<<endl;
//        stime += statistics[t].stime;
//        etime += statistics[t].etime;
//        ptime += statistics[t].ptime;
//        assert(statistics[t].btime >= 0.0 && "select time is negative.");
//        btime += statistics[t].btime;
//        nrand += statistics[t].nrand;
//
//    }
//    //ttime/=plyOpt.nthreads;
//    std::stringstream buffer;
//    buffer << std::fixed << std::setprecision(2);
//    buffer << setw(10) << roots[0]->visits << "," <<
//            //setw(10) << ttime / plyOpt.nthreads << "," <<
//            setw(10) << ttime << "," <<
//            setw(10) << (stime / (ttime*plyOpt.nthreads))*100 << "," <<
//            setw(10) << (etime / (ttime*plyOpt.nthreads))*100 << "," <<
//            setw(10) << (ptime / (ttime*plyOpt.nthreads))*100 << "," <<
//            setw(10) << (btime / (ttime*plyOpt.nthreads))*100 << "," <<
//            setw(10) << ttime2/ plyOpt.nthreads << ",";
//    log1 = buffer.str();
//}

#endif	/* UCT_H */
//
//undomoves{
//    //        int i = state.CurrIndicator();
////        while (i > d) {
////            state.UndoMove();
////            i--;
////        }
//}