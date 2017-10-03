/* 
 * File:   Utilities.h
 * Author: mirsoleimanisa
 *
 * Created on 22 september 2014, 12:31
 */
#include <iomanip>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <time.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <atomic>
#include <sys/time.h>
#include <stddef.h>

#include <boost/assert.hpp>

using namespace std;

#ifndef UTILITIES_H
#define	UTILITIES_H

#define NDEBUG
#include <assert.h>

//#define THREADPOOL
#ifdef THREADPOOL
#include <boost/thread/thread.hpp>
#include "threadpool.hpp"
#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION < 105000
#define TIME_UTC_ TIME_UTC
#endif
#endif

#define MKLRNG
#ifndef MKLRNG
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
typedef boost::mt19937 ENG; // Mersenne Twister
typedef boost::uniform_int<int> DIST;
typedef boost::variate_generator<ENG, DIST > GEN;
#endif

//#define CILKSELECT
//#define TIMING
#define MAXDEPTH //measure the maximum depth of the tree that is reached.
#define COUNTNRANDVEC   //counting the number of generated random buffers.
#define MAXNUMVISITS

#define LOCKFREE
//#define FINELOCK
//#define COARSELOCK

//#define COPYSTATE
#define VECTORIZEDBACKUP


#define POS(i,j,dim) i*dim+j       
#define MAX(n,m) ((n)>(m)?(n):(m))

#define NSTREAMS 6024
static const int MAXNUMSTREAMS = 6024;
static const int MAXRNGBUFSIZE = 1024;
static const int MAXRAND_N = 10000;
static const int SIMDALIGN = 64;
static const int NTHREADS = 244;
static const int MAXTHREAD = 273;

static const int TOPDOWN = 11;
static const int LEFTRIGHT = 22;
static const int BLACK = 1;
static const int WHITE = 2;
static const int CLEAR = 3;
static const float WIN = 1.0;
static const float LOST = 0.0;
static const float DRAW = 0.5;

typedef int MOVE;
typedef std::vector<int> term;
typedef std::vector<term> polynomial;
static vector<vector<int>> _poly;
#define HEXSTATICBOARD
#ifdef HEXSTATICBOARD
static vector<vector<int>> edges;
static vector<int> lefPos;
#endif

enum THREADLIB{NONE=0,CPP11,THPOOL,CILKPSPAWN,TBBTASKGROUP,CILKPFOR,TBBSPSPIPELINE,LASTTHREADLIB};
//static const vector<string> THREADLIBNAME = {"none","c++11","threadpool","cilk_spawn","tbb_task_group","cilk_for","tbb_sps_pipeline"};
enum PARMETHOD{SEQUENTIAL=0,TREEPAR,ROOTPAR,PIPEPARFIVE,PIPEPARSIX,PIPEPARSEVEN,PIPEPAREIGHT,LASTPAR};
//static const vector<string> PARMETHODNAME = {"sequential","tree","root","pipeline"};
enum GAME{NOGAME=0,HEX,PGAME,HORNER,GEMPUZZLE,LASTGAME};
//static const vector<string> GAMENAME = {"none","hex","pgame","horner","gem-puzzle"};
enum LOCKMETHOD{FREELOCK,FINEGRAINLOCK,COARSEGRAINLOCK,LASTLOCKMETHOD};
//static const vector<string> LOCKMETHODNAME = {"lock-free","fine-lock","coarse-lock"};
enum BACKUPDIRCT{BOTTOMUPBACKUP=0,TOPDOWNBACKUP,LASTBACKUPDIRECT};
//typedef std::mt19937 ENG; // Mersenne Twister
//typedef std::uniform_int<int> DIST; // Uniform Distribution
//typedef std::variate_generator<ENG&, DIST> GEN; // Variate generator

class Timer {
public:

    Timer() {
        clock_gettime(CLOCK_REALTIME, &beg_);
    }

    double elapsed() {
        timespec end_;
        clock_gettime(CLOCK_REALTIME, &end_);
        return end_.tv_sec - beg_.tv_sec +
                (end_.tv_nsec - beg_.tv_nsec) / 1000000000.;
    }

    void reset() {
        clock_gettime(CLOCK_REALTIME, &beg_);
    }

    double second() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double) tv.tv_sec + (double) tv.tv_usec / 1000000.0;
    }

private:
    timespec beg_;//, end_;
};

template <typename T>
string NumToStr(T num) {
    ostringstream ss;
    ss << std::setprecision(4) << num;
    return ss.str();
}

template <class T>
inline void PRINT_ELEMENTS(const T& coll, const char* optcstr = "") {
    typename T::const_iterator pos;

    std::cout << optcstr;
    for (pos = coll.begin(); pos != coll.end(); ++pos) {
        std::cout << *pos << ' ';
    }
    std::cout << std::endl;
}


/**
 * Generates all ordered subsets of size k. Should yield (n ncr k) elements.
 * @param s Array to generate subsets from
 * @param k Number of elements to pick
 * @param t Temporary buffer. Should have size k.
 * @param out List of subsets
 * @param q Internal use
 * @param r Internal use
 */
template <typename T>
void genSubsets(const std::vector<T>& s, unsigned int k, std::vector<T>& t, std::vector<std::vector<T >> &out, unsigned int q = 0, int r = 0) {
    if (k - q > s.size() - r) {
        return;
    }

    if (q == k) {
        out.push_back(t);
    } else {
        for (unsigned int i = r; i < s.size(); i++) {
            t[q] = s[i];
            genSubsets(s, k, t, out, q + 1, i + 1);
        }
    }
}

template <typename T>
float getAverage(std::vector<T>& sample) {
    T sum = 0;
    for (T& s : sample) {
        sum += s;
    }

    return sum / (float) sample.size();
}

template <typename T>
float getStdDev(std::vector<T>& sample) {
    float avg = getAverage(sample);

    float sum = 0;
    for (T& s : sample) {
        sum += (s - avg) * (s - avg);
    }

    return sqrt(sum / (sample.size() - 1.0));
}


#endif	/* UTIITIES_H */

