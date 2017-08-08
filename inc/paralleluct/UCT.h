/* 
 * File:   UCT.h
 * Author: mirsoleimanisa
 *
 * Created on 17 september 2014, 16:32
 */

#include "paralleluct/state/PGameState.h"
#include "paralleluct/state/HexState.h"
#include "Utilities.h"
#include <thread>
#include <mutex>
#include <memory>
//#include <omp.h>
#include <chrono>
#include <algorithm>
#include "mkl_vsl.h"
#include "mkl.h"
#include <tbb/task_group.h>
#include <tbb/pipeline.h>
#include <fstream>

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
    int par = 0;    
    int threadruntime=0;
    int game=0;
    bool verbose = false;
    unsigned int seed=1;
    int bestreward=4200;
    int nmoves = 0;
    bool virtualloss=0;
    char* locking=const_cast<char *>("LOCKFREE");
    int twoply=1;
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

        Node(int move, UCT<T>::Node* parent, int ply) : _move(move),
        _pjm(ply), _wins(0), _visits(1),  _parent(parent) {
            /*http://en.cppreference.com/w/cpp/atomic/atomic_flag_clear*/
#ifdef LOCKFREE
            SetWinsVisits(0,0);
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

#ifdef LOCKFREE
         /**
         * Update the values of a node with reward that
         * comes from the evaluation of a non-terminal 
         * state after the playout operation.
         * @param result
         */
        void Update(int result) {
            /* multiple producer multiple consumer 
             * http://en.cppreference.com/w/cpp/atomic/memory_order
             */
            int w=result;
            int n=1;
            int64_t wnp;
            wnp = (((int64_t) w) << 32) | ((int64_t) n);
            _wins_visits.fetch_add(wnp, std::memory_order_relaxed);
        }
        void Update(int result,int c) {
            /* multiple producer multiple consumer 
             * http://en.cppreference.com/w/cpp/atomic/memory_order
             */
            int w=result;
            int n=c;
            int64_t wnp;
            wnp = (((int64_t) w) << 32) | ((int64_t) n);
            _wins_visits.fetch_add(wnp, std::memory_order_relaxed);
        }
#else

        /**
         * Update the values of a node with reward that
         * comes from the evaluation of a non-terminal 
         * state after the playout operation.
         * @param result
         */
        void Update(int result) {
            std::lock_guard<std::mutex> lock(mtx1);
            _visits++;
            _wins += result;
        }

        void Update(int result, int c) {
            std::lock_guard<std::mutex> lock(mtx1);
            _visits+=c;
            _wins += result;
        }
#endif

#ifdef FINELOCK

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

#elif defined(COARSELOCK)

        void CreatChildren(std::vector<int>& moves, int pjm) {
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

            //http://www.cplusplus.com/reference/atomic/atomic/exchange/
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

#ifdef LOCKFREE

        int GetWins() const {
            int w;
            int64_t wnp;
            wnp = _wins_visits.load(std::memory_order_relaxed);
            w = wnp >> 32; //high 32 bits;
            return w;
        }

        int GetVisits() const{
            int n;
            int64_t wnp;
            wnp = _wins_visits.load(std::memory_order_relaxed);
            n = wnp & 0xFFFFFFFF; //low 32 bits;
            return n;
        }
        
        void SetWinsVisits(int w, int n) {
            int64_t wnp;
            wnp = (((int64_t) w) << 32) | ((int64_t) n);
            _wins_visits.store(wnp, std::memory_order_relaxed);
        }
        
#else
        int GetWins(){
            return _wins;
        }
        
        int GetVisits(){
            return _visits;
        }
        
        void SetWinsVisit(int w, int n){
            std::lock_guard<std::mutex> lock(mtx1);
            _wins = w;
            _visits = n;
        }

#endif

        /**
         * save the node in dot language. attache himself to @pId as @gId.
         * print himself as a new parent for the children.
         * @param fout
         * @param gId
         * @param pId
         */
        void SaveDot(std::ofstream& fout, int& gId, const int pId) {
            int id = gId;

            fout << NumToStr(pId) << "->" << NumToStr(id)
                    << "[ label = \"" << NumToStr(GetVisits())
                    << "\" ];\n";
            if (_isParent) {
                fout << NumToStr(id)
                        << "[ label = \""
                        << NumToStr(_move.load())
                        << "\" ];\n";
                for (auto c : _children) {
                    if ((*c).GetVisits() > 0) {
                        gId++;
                        c->SaveDot(fout, gId, id);
                    }
                }
            } else {
                fout << NumToStr(id) 
                        << "[ label = \"" 
                        << NumToStr(_move.load())
                        << "\" , shape=box ];\n";
                return;
            }
            return;
        }
        
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
        std::atomic<int_fast64_t> _wins_visits;
#endif
        Node* _parent;
        std::vector<Node*> _children;
    }; // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="token of the pipeline">
    
    struct Identity{
    public:
        Identity():_id(0),_rid(0),_index(0){}
        Identity(int id):_id(id),_rid(0),_index(0){}
        Identity(int id,int rid):_id(id),_rid(rid),_index(0){}
        Identity& operator=(const Identity& orig){
            if(this == &orig){
                return *this;
            }
            _id = orig._id;
            _rid = orig._rid;
            _index = orig._index;
            
            return *this;
        }
        int _id;
        int _rid;
        unsigned int _index;
    };
    
    struct Token {
    public:
        Token(const T state, Identity id) {
            _state=state; //copy the state
            _identity=id;
            _score = 0;
        }

        ~Token() {
            _path.clear();
        }

        Identity _identity;
        T _state;
        vector<Node*> _path;
        int _score;
    }; // </editor-fold>
    
    typedef Node* NodePtr;
    typedef typename std::vector<NodePtr>::const_iterator const_iterator;
    typedef typename std::vector<NodePtr>::iterator iterator;

    UCT(const PlyOptions opt, int vb, const std::vector<unsigned int> seed);
    UCT(const UCT<T>& orig);
    virtual ~UCT();

    /*Multithreaing section*/
    T Run(const T& state, int& m, std::string &log1, std::string &log2, double& ttime);

    void UCTSearch(const T& state, int sid, int rid, Timer tmr);
    void UCTSearchTBBSPSPipe(const T& state, int sid, int rid, Timer tmr);

    /*MCTS functions*/
#ifdef MKLRNG
    Token* Select(Token* token);
    Token* Expand(Token* token);
    Token* Playout(Token* token);
    Token* Evaluate(Token* token);
    void Backup(Token* token);
#else
    NodePtr Select(NodePtr node, T& state);
    NodePtr Expand(NodePtr node, T& state, GEN& engine);
    void Playout(T& state, GEN& engine);
    void Backup(NodePtr node, T& state);
#endif
    
    /*Print functions*/
    void Print(std::string fileName);
    void PrintStats_1(std::string& log1, double total);
    void PrintStats_2(std::string& log2);
    void SaveDot(std::string fileName);   
    int NumPlayoutsRoot() {
        return roots[0]->GetVisits();
    }

    /*َUtility functions*/
    static bool SortChildern(NodePtr a, NodePtr b) {
        return (b->_move > a->_move);
    }
#ifdef MKLRNG
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
#endif 
    
private:

    int verbose;
    PlyOptions plyOpt;
    std::vector<NodePtr> roots;
    std::vector<TimeOptions*> statistics;
    std::vector<T> _localBestState;
    T _globalBestState;
    int _nPlayouts;
    float _score;
#ifdef MKLRNG
    VSLStreamStatePtr _stream[MAXNUMSTREAMS]; /* Each token is associated with an unique stream*/
    unsigned int* _iRNGBuf[MAXNUMSTREAMS]; /* Each token is associated with a unique buffer of uniforms*/
#else
    std::vector<ENG> gengine;
#endif
    std::atomic<bool> _finish; 
#ifdef COARSELOCK
    std::mutex _mtxExpand;
#endif
};


#include "UCT.cpp"
#endif	/* UCT_H */
