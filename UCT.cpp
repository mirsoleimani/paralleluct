#include "UCT.h"

// <editor-fold defaultstate="collapsed" desc="Con/Destruction">

template <class T>
UCT<T>::UCT(const PlyOptions opt, int vb, vector<unsigned int> seed) : verbose(vb) {//TODO remove verbose from class property use plyopt

    plyOpt = opt;
    _score = 0;
#ifdef MKLRNG
    if (plyOpt.nthreads > MAXNUMSTREAMS) {
        std::cerr << "MKLRNG can not support more than " << MAXNUMSTREAMS << " threads!\n";
        exit(0);
    } else {
        for (int i = 0; i < plyOpt.nthreads; i++) {
            /* Each RNG stream will produce RNG sequence inspite of same seed used */
            if ((vslNewStream(&_stream[i], VSL_BRNG_MT2203 + i, plyOpt.seed)) != VSL_STATUS_OK) {
                std::cerr << "MKLRNG: Stream initialization failed for stream " << i << "!\n";
                exit(0);
            }
        }
        int RNGBUFSIZE = 1024; //TODO buffer size should be dynamic based on number of moves
        for (int i = 0; i < plyOpt.nthreads; i++) {
            if (!(_iRNGBuf[i] = (unsigned int*) mkl_malloc(sizeof (unsigned int)*MAXRNGBUFSIZE, SIMDALIGN))) {
                std::cerr << "MKLRNG: Memory allocation failed for buffer " << i << "threads!\n";
                exit(0);
            }
        }
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

#ifdef MKLRNG
    mkl_free_buffers();
    for (int i = 0; i < plyOpt.nthreads; i++) {
        vslDeleteStream(&_stream[i]);
        mkl_free(_iRNGBuf[i]);
    }
#endif
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Run">

template <class T>
T UCT<T>::Run(const T& state, int& m, std::string& log1, std::string& log2, double& ttime) {

    // <editor-fold defaultstate="collapsed" desc="initialize">
    std::vector<std::thread> threads;
    tbb::task_group g;

    T lstate(state);
    _finish = false;
    //double ttime = 0;
    ttime = 0;
    Timer tmr;
    for (int i = 0; i < plyOpt.nthreads; i++) {
        roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        _bestState.emplace_back(state);
        statistics.push_back(new TimeOptions());
    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="fork">
    // <editor-fold defaultstate="collapsed" desc="sequential">
    if (plyOpt.par == SEQUENTIAL) {
        /*create root of the tree*/
        //roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        tmr.reset();
        UCTSearch(lstate, 0, 0, tmr);
        ttime = tmr.elapsed();
    }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="tree parallelization">
    else if (plyOpt.par == TREEPAR) {
        /*create the root*/
        //roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        //        tmr.reset();
        if (plyOpt.threadruntime == CPP11) {
            tmr.reset();
            for (int i = 0; i < plyOpt.nthreads; i++) {
                auto Search = std::bind(&UCT::UCTSearch, std::ref(*this), std::cref(lstate), i, 0, tmr);
                threads.push_back(std::thread(Search));
                //threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
                assert(threads[i].joinable());
            }

        } else if (plyOpt.threadruntime == THPOOL) {
            tmr.reset();
#ifdef THREADPOOL
            for (int i = 0; i < plyOpt.nthreads; i++) {
                thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
            }
#else
            std::cerr << "THREADPOOL is not defined!\n";
            exit(0);
#endif           
        } else if (plyOpt.threadruntime == CILKPSPAWN) {
            tmr.reset();
#ifdef __INTEL_COMPILER
            for (int i = 0; i < plyOpt.nthreads; i++) {
                cilk_spawn UCTSearch(lstate, i, 0, tmr);
            }
#else
            std::cerr << "No Intel compiler for cilk plus!\n";
            exit(0);
#endif
        } else if (plyOpt.threadruntime == TBBTASKGROUP) {
            tmr.reset();
            for (int i = 0; i < plyOpt.nthreads; i++) {
                g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, 0, tmr));
            }
        } else if (plyOpt.threadruntime == CILKPFOR) {
            tmr.reset();
#ifdef __INTEL_COMPILER

            cilk_for(int i = 0; i < plyOpt.nthreads; i++) {
                UCTSearch(lstate, i, 0, tmr);
            }
#else
            std::cerr << "No Intel compiler for cilk plus!\n";
            exit(0);
#endif            
        }
        else {
            std::cerr << "No threading library is selected for tree parallelization!\n";
            exit(0);
        }
    }// </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="root parallelization">
    else if (plyOpt.par == ROOTPAR) {
        /*create the roots among threads*/
        //        for (int i = 0; i < plyOpt.nthreads; i++) {
        //            roots.push_back(new Node(0, NULL, lstate.GetPlyJM()));
        //        }
        //        tmr.reset();
        if (plyOpt.threadruntime == CPP11) {
            tmr.reset();
            for (int i = 0; i < plyOpt.nthreads; i++) {
                auto Search = std::bind(&UCT::UCTSearch, std::ref(*this), std::cref(lstate), i, 0, tmr);
                threads.push_back(std::thread(Search));
                //threads.push_back(std::thread(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
                assert(threads[i].joinable());
            }
        } else if (plyOpt.threadruntime == THPOOL) {
            tmr.reset();
#ifdef THREADPOOL
            for (int i = 0; i < plyOpt.nthreads; i++) {
                thread_pool.schedule(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
            }
#else
            std::cerr << "THREADPOOL is not defined!\n";
            exit(0);
#endif            
        } else if (plyOpt.threadruntime == CILKPSPAWN) {
            tmr.reset();
#ifdef __INTEL_COMPILER
            for (int i = 0; i < plyOpt.nthreads; i++) {
                cilk_spawn UCTSearch(lstate, i, i, tmr);
            }
#else
            std::cerr << "No Intel compiler for cilk plus!\n";
            exit(0);
#endif          
        } else if (plyOpt.threadruntime == TBBTASKGROUP) {
            tmr.reset();
            for (int i = 0; i < plyOpt.nthreads; i++) {
                g.run(std::bind(std::mem_fn(&UCT<T>::UCTSearch), std::ref(*this), std::cref(lstate), i, i, tmr));
            }
        } else if (plyOpt.threadruntime == CILKPFOR) {
            tmr.reset();
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

        // <editor-fold defaultstate="collapsed" desc="pipe parallelization">
    else if (plyOpt.par == PIPEPAR) {
        if (plyOpt.threadruntime == TBBSPSPIPELINE) {
            tmr.reset();
            UCTSearchTBBSPSPipe(lstate, 0, 0, tmr);
            //ttime = tmr.elapsed();

            //                auto Search = std::bind(&UCT::UCTSearchTBBSPSPipe, std::ref(*this), std::cref(lstate), 0, 0, tmr);
            //                threads.push_back(std::thread(Search));
            //                assert(threads[i].joinable());
            //            }
        } else {
            std::cerr << "No threading library is selected for tree parallelization!\n";
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
    }
    //    }else if (plyOpt.threadruntime == TBBSPSPIPELINE) {
    //        if (threads.size() > 0) {
    //            std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    //        }
    //    }// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="collect">
    if (plyOpt.par == TREEPAR || plyOpt.par == PIPEPAR) {
        ttime = tmr.elapsed();
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

    int reward = _bestState[0].GetResult(WHITE);
    T bestState = _bestState[0];
    for (int i = 1; i < plyOpt.nthreads; i++) {
        if (_bestState[i].GetResult(WHITE) < reward) {
            reward = _bestState[i].GetResult(WHITE);
            bestState = _bestState[i];
        }
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
    nn = Select(roots[0], rootState); //TODO the cp value should be zero
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
    statistics.clear();
    _bestState.clear();
    // </editor-fold>

    return bestState;
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="MCTS steps for pipeline">
#ifdef MKLRNG

template <class T>
typename UCT<T>::Token* UCT<T>::Select(Token* token) {

    vector<UCT<T>::Node*> &path = (*token)._path;
    T &state = (*token)._state;
    UCT<T>::Identity& tid = (*token)._identity;
    path.clear();

    /*the first node in the path is root node*/
    UCT<T>::Node* n = roots[tid._rid];
    
    if (plyOpt.game == HORNER) {
        _score = state.GetResult(WHITE);
    } else {
        assert("_score is not initialized!\n");
    }
    
    //add virtual loss to root node
    if (plyOpt.virtualloss) {
        n->Update(_score,1);
    }
    
    path.push_back(n);
    
    while (n->IsFullExpanded()) {
        // <editor-fold defaultstate="collapsed" desc="initializing">
        UCT<T>::Node* next = NULL;
        assert(n->_children.size() > 0 && "_children can not be empty!\n");
        float l = 2.0 * logf((float) n->_visits);
        int index = -1;
        float cp = plyOpt.cp;
        std::vector<float> UCT; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="calculate UCT value for all children">
        //TODO vectorized UCT calculation
        for (iterator itr = n->_children.begin(); itr != n->_children.end(); itr++) {
            int visits = (*itr)->_visits;
            float wins = (*itr)->_wins;
            assert(wins >= 0);
            assert(visits > 0);

            float exploit = 0;
            if (plyOpt.game == HORNER) {
                //_score = state.GetResult(WHITE);
                assert(_score > 0&&"_score is not initialized!\n");
                exploit = _score / (wins / (float) (visits));
            } else if (plyOpt.game == HEX) {
                exploit = wins / (float) (visits);
            }
            float explore = cp * sqrtf(l / (float) (visits));

            UCT.push_back(exploit + explore);
        }
        // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="find a child with max UCT value">
        index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end()));
        assert(index < n->_children.size() && "index is out of range\n");
        next = n->_children[index]; // </editor-fold>

        if (plyOpt.virtualloss) {
            next->Update(_score,1);
        }

        // <editor-fold defaultstate="collapsed" desc="update path and state">
        n = next;
        path.push_back(n);
        state.SetMove(n->_move); /*this line could be removed*/// </editor-fold>
    }
    return token;
}

template <class T>
typename UCT<T>::Token* UCT<T>::Expand(Token* token) {
#ifdef COARSELOCK
    std::lock_guard<std::mutex> lock(_mtxExpand);
#endif
    UCT<T>::Token& t = (*token);
    vector<UCT<T>::Node*> &path = t._path;
    T &state = (*token)._state;
    UCT<T>::Identity& tId = t._identity;

    UCT<T>::Node* n = path.back();
    if (!state.IsTerminal()) {
        // <editor-fold defaultstate="collapsed" desc="create childern for n based on the current state">
        vector<int> moves;
        //set the number of untried moves for n based on the current state
        state.GetMoves(moves);
        RandomShuffle(moves.begin(), moves.end(), tId);
        n->CreatChildren(moves, (state.GetPlyJM() == WHITE) ? WHITE : BLACK);
        // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="add new node child to n">
        n = n->AddChild();
        if (n != path.back()) {
            int m = n->_move;
            assert(m > 0 && "move is not valid!\n");
            state.SetMove(n->_move); /*this line could be removed*/
        }
        // </editor-fold>
    }
    return token;
}

template <class T>
typename UCT<T>::Token* UCT<T>::Playout(Token* token) {
    UCT<T>::Token& t = (*token);
    T& state = t._state;
    UCT<T>::Identity& tId = t._identity;

    // <editor-fold defaultstate="collapsed" desc="find untried moves and randomly suffle them">
    vector<int> moves;
    state.GetPlayoutMoves(moves);
    //std::random_shuffle(moves.begin(), moves.end());
    RandomShuffle(moves.begin(), moves.end(), tId);
    //TODO it is just for test not thread safe// </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="perform a simulation until a terminal state is reached, then evaluate">
    state.SetPlayoutMoves(moves);
    //state.Evaluate();
    // </editor-fold>

    return token;

}

template <class T>
typename UCT<T>::Token* UCT<T>::Evaluate(UCT<T>::Token* token) {

    T &state = (*token)._state;

    state.Evaluate();

    return token;
}

template <class T>
void UCT<T>::Backup(Token* token) {

    vector<UCT<T>::Node*> &path = (*token)._path;
    T &state = (*token)._state;
    int score = (*token)._score;

    UCT<T>::Node* n = path.back();
    float rewardWhite = state.GetResult(WHITE);
    float rewardBlack = state.GetResult(BLACK);

#ifdef VECTORIZEDBACKUP
    if (plyOpt.virtualloss) {
        assert("virtual loss is not implemented!");
    } else {
#pragma simd
        for (int i = 0; i < path.size(); i++)
            path[i]->Update((path[i]->_pjm == WHITE) ? rewardWhite : rewardBlack);
    }
#else
    while (n != NULL) {
        if (plyOpt.virtualloss) {
            n->Update(-_score,-1); //remove virtual loss
            n->Update((n->_pjm == WHITE) ? rewardWhite : rewardBlack);
        } else {
            n->Update((n->_pjm == WHITE) ? rewardWhite : rewardBlack);
        }
        n = n->_parent;
    }
#endif

}

#else

template <class T>
typename UCT<T>::Node* UCT<T>::Select(UCT<T>::Node* n, T& state) {

    while (n->IsFullExpanded()) {
        // <editor-fold defaultstate="collapsed" desc="initializing">
        assert(n->_children.size() > 0 && "_children can not be empty!\n");
        float l = 2.0 * logf((float) n->_visits);
        int index = -1;
        float cp = plyOpt.cp;
        std::vector<float> UCT; // </editor-fold>

        // <editor-fold defaultstate="collapsed" desc="calculate UCT value for all children">
        for (iterator itr = n->_children.begin(); itr != n->_children.end(); itr++) {

            int visits = (*itr)->_visits;
            float wins = (*itr)->_wins;
            assert(wins >= 0);
            assert(visits > 0);

            float exploit = 0;
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
        index = std::distance(UCT.begin(), std::max_element(UCT.begin(), UCT.end())); //TODO: why max is better than min?
        assert(index < n->_children.size() && "index is out of range\n");
        n = n->_children[index]; // </editor-fold>

        //TODO: Add child to current trajectory
        state.SetMove(n->_move); /*this line could be removed*/
    }
    return n;

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
            state.SetMove(n->_move); /*this line could be removed*/
        }
        // </editor-fold>
    }
    return n;
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
void UCT<T>::Backup(UCT<T>::Node* node, T& state) {
    //TODO: Using trajectory makes it possible to do vectorization.
    UCT<T>::Node* n = node;
    float rewardWhite = state.GetResult(WHITE);
    float rewardBlack = state.GetResult(BLACK);
    while (n != NULL) {
        n->Update((n->_pjm == WHITE) ? rewardWhite : rewardBlack);
        n = n->_parent;
    }
}
#endif
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Search">

template <class T>
void UCT<T>::UCTSearch(const T& rstate, int sid, int rid, Timer tmr) {

#ifdef MKLRNG
    UCT<T>::Identity tId(sid, rid);
    UCT<T>::Token* t = new UCT<T>::Token(rstate, tId);
#else    
    /*Create a copy of the current state for each thread*/
    T lstate(rstate);
    //int origMoveCounter = lstate.GetMoveCounter();
    UCT<T>::Node* n = NULL;
    DIST dist(0, 1);
    GEN gen(gengine[sid], dist);
#endif

    float reward = std::numeric_limits<float>::max();
    TimeOptions* timeopt = statistics[sid];
    timeopt->nrand = 0;
    int itr = 0;
    //TODO is it thread safe to calculate max here?
    float nsims = plyOpt.nsims / (float) plyOpt.nthreads;
    int max = ceil(nsims);
#ifdef TIMING
    timeopt->stime = 0;
    timeopt->btime = 0;
    timeopt->ptime = 0;
    timeopt->etime = 0;
    double time = 0.0;
#endif
    
    while ((timeopt->ttime = tmr.elapsed()) < plyOpt.nsecs &&
            itr < max &&
            !_finish) {
#ifndef MKLRNG
        n = roots[rid];
        assert(n != NULL && "Root of the tree is zero!\n");
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef MKLRNG
        t = Select(t);
#else
        n = Select(n, lstate);
#endif        
#ifdef TIMING
        timeopt->stime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef MKLRNG
        t = Expand(t);
#else
        n = Expand(n, lstate, gen);
#endif
#ifdef TIMING
        timeopt->etime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef MKLRNG
        t = Playout(t);
        t = Evaluate(t);
#else
        Playout(lstate, gen);
#endif
#ifdef TIMING
        timeopt->ptime += tmr.elapsed() - time;
#endif

#ifdef TIMING
        time = tmr.elapsed();
#endif
#ifdef MKLRNG
        Backup(t);
#else
        Backup(n, lstate);
#endif
#ifdef TIMING
        timeopt->btime += tmr.elapsed() - time;
#endif

        itr++;
#ifdef MKLRNG
        if (plyOpt.game == HORNER) {
            reward = t->_state.GetResult(WHITE);
            if (t->_state.GetResult(WHITE) < plyOpt.bestreward) {
                _bestState[t->_identity._id] = t->_state;
                _finish = true;
            }
        }
        t->_state = rstate;
        
#else
        reward = lstate.GetResult(WHITE);
        //#ifdef COPYSTATE
        if (lstate.GetResult(WHITE) < plyOpt.bestreward) {
            //_bestState[t->_identity._id] = lstate;
            //finish = true;
        }
        lstate = rstate;
        //#else
        //lstate.UndoMoves(origMoveCounter); //TODO undo moves is wrong 
        //#endif     
#endif

    }
#ifdef MKLRNG
    delete t;
#endif
}

template <class T>
void UCT<T>::UCTSearchTBBSPSPipe(const T& rstate, int sid, int rid, Timer tmr) {

    vector<UCT<T>::Token*> buffer;
    buffer.reserve(plyOpt.nthreads);
    for (int i = 0; i < plyOpt.nthreads; i++) {
        UCT<T>::Identity tId(i);
        buffer.emplace_back(new UCT<T>::Token(rstate, tId));
    }

    //TODO use circular buffer to create and use tokens.
    int itr = plyOpt.nsims;
    int index = 0;
    int ntokens = plyOpt.nthreads;
    bool finish = false;
    tbb::parallel_pipeline(
            ntokens,
            tbb::make_filter<void, UCT<T>::Token*>(
            tbb::filter::serial_in_order, [&](tbb::flow_control & fc)->UCT<T>::Token* {
                //UCT<T>::Token *t = new UCT<T>::Token(rstate, roots[0]);
                UCT<T>::Token* t = buffer[index];
                index = (index + 1) % ntokens;
                if (--itr == 0 || finish) {
                    fc.stop();
                    return NULL;

                } else {
                    t = Select(t);
                    return t;
                }

            })
    &
    tbb::make_filter<UCT<T>::Token*, UCT<T>::Token*>(
            tbb::filter::parallel, [&](UCT<T>::Token * t) {
                return Expand(t);
            })
    &
    tbb::make_filter<UCT<T>::Token*, UCT<T>::Token*>(
            tbb::filter::parallel, [&](UCT<T>::Token * t) {
                return Playout(t);
            })
    &
    tbb::make_filter<UCT<T>::Token*, UCT<T>::Token*>(
            tbb::filter::parallel, [&](UCT<T>::Token * t) {
                return Evaluate(t);
            })
    &
    tbb::make_filter<UCT<T>::Token*, void>(
            tbb::filter::serial_in_order, [&](UCT<T>::Token * t) {
                Backup(t);
                if (plyOpt.game == HORNER) {
                    if (t->_state.GetResult(WHITE) < plyOpt.bestreward) {
                        _bestState[t->_identity._id] = t->_state;
                                finish = true;
                    }
                }
                t->_state = rstate;
            }));

    for (int i = 0; i < ntokens; i++)
        delete buffer[i];

}// </editor-fold>

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
