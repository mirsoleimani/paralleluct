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
void UCT<T>::Run(const T& state, int& m, std::string& log1, std::string& log2) {

    // <editor-fold defaultstate="collapsed" desc="initialize">
    std::vector<std::thread> threads;
    tbb::task_group g;

    T lstate(state);
    double ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        statistics.push_back(new TimeOptions());
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="fork">
    // <editor-fold defaultstate="collapsed" desc="sequential">
    if (plyOpt.par == SEQUENTIAL) {
        /*create root of the tree*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        UCTSearch(lstate, 0, 0, tmr);
        ttime=tmr.elapsed();
    }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="tree parallelization">
    else if (plyOpt.par == TREEPAR) {
        /*create the root*/
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        if (plyOpt.threadruntime == CPP11) {
            
            for (int i = 0; i < plyOpt.nthreads; i++) {
                auto Search = std::bind(&UCT::UCTSearch,std::ref(*this),std::cref(lstate),i,0,tmr);
                threads.push_back(std::thread(Search));
                //threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
                assert(threads[i].joinable());
            }
            
        } else if (plyOpt.threadruntime == THPOOL) {  
#ifdef THREADPOOL
            for (int i = 0; i < plyOpt.nthreads; i++) {
                thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
            }
#else
            std::cerr << "THREADPOOL is not defined!\n";
            exit(0);
#endif           
        } else if (plyOpt.threadruntime == CILKPSPAWN) {
#ifdef __INTEL_COMPILER
            for (int i = 0; i < plyOpt.nthreads; i++) {
                cilk_spawn UCTSearch(lstate, i, 0, tmr);
            }
#else
            std::cerr<<"No Intel compiler for cilk plus!\n";
            exit(0);
#endif
        } else if (plyOpt.threadruntime == TBBTASKGROUP) {  
            for (int i = 0; i < plyOpt.nthreads; i++) {
                g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
            }            
        } else if (plyOpt.threadruntime == CILKPFOR) {
#ifdef __INTEL_COMPILER
            cilk_for(int i = 0; i < plyOpt.nthreads; i++) {
                UCTSearchTBBSPSPipe(lstate, i, 0, tmr);
            }
#else
            std::cerr<<"No Intel compiler for cilk plus!\n";
            exit(0);
#endif            
        } else if (plyOpt.threadruntime == TBBSPSPIPELINE) {
            for (int i = 0; i < plyOpt.nthreads; i++) {
                auto Search = std::bind(&UCT::UCTSearchTBBSPSPipe, std::ref(*this), std::cref(lstate), i, 0, tmr);
                threads.push_back(std::thread(Search));
                assert(threads[i].joinable());
            }
        } else {
            std::cerr << "No threading library is selected for tree parallelization!\n";
            exit(0);
        }
    }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="root parallelization">
    else if (plyOpt.par == ROOTPAR) {
        /*create the roots among threads*/
        for (int i = 0; i < plyOpt.nthreads; i++) {
            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        }
        tmr.reset();
        if (plyOpt.threadruntime == CPP11) {
            for (int i = 0; i < plyOpt.nthreads; i++) {
                auto Search = std::bind(&UCT::UCTSearch,std::ref(*this),std::cref(lstate),i,0,tmr);
                threads.push_back(std::thread(Search));
                //threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
                assert(threads[i].joinable());
            }
        } else if (plyOpt.threadruntime == THPOOL) {            
#ifdef THREADPOOL
            for (int i = 0; i < plyOpt.nthreads; i++) {
                thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
            }
#else
            std::cerr<<"THREADPOOL is not defined!\n";
            exit(0);
#endif            
        } else if (plyOpt.threadruntime == CILKPSPAWN) {
#ifdef __INTEL_COMPILER
            for (int i = 0; i < plyOpt.nthreads; i++) {
                cilk_spawn UCTSearch(lstate, i, i, tmr);
            }
#else
            std::cerr<<"No Intel compiler for cilk plus!\n";
            exit(0);
#endif          
        } else if (plyOpt.threadruntime == TBBTASKGROUP) {
            for (int i = 0; i < plyOpt.nthreads; i++) {
                g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
            }           
        } else if (plyOpt.threadruntime == CILKPFOR) {
            std::cerr < "Cilk plus for for root parallelization is not implemented!\n";
            exit(0);

            /*cilk_for(int i = 0; i < plyOpt.nthreads; i++) {
                      UCTSearch(lstate, i, i, tmr);
              }*/
        } else {
            std::cerr << "No threading library is selected for root parallelization!\n";
            exit(0);
        }
    }// </editor-fold>

    else {
        std::cerr << "No parallel method (-y) is selected!\n";
        exit(0);
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="join">
    if (plyOpt.threadruntime == CPP11) {
        if (threads.size() > 0) {
            std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
        }
    } else if (plyOpt.threadruntime == THPOOL) {
#ifdef THREADPOOL
        thread_pool.wait(0);
#else
        std::cerr << "THREADPOOL is not defined!\n";
        exit(0);
#endif
    } else if (plyOpt.threadruntime == CILKPSPAWN) {
#ifdef __INTEL_COMPILER
        cilk_sync;
#else
        std::cerr << "No Intel compiler for cilk plus!\n";
        exit(0);
#endif
    } else if (plyOpt.threadruntime == TBBTASKGROUP) {
        g.wait();
    } else if (plyOpt.threadruntime == TBBSPSPIPELINE) {
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="collect">
    if(plyOpt.par==TREEPAR){
        ttime=tmr.elapsed();
    }
    if (plyOpt.par == ROOTPAR) {
        for (int j = 1; j < roots.size(); j++) {
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
        ttime = tmr.elapsed();
    }
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="select best child">
    /*Find the best node*/
    T rootState(state);
    vector<int>UCT;
    UCT::Node* nn = roots[0];
#ifdef MAXNUMVISITS
    // <editor-fold defaultstate="collapsed" desc="extract number of visits for each children">
    for (iterator itr = nn->_children.begin(); itr != nn->_children.end(); itr++) {
        int visits = (*itr)->_visits;
        UCT.push_back(visits);
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="find a child with max UCT value">
    int index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end()));
    nn = nn->_children[index]; // </editor-fold>
#else
    nn = UCT<T>::Select(roots[0], rootState, 0);
#endif
    m = nn->_move; // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="log">
    if (verbose) {
        PrintStats_1(log1, ttime);
    }
    if (verbose == 3) {
        PrintStats_2(log2);
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="free">
    for (iterator itr = roots.begin(); itr != roots.end(); itr++)
        delete (*itr);
    roots.clear();
    for (vector<TimeOptions*>::const_iterator itr = statistics.begin(); itr != statistics.end(); itr++)
        delete (*itr);
    statistics.clear(); // </editor-fold>
    
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="MCTS steps">

template <class T>
typename UCT<T>::Node* UCT<T>::Select(UCT<T>::Node* n, T& state) {

    while (n->IsFullExpanded()) {
        // <editor-fold defaultstate="collapsed" desc="initializing">
        assert(n->_children.size()>0&&"_children can not be empty!\n");
        float l = 2.0 * logf((float) n->_visits);
        int index = -1;
        float cp=plyOpt.cp;
        std::vector<float> UCT; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="calculate UCT value for all children">
        for (iterator itr = n->_children.begin(); itr != n->_children.end(); itr++) {

            int visits = (*itr)->_visits;
            float wins = (*itr)->_wins;
            assert(wins >= 0);
            assert(visits > 0);

            float exploit=0;
            if (plyOpt.game == HORNER) {
                float score = state.GetResult(WHITE);
                assert(score > 0);
                exploit = score / (wins / (float) (visits));
            } else if (plyOpt.game == HEX) {
                exploit = wins / (float) (visits);
            }
            float explore = cp * sqrtf(l / (float) (visits));

            UCT.push_back(exploit + explore);

        }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="find a child with max UCT value">
        index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end()));   //TODO: why max is better than min?
        assert(index<n->_children.size() && "index is out of range\n");
        n = n->_children[index]; // </editor-fold>

        //TODO: Add child to current trajectory
        state.SetMove(n->_move);    /*this line could be removed*/
    }
    return n;

}

template <class T>
typename UCT<T>::Node* UCT<T>::SelectVecRand(UCT<T>::Node* node, T& state, float cp, int* random, int &randIndex) {

//    UCT<T>::Node* n = node;
//    //while (n->GetUntriedMoves() == 0 && !n->_children.empty()) {
//    while (n->GetUntriedMoves() == 0) {
//        n = n->UCTSelectChild(cp, random[randIndex++]);
//        state.SetMove(n->_move);
//    }
//    return n;
}

template <class T>
typename UCT<T>::Node* UCT<T>::Expand(UCT<T>::Node* node, T& state, GEN& engine) {

    UCT<T>::Node* n = node;
    if (!state.IsTerminal()) {
        // <editor-fold defaultstate="collapsed" desc="create childern for n based on the current state">
        vector<int> moves;
        //set the number of untried moves for n based on the current state
        state.GetMoves(moves);
        std::random_shuffle(moves.begin(), moves.end(), engine);
        n->CreatChildren(moves, (state.GetPlyJM() == WHITE) ? WHITE : BLACK);
        // </editor-fold>

        //TODO: Appending child could be happened in update phase.
        //TODO: Append new node the trajectory
        // <editor-fold defaultstate="collapsed" desc="add new node child to n">
        n = n->AddChild();
        if (n != node) {
            int m = n->_move;
            assert(m > 0 && "move is not valid!\n");
            state.SetMove(n->_move);    /*this line could be removed*/
        }
        // </editor-fold>
    }
    return n;
}

template <class T>
typename UCT<T>::Node* UCT<T>::ExpandVecRand(UCT<T>::Node* node, T& state, int* random, int &randIndex) {

//    UCT<T>::Node* n = NULL;
//    n = node->AddChild2(state, random, randIndex);
//    assert(n != NULL && "The node to be expanded is NULL");
//
//    return n;
}

template <class T>
void UCT<T>::Playout(T& state, GEN& engine) {
    vector<int> moves;
    state.GetPlayoutMoves(moves);
    std::random_shuffle(moves.begin(), moves.end(), engine);

    // <editor-fold defaultstate="collapsed" desc="perform a simulation until a terminal state is reached">
    state.SetPlayoutMoves(moves);
    // </editor-fold>

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

// <editor-fold defaultstate="collapsed" desc="MCTS steps for pipeline">

template <class T>
typename UCT<T>::Token* UCT<T>::Select(Token* token) {
    
    vector<UCT<T>::Node*> &path=(*token)._path;
    T &state = (*token)._state;
    
    path.clear();
    
    /*the first node in the path is root node*/
    UCT<T>::Node* n = roots[0];
    path.push_back(n);
    while (n->IsFullExpanded()) {
#ifdef VIRTUALLOSS
        //TODO implement virtual loss
#endif
        
        // <editor-fold defaultstate="collapsed" desc="initializing">
        UCT<T>::Node* next=NULL;
        assert(n->_children.size()>0&&"_children can not be empty!\n");
        float l = 2.0 * logf((float) n->_visits);
        int index = -1;
        float cp=plyOpt.cp;
        std::vector<float> UCT; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="calculate UCT value for all children">
        for (iterator itr = n->_children.begin(); itr != n->_children.end(); itr++) {

            int visits = (*itr)->_visits;
            float wins = (*itr)->_wins;
            assert(wins >= 0);
            assert(visits > 0);

            float exploit=0;
            if (plyOpt.game == HORNER) {
                float score = state.GetResult(WHITE);
                assert(score > 0);
                exploit = score / (wins / (float) (visits));
            } else if (plyOpt.game == HEX) {
                exploit = wins / (float) (visits);
            }
            float explore = cp * sqrtf(l / (float) (visits));

            UCT.push_back(exploit + explore);

        }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="find a child with max UCT value">
        index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end()));
        assert(index<n->_children.size() && "index is out of range\n");
        next = n->_children[index]; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="update path and state">
        n = next;
        path.push_back(n);
        state.SetMove(n->_move); /*this line could be removed*/// </editor-fold>
    }
    return token;
}

template <class T>
typename UCT<T>::Token* UCT<T>::Expand(Token* token) {

    vector<UCT<T>::Node*> &path = (*token)._path;
    T &state = (*token)._state;
    
    UCT<T>::Node* n = path.back();
    if (!state.IsTerminal()) {
        // <editor-fold defaultstate="collapsed" desc="create childern for n based on the current state">
        vector<int> moves;
        //set the number of untried moves for n based on the current state
        state.GetMoves(moves);
        std::random_shuffle(moves.begin(), moves.end()); //TODO it is just for test not thread safe
        n->CreatChildren(moves, (state.GetPlyJM() == WHITE) ? WHITE : BLACK);
        // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="add new node child to n">
        n = n->AddChild();
        if (n != path.back()) {
            int m = n->_move;
            assert(m > 0 && "move is not valid!\n");
            state.SetMove(n->_move);    /*this line could be removed*/
        }
        // </editor-fold>
    }
    return token;
}

template <class T>
typename UCT<T>::Token* UCT<T>::Playout(Token* token) {

    vector<UCT<T>::Node*> &path = (*token)._path;
    T &state = (*token)._state;

    // <editor-fold defaultstate="collapsed" desc="find untried moves and randomly suffle them">
    vector<int> moves;
    state.GetPlayoutMoves(moves);
    std::random_shuffle(moves.begin(), moves.end());
    //TODO it is just for test not thread safe// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="perform a simulation until a terminal state is reached, then evaluate">
    state.SetPlayoutMoves(moves);
    state.Evaluate();
    // </editor-fold>
    
    return token;
    
}

template <class T>
void UCT<T>::Backup(Token* token) {

    vector<UCT<T>::Node*> &path = (*token)._path;
    T &state = (*token)._state;

    UCT<T>::Node* n = path.back();
    float rewardWhite = state.GetResult(WHITE);
    float rewardBlack = state.GetResult(BLACK);
    while (n != NULL) {
        n->Update((n->_pjm == WHITE) ? rewardWhite : rewardBlack);
        n = n->_parent;
    }
    
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Search">

template <class T>
void UCT<T>::UCTSearch(const T& rstate, int sid, int rid, Timer tmr) {
    
    /*Create a copy of the current state for each thread*/
    T lstate(rstate);
    int origMoveCounter = lstate.GetMoveCounter();
    float reward=std::numeric_limits<float>::infinity();

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
    //TODO is it thread safe to calculate max here?
    float nsims = plyOpt.nsims / (float) plyOpt.nthreads;
    int max = ceil(nsims);
    
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

    while ((timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs && itr < max && reward > plyOpt.minReward) {

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
        n = Select(n, lstate);
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
        reward = lstate.GetResult(WHITE);
#ifdef COPYSTATE
        lstate = rstate;
#else
        lstate.UndoMoves(origMoveCounter);
#endif

    }
#ifdef VECRAND
    //_mm_free(random);
#endif
}// </editor-fold>

template <class T>
void UCT<T>::UCTSearchTBBSPSPipe(const T& rstate, int sid, int rid, Timer tmr) {

    /*Create a copy of the current state for each thread*/
    UCT<T>::Token* t = new UCT<T>::Token(rstate, roots[rid]);
    vector<UCT<T>::Node*> &path = (*t)._path;
    T &state = (*t)._state;
    int origMoveCounter = state.GetMoveCounter();
    float reward = std::numeric_limits<float>::infinity();

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
    //TODO is it thread safe to calculate max here?
    float nsims = plyOpt.nsims / (float) plyOpt.nthreads;
    int max = ceil(nsims);

    while ((timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs 
            && itr < max 
            && reward > plyOpt.minReward) {
//        n = roots[rid];
//        assert(n != NULL && "Root of the tree is zero!\n");

#ifdef TIMING
        time = tmr.elapsed();
#endif
        t = Select(t);    
#ifdef TIMING
        timeopt->stime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
        t = Expand(t);
#ifdef TIMING
        timeopt->etime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
        t=Playout(t);
#ifdef TIMING
        timeopt->ptime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
        Backup(t);
#ifdef TIMING
        timeopt->btime += tmr.elapsed() - time;
#endif

        itr++;
        //reward = lstate.GetResult(WHITE);
        
#ifdef COPYSTATE
        //lstate = rstate;
#else
        t->_state.UndoMoves(origMoveCounter);
#endif

    }
    
}


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
