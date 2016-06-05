#include <algorithm>

#include "UCT.h"

// <editor-fold defaultstate="collapsed" desc="Con/Destruction">

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
    for (int i = 0; i < plyOpt.nthreads; i++)
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

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Run">

template <class T>
void UCT<T>::RunCilkFor(const T& state, int& m, string& log1, string& log2) {
    T lstate(state);
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));

        /*create threads*/
        tmr.reset();

        cilk_for(int i = 0; i < plyOpt.nthreads; i++) {
            UCTSearch(lstate, i, 0, tmr);
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        /*create threads*/
        tmr.reset();

        cilk_for(int i = 0; i < plyOpt.nthreads; i++) {
            UCTSearch(lstate, i, i, tmr);
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        UCTSearch(lstate, 0, 0, tmr);
    }

    /*Join the threads with the main thread*/
    ttime = tmr.elapsed();

    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->_wins += roots[j]->_wins;
            roots[0]->_visits += roots[j]->_visits;
            for (int i = 0; i < roots[0]->_children.size(); i++) {
                for (int k = 0; k < roots[j]->_children.size(); k++) {
                    if (roots[0]->_children[i]->_move == roots[j]->_children[k]->_move) {
                        roots[0]->_children[i]->_wins += roots[j]->_children[k]->_wins;
                        roots[0]->_children[i]->_visits += roots[j]->_children[k]->_visits;
                    }
                }
            }
        }
    }

    /*Find the best node*/
    T rootstate(state);
    UCT<T>::Node* n = UCT<T>::Select(roots[0], rootstate, 0);
    m = n->_move;


    if (verbose) {
        PrintStats_1(log1, ttime);
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
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));
    }

    /*Join the threads with the main thread*/
    g.wait();
    ttime = tmr.elapsed();

    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->_wins += roots[j]->_wins;
            roots[0]->_visits += roots[j]->_visits;
            for (int i = 0; i < roots[0]->_children.size(); i++) {
                for (int k = 0; k < roots[j]->_children.size(); k++) {
                    if (roots[0]->_children[i]->_move == roots[j]->_children[k]->_move) {
                        roots[0]->_children[i]->_wins += roots[j]->_children[k]->_wins;
                        roots[0]->_children[i]->_visits += roots[j]->_children[k]->_visits;
                    }
                }
            }
        }
    }

    /*Find the best node*/
    T rootstate(state);
    UCT<T>::Node* n = UCT<T>::Select(roots[0], rootstate, 0);
    m = n->_move;

    if (verbose) {
        PrintStats_1(log1, ttime);
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
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            cilk_spawn UCTSearch(lstate, i, 0, tmr);
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            cilk_spawn UCTSearch(lstate, i, i, tmr);
        }
    } else if (plyOpt.par == 0) {
        //assert(seed[0] > 0 && "seed can not be negative!\n");
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        cilk_spawn UCTSearch(lstate, 0, 0, tmr);
    }

    /*Join the threads with the main thread*/
    cilk_sync;
    ttime = tmr.elapsed();

    if (plyOpt.par == 2 && plyOpt.nthreads > 1) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->_wins += roots[j]->_wins;
            roots[0]->_visits += roots[j]->_visits;
            for (int i = 0; i < roots[0]->_children.size(); i++) {
                for (int k = 0; k < roots[j]->_children.size(); k++) {
                    if (roots[0]->_children[i]->_move == roots[j]->_children[k]->_move) {
                        roots[0]->_children[i]->_wins += roots[j]->_children[k]->_wins;
                        roots[0]->_children[i]->_visits += roots[j]->_children[k]->_visits;
                    }
                }
            }
        }
    }

    /*Find the best node*/
    T rootstate(state);
    UCT<T>::Node* n = UCT<T>::Select(roots[0], rootstate, 0);
    m = n->_move;


    if (verbose) {
        PrintStats_1(log1, ttime);
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
void UCT<T>::RunThreadPool(const T& state, int& m, string& log1, string& log2, boost::threadpool::pool& thread_pool) {
    T lstate(state);
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));

        /*create threads*/
        // Timer tmr;
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));


        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        /*create threads*/
        //Timer tmr;
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));


        }
    } else if (plyOpt.par == 0) {
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        //Timer tmr;
        tmr.reset();
        thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));



    }

    /*Join the threads with the main thread*/
    thread_pool.wait(0);
    ttime = tmr.elapsed();

    //    Timer tmr1;
    if (plyOpt.par == 2 /*&& plyOpt.nthreads > 1*/) {
        for (int j = 1; j < plyOpt.nthreads; j++) {
            roots[0]->_wins += roots[j]->_wins;
            roots[0]->_visits += roots[j]->_visits;
            int i = 0;
            while (i < roots[0]->_children.size()) {
                //for (int i = 0; i < roots[0]->_children.size(); i++) {
                for (int k = 0; k < roots[j]->_children.size(); k++) {
                    if (roots[0]->_children[i]->_move == roots[j]->_children[k]->_move) {
                        roots[0]->_children[i]->_wins += roots[j]->_children[k]->_wins;
                        roots[0]->_children[i]->_visits += roots[j]->_children[k]->_visits;
                        i++;
                        break;
                    }
                }
            }
        }
    }
    //        printf("\n***%f***\n",tmr1.elapsed());
    /*Find the best node*/
    T rootstate(state);
    UCT<T>::Node* n = UCT<T>::Select(roots[0], rootstate, 0);
    m = n->_move;


    if (verbose) {
        PrintStats_1(log1, ttime);
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
    double ttime = 0;
    Timer tmr;
    T lstate(state);
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }

    if (plyOpt.par == 1) {
        /*tree parallelization*/
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));

        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, 0, std::ref(tmr)));
            assert(threads[i].joinable());
        }

    } else if (plyOpt.par == 2) {
        /*root parallelization*/
        /*create the roots among threads*/

        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        /*create threads*/
        tmr.reset();
        for (int i = 0; i < plyOpt.nthreads; i++) {
            threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), i, i, std::ref(tmr)));
            assert(threads[i].joinable());
        }
    } else if (plyOpt.par == 0) {
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::ref(lstate), 0, 0, std::ref(tmr)));
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
            roots[0]->_wins += roots[j]->_wins;
            roots[0]->_visits += roots[j]->_visits;
            for (int i = 0; i < roots[0]->_children.size(); i++) {
                for (int k = 0; k < roots[j]->_children.size(); k++) {
                    if (roots[0]->_children[i]->_move == roots[j]->_children[k]->_move) {
                        roots[0]->_children[i]->_wins += roots[j]->_children[k]->_wins;
                        roots[0]->_children[i]->_visits += roots[j]->_children[k]->_visits;
                    }
                }
            }
        }
    }

    /*Find the best node*/
    T rootState(state);
    Node* n = UCT<T>::Select(roots[0], rootState, 0);
    m = n->_move;


    if (verbose) {
        PrintStats_1(log1, ttime);
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

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="MCTS steps">

template <class T>
UCT<T>::Node* UCT<T>::Select(UCT<T>::Node* node, T& state, float cp) {

    UCT<T>::Node* n = node;
    while (n->IsFullExpanded()) {
        // <editor-fold defaultstate="collapsed" desc="initializing">
        float l = 2.0 * logf((float) n->_visits);
        int index = -1;
        std::vector<float> UCT; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="calculate UCT value for all children">
        for (iterator itr = n->_children.begin(); itr != n->_children.end(); itr++) {
            float exploit = (*itr)->_wins / (float) ((*itr)->_visits);
            float explore = cp * sqrtf(l / (float) ((*itr)->_visits));
            UCT.push_back(exploit + explore);
        }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="find a child with max UCT value">
        index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end()));   //TODO: why max is better than min?
        n = n->_children[index]; // </editor-fold>

        //TODO: Add child to current trajectory
        state.SetMove(n->_move);
    }
    return n;

}

template <class T>
UCT<T>::Node* UCT<T>::SelectVecRand(UCT<T>::Node* node, T& state, float cp, int* random, int &randIndex) {

//    UCT<T>::Node* n = node;
//    //while (n->GetUntriedMoves() == 0 && !n->_children.empty()) {
//    while (n->GetUntriedMoves() == 0) {
//        n = n->UCTSelectChild(cp, random[randIndex++]);
//        state.SetMove(n->_move);
//    }
//    return n;
}

template <class T>
UCT<T>::Node* UCT<T>::Expand(UCT<T>::Node* node, T& state, GEN& engine) {

    UCT<T>::Node* n = node;
    if (!state.IsTerminal()) {
        //Check if n has already children
        if (!n->IsParent()) {
            // <editor-fold defaultstate="collapsed" desc="create childern for n based on the current state">
            vector<int> moves;
            //set the number of untried moves for n based on the current state
            state.GetMoves(moves);
            std::random_shuffle(moves.begin(), moves.end(), engine);
            n->CreatChildren(moves,(state.GetPlyJM()==WHITE)?WHITE:BLACK);
            // </editor-fold>

        }

        //TODO: Appending child could be happened in update phase.
        // <editor-fold defaultstate="collapsed" desc="add new node child to n">
        if (!n->IsFullExpanded()) {
            //TODO: Append new node the trajectory
            n = n->AddChild();
            assert(n!=NULL&&"AddChild should not return a NULL pointer!\n");
            state.SetMove(n->_move);
        }// </editor-fold>
    }
    return n;
}

template <class T>
UCT<T>::Node* UCT<T>::ExpandVecRand(UCT<T>::Node* node, T& state, int* random, int &randIndex) {

//    UCT<T>::Node* n = NULL;
//    n = node->AddChild2(state, random, randIndex);
//    assert(n != NULL && "The node to be expanded is NULL");
//
//    return n;
}

template <class T>
void UCT<T>::Playout(T& state, GEN& engine) {

    vector<int> moves;
    state.GetMoves(moves); //TODO: Rename GetMoves to GetUntriedMoves
    std::random_shuffle(moves.begin(), moves.end(), engine);

    // <editor-fold defaultstate="collapsed" desc="perform a simulation until a terminal state is reached">
    int m = 0;
    while (!state.IsTerminal()) { 
        state.SetMove(moves[m]);
        m++;
    }// </editor-fold>

    //TODO: Evaluate the current state
    state.Evaluate();
}

template <class T>
void UCT<T>::PlayoutVecRand(T& state, int* random, int& randIndex) {
//    vector<int> moves;
//    state.GetMoves(moves);
//
//    vector<int>::iterator __first = moves.begin();
//    vector<int>::iterator __last = moves.end();
//
//    if (__first != __last)
//        for (vector<int>::iterator __i = __first + 1; __i != __last; ++__i) {
//            int rand = random[randIndex++];
//            assert(rand >= 0 && "random number is negative.");
//            std::iter_swap(__i, __first + (rand % ((__i - __first) + 1)));
//        }
//
//    int m = 0;
//    while (!state.GameOver()) {
//        state.SetMove(moves[m]);
//        m++;
//    }
}

template <class T>
void UCT<T>::Backup(UCT<T>::Node* node,T& state) {
    //TODO: Using trajectory makes it possible to do vectorization.
    UCT<T>::Node* n = node;
    float rewardWhite = state.GetResult(WHITE);
    float rewardBlack = state.GetResult(BLACK);
    while (n != NULL) {
        n->Update((n->_pjm == WHITE) ? rewardWhite : rewardBlack);
        n = n->_parent;
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Search">

template <class T>
void UCT<T>::UCTSearch(const T& rstate, int sid, int rid, Timer& tmr) {
    /*Create a copy of the current state for each thread*/
    //T state(rstate);
    //T lstate(state);
    T lstate(rstate);

    UCT<T>::Node* n;
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
    int max = ceil(plyOpt.nsims / (float) plyOpt.nthreads);

#ifdef VECRAND
    int umoves = lstate.GetMoves();
    int nRandItr = umoves * 2;
    int RAND_N = max * 2 * umoves;
    if (RAND_N > 16384 * 4)
        RAND_N = 16384 * 4;
    int random[RAND_N] __attribute__((aligned(SIMDALIGN)));
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
        //lstate = state;
        lstate = rstate;

    }
#ifdef VECRAND
    //_mm_free(random);
#endif
}

template <class T>
void UCT<T>::UCTSearchCilkFor(const T& rstate, int sid, int rid, Timer& tmr) {
    /*Create a copy of the current state for each thread*/
    T state(rstate);
    T lstate(state);

    UCT<T>::Node* n;
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
    int max = ceil(plyOpt.nsims / (float) plyOpt.nthreads);

#ifdef VECRAND
    int umoves = lstate.GetMoves();
    int nRandItr = umoves * 2;
    int RAND_N = max * 2 * umoves;
    if (RAND_N > 16384 * 4)
        RAND_N = 16384 * 4;
    int random[RAND_N] __attribute__((aligned(SIMDALIGN)));
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

    UCT<T>::Node* n;
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

    for (itr = 0; itr < max; itr++) {

#ifdef VECRAND
        if (randIndex > limit) {
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
        n = SelectVecRand(n, lstate, plyOpt.cp, random1, randIndex);
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

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Printing">

template <class T>
void UCT<T>::PrintSubTree(UCT<T>::Node* root) {
    cout << root->TreeToString(0) << endl;
}

template <class T>
void UCT<T>::PrintTree() {
    cout << roots[0]->TreeToString(0) << endl;
}

template <class T>
void UCT<T>::PrintRootChildren() {
    for (iterator itr = roots[0]->_children.begin(); itr != roots[0]->_children.end(); itr++) {
        //cout << (*itr)->_move << "," << (*itr)->_visits << "," << (*itr)->_wins / (*itr)->_visits << " | ";
        cout << (*itr)->_move << "," << (*itr)->_visits << ",";
    }
}

template <class T>
void UCT<T>::PrintStats_1(string& log1, double ttime) {
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
    stime /= plyOpt.nthreads;
    etime /= plyOpt.nthreads;
    ptime /= plyOpt.nthreads;
    btime /= plyOpt.nthreads;

    //ttime/=plyOpt.nthreads;
    std::stringstream buffer;
    buffer << std::fixed << std::setprecision(2);
    buffer << setw(10) << roots[0]->_visits << "," <<
            //setw(10) << ttime / plyOpt.nthreads << "," <<
            setw(10) << ttime << "," <<
            setw(10) << (stime / (ttime))*100 << "," <<
            setw(10) << (etime / (ttime))*100 << "," <<
            setw(10) << (ptime / (ttime))*100 << "," <<
            setw(10) << (btime / (ttime))*100 << "," <<
            setw(10) << nrand << ",";
    log1 = buffer.str();
}

template <class T>
void UCT<T>::PrintStats_2(string& log2) {
    std::stringstream buffer;
    std::sort(roots[0]->_children.begin(), roots[0]->_children.end(), SortChildern);
    for (iterator itr = roots[0]->_children.begin(); itr != roots[0]->_children.end(); itr++) {
        buffer << /*(*itr)->_move << "," <<*/ (*itr)->_visits << ",";
    }
    log2 = buffer.str();
}

// </editor-fold>
