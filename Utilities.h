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


#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

//#define THREADPOOL
#ifdef THREADPOOL
#include <boost/thread/thread.hpp>
#include "threadpool.hpp"
#include <boost/assert.hpp>
#include <boost/asio.hpp>
#endif

using namespace std;

#ifndef UTILITIES_H
#define	UTILITIES_H

//#define MKLRAND
//#define VECRAND

//#define CILKSELECT
#define MAXNUMVISITS

//#define TIMING

#define NDEBUG
#include <assert.h>

#define LOCKFREE

//#define COPYSTATE

#define POS(i,j,dim) i*dim+j       
#define MAX(n,m) ((n)>(m)?(n):(m))

#define NSTREAMS 6024
static const int MAXRAND_N=10000;
static const int SIMDALIGN= 64;  
static const int NTHREADS= 244;
//static const int MAXTHREAD = 273;
static const int TOPDOWN = 11;
static const int LEFTRIGHT = 22;
static const int BLACK = 1;
static const int WHITE = 2;
static const int CLEAR = 3;
static const float WIN=1.0;
static const float LOST=0.0;
static const float DRAW=0.5;

typedef int MOVE;
typedef std::vector<int> term;
typedef std::vector<term> polynomial;

enum threadlib{NONE,CPP11,THPOOL,CILKPSPAWN,TBBTASKGROUP,CILKPFOR,TBBSPSPIPELINE};
enum parallelization{SEQUENTIAL=0,TREEPAR,ROOTPAR};
enum GAME{NOGAME=0,HEX,PGAME,HORNER,GEMPUZZLE};

//#define RandInt(n) ((int)((float)(n)*rand()/(RAND_MAX+1.0)))
//#define RandChar(n) ((char)((float)(n)*rand()/(RAND_MAX+1.0)))
//#define RandFloat(n) ((float)(v)*rand()/(RAND_MAX+1.0))

//typedef std::mt19937 ENG; // Mersenne Twister
//typedef std::uniform_int<int> DIST; // Uniform Distribution
//typedef std::variate_generator<ENG&, DIST> GEN; // Variate generator

typedef boost::mt19937 ENG; // Mersenne Twister
typedef boost::uniform_int<int> DIST;
typedef boost::variate_generator<ENG, DIST > GEN;

//const FP_TYPE NUM_ONE = 1.0;
//static inline FP_TYPE RandDouble(FP_TYPE low, FP_TYPE high){
//    double t = (FP_TYPE)rand() / (FP_TYPE)RAND_MAX;
//    return (NUM_ONE - t) * low + t * high;
//}

//static inline float RandFloat_T(float low, float high, unsigned int *seed){
//    float t = (float)rand_r(seed) / (float)RAND_MAX;
//    return (1.0 - t) * low + t * high;
//}

template<typename _RandomAccessIterator>
inline void
random_shuffle_custome(_RandomAccessIterator __first, _RandomAccessIterator __last, int rand) {
    // concept requirements
    __glibcxx_function_requires(_Mutable_RandomAccessIteratorConcept<
            _RandomAccessIterator>)
            __glibcxx_requires_valid_range(__first, __last);

    if (__first != __last)
        for (_RandomAccessIterator __i = __first + 1; __i != __last; ++__i)
            std::iter_swap(__i, __first + (rand % ((__i - __first) + 1)));
}


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

static unsigned GCD(unsigned u, unsigned v) {
    while ( v != 0) {
        unsigned r = u % v;
        u = v;
        v = r;
    }
    return u;
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
void genSubsets(const std::vector<T>& s, int k, std::vector<T>& t, std::vector<std::vector<T >> &out, int q = 0, int r = 0) {
    if (k - q > s.size() - r) {
        return;
    }

    if (q == k) {
        out.push_back(t);
    } else {
        for (int i = r; i < s.size(); i++) {
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

