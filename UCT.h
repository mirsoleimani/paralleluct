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
#include <algorithm>
#include "CheckError.h"
#include "mkl_vsl.h"
#include "mkl.h"
#include <tbb/task_group.h>
#include <tbb/pipeline.h>

#ifdef __INTEL_COMPILER
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_max.h>
#endif

#ifndef UCT_H
#define	UCT_H

struct PlyOptions {
    int nthreads = 1;
    unsigned int nsims = 1048576;
    float nsecs = 999999;
    float cp = 1.0;
    float minReward=std::numeric_limits<float>::min();
    int par = 0;    
    int threadruntime=0;
    int game=0;
    bool verbose = false;
    unsigned int seed=1;
    int bestreward=4200;
};

struct TimeOptions {
    double ttime = 0.0;
    double stime = 0.0; //select
    double etime = 0.0; //expand
    double ptime = 0.0; //playout
    double btime = 0.0; //backup
    size_t nrand = 0; //number of random numbers are used in simulation
};

#ifdef THREADPOOL
    boost::threadpool::pool thread_pool(NTHREADS);
#endif

template <class T>
class UCT {
public:
     
    // <editor-fold defaultstate="collapsed" desc="node of the tree">
    struct Node {
    public:
        typedef typename std::vector<Node*>::const_iterator constItr;

        Node(int move, UCT<T>::Node* parent, int ply) : _move(move), _parent(parent),
        _pjm(ply), _visits(1), _wins(0) {
            /*http://en.cppreference.com/w/cpp/atomic/atomic_flag_clear*/
#ifdef LOCKFREE
            _isParent=false;
            _isExpandable = false;
            _isFullExpanded = false;
            _untriedMoves = -1;
#else
            _untriedMoves = -1;
#endif
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
#ifdef LOCKFREE

        bool IsFullExpanded() {
            if (_isFullExpanded)
                return true;
            return false;
        }

#else

        bool IsFullExpanded() {
            //std::lock_guard<std::mutex> lock(mtx1);
            if (_untriedMoves == 0)
                return true;
            return false;
        }
#endif

                /**
         * Update the values of a node with reward that
         * comes from the evaluation of a non-terminal 
         * state after the playout operation.
         * @param result
         */
        void Update(int result) {
            //std::lock_guard<std::mutex> lock(mtx1);
            //_visits++;
            //_wins += result;
            /* multiple producer multiple consumer 
             * http://en.cppreference.com/w/cpp/atomic/memory_order
             */
            _visits.fetch_add(1, std::memory_order_seq_cst);
            _wins.fetch_add(result, std::memory_order_seq_cst);
        }
#ifndef LOCKFREE

        void CreatChildren(std::vector<int>& moves, int pjm) {
            std::lock_guard<std::mutex> lock(mtx1);
            if (!IsParent()) {
                for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); itr++) {
                    _children.push_back(new Node(*itr, this, pjm));
                }
                _untriedMoves = moves.size();
            }
        }

        /**
         * Expand a new child from _children. 
         * _untriedMoves-1 will be the index 
         * of the new child.
         * @return A pointer to the new expanded child.
         */
        Node* AddChild() {
            std::lock_guard<std::mutex> lock(mtx1);
            if (!IsFullExpanded()) {
                assert(_untriedMoves > 0 && "AddChild: There is no more child to expand!\n");
                return _children[--_untriedMoves];
            } else {
                return this;
            }
        }

#else

        /**
         * http://en.cppreference.com/w/cpp/atomic/atomic_flag_test_and_set
         * @param moves
         * @param pjm
         */
        void CreatChildren(std::vector<int>& moves, int pjm) {

            /*http://www.cplusplus.com/reference/atomic/atomic/exchange/*/
            if (!_isParent.exchange(true)) {
                for (std::vector<int>::iterator itr = moves.begin(); itr != moves.end(); itr++) {
                    _children.push_back(new Node(*itr, this, pjm));
                }
                _untriedMoves = moves.size();
                /*signal to the waiting threads on _isExpandable*/
                _isExpandable.store(true, std::memory_order_release);
            }
            /*wait for the _isExpandable signal*/
            //while (!_isParent&&!_isExpandable.load(std::memory_order_acquire)) {}
        }

        /**
         * Expand a new child from _children. 
         * _untriedMoves-1 will be the index 
         * of the new child. Following this method
         * https://software.intel.com/en-us/node/506090
         * @return A pointer to the new expanded child.
         */
        Node* AddChild() {
            int index = 0;

            /* wait for the _isExpandable signal
             * http://en.cppreference.com/w/cpp/atomic/memory_order
             */
            //while (!_isParent&&!_isExpandable.load(std::memory_order_acquire)) {}
            //while (!_isExpandable.load(std::memory_order_acquire)) {}
            //TODO is it sufficient to check the following if condition?
            if (_isParent&&_isExpandable.load(std::memory_order_acquire)) {

                if ((index = --_untriedMoves) == 0) {
                    _isFullExpanded = true;
                }

                if (index < 0) {
                    return this;
                } else {
                    assert(index >= 0 && "AddChild: There is no more child to expand!\n");
                    return _children[index];
                }
            } else {
                return this;
            }
        }
#endif

        std::atomic_int _move;
        int _pjm;
        std::atomic_int _wins;
        std::atomic_int _visits;
#ifndef LOCKFREE
        int _untriedMoves;
        std::mutex mtx1;
        std::mutex mtx2;
#else
        std::atomic_int _untriedMoves;
        std::atomic<bool> _isParent;
        std::atomic<bool> _isExpandable;
        std::atomic<bool> _isFullExpanded;
#endif
        Node* _parent;
        std::vector<Node*> _children;
    }; // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="token of the pipeline">
    
    struct Identity{
    public:
        Identity(){
            _id =0;
            _index=0;
        }
        Identity(int id):_id(id),_index(0){}
        Identity& operator=(const Identity& orig){
            if(this == &orig){
                return *this;
            }
            _id = orig._id;
            _index = orig._index;
            
            return *this;
        }
        int _id;
        unsigned int _index;
    };
    
    struct Token {
    public:
        Token(const T state, Identity id) {
            _state=state; //copy the state
            _identity=id;
        }

        ~Token() {
            _path.clear();
        }

        Identity _identity;
        T _state;
        vector<Node*> _path;
    }; // </editor-fold>
    
    typedef Node* NodePtr;
    typedef typename std::vector<NodePtr>::const_iterator const_iterator;
    typedef typename std::vector<NodePtr>::iterator iterator;

    UCT(const PlyOptions opt, int vb, const std::vector<unsigned int> seed);
    UCT(const UCT<T>& orig);
    virtual ~UCT();
    
    void UCTSearch(const T& state, int sid, int rid, Timer tmr);
    void UCTSearchTBBSPSPipe(const T& state, int sid, int rid, Timer tmr);

    /*MCTS functions*/
    NodePtr Select(NodePtr node, T& state);
    NodePtr Expand(NodePtr node, T& state, GEN& engine);
    void Playout(T& state, GEN& engine);
    void Backup(NodePtr node, T& state);

    /*Vector functions*/
    NodePtr SelectVecRand(NodePtr node, T& state, float cp, int *random, int& randIndex);
    NodePtr ExpandVecRand(NodePtr node, T& state, int* random, int& randIndex);
    void PlayoutVecRand(T& state, int* random, int& randIndex);

    /*Pipeline functions*/
    Token* Select(Token* token);
    Token* Expand(Token* token);
    Token* Playout(Token* token);
    Token* Evaluate(Token* token);
    void Backup(Token* token);
    
    /*Print functions*/
    void PrintSubTree(NodePtr root);
    void PrintTree();
    void PrintRootChildren();
    void PrintStats_1(std::string& log1, double total);
    void PrintStats_2(std::string& log2);

    /*Multithreaing section*/
    T Run(const T& state, int& m, std::string &log1, std::string &log2);

    /*َUtility functions*/
    static bool SortChildern(NodePtr a, NodePtr b) {
        return (b->_move > a->_move);
    }

    inline unsigned int NextUniformInt(Identity& tid) {

#define IRNGBUF (*_iRNGBuf)
        unsigned int i;
        if (tid._index == 0) {
            /*The first call to NextUniformInt or _dRNGBuf has been completely used*/
            /* Generate double-precision uniforms from [0;1) */
            viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, _stream[tid._id], MAXRNGBUFSIZE, (int*)_iRNGBuf[tid._id], 0, RAND_MAX);
        }

        /* Return next random integer from buffer */
        i = IRNGBUF[tid._index];
        tid._index = tid._index + 1;

        /* Check if buffer has been completely used */
        if (tid._index == MAXRNGBUFSIZE)
            tid._index = 0;
        return i;
        
    }
    /**
     * Randomly permutes elements in the range [first,last). Functionality is
     * the same as C++'s "random_shuffle", but it uses customized RNG.
     * @param first
     * @param last
     */
    template <class RandomAccessIterator>
    inline void RandomShuffle(RandomAccessIterator first, RandomAccessIterator last, Identity& tid) {
        int i;
        int n;
        n = (last - first);
        for (i = n - 1; i > 0; --i){
            swap(first[i], first[ NextUniformInt(tid) % (i + 1)]);
        }
    }
T _bestState;
private:

    int verbose;
    std::vector<NodePtr> roots;
    std::vector<TimeOptions*> statistics;
    PlyOptions plyOpt;
    std::vector<ENG> gengine;
//    T _bestState;
#ifdef VECRAND
    VSLStreamStatePtr gstream[NSTREAMS];
#endif
#ifdef MKLRNG
    VSLStreamStatePtr _stream[MAXNUMSTREAMS];  /* Each token is associated with an unique stream*/
    unsigned int* _iRNGBuf[MAXNUMSTREAMS]; /* Each token is associated with a unique buffer of uniforms*/
#endif

};


#include "UCT.cpp"
#endif	/* UCT_H */

