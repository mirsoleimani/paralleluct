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
#include <assert.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <atomic>
#include <sys/time.h>
#include <stddef.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>



using namespace std;

#ifndef UTILITIES_H
#define	UTILITIES_H

//#define NDEBUG

#define POS(i,j,dim) i*dim+j       
#define RandInt(n) ((int)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandChar(n) ((char)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandFloat(n) ((float)(v)*rand()/(RAND_MAX+1.0))
#define MAX(n,m) ((n)>(m)?(n):(m))
#ifndef max
  #define max(n,m) (((n) > (m)) ? (n) ; (m))
#endif
#define INF 10000
static inline float RandFloat_T(float low, float high, unsigned int *seed){
    float t = (float)rand_r(seed) / (float)RAND_MAX;
    return (1.0 - t) * low + t * high;
}

static const int TOPDOWN = 11;
static const int LEFTRIGHT = 22;
static const int BLACK = 1;
static const int WHITE = 2;
static const int CLEAR = 3;
static const float WIN=1.0;
static const float LOST=0.0;
static const float DRAW=0.5;

typedef int MOVE;


//typedef std::mt19937 ENG; // Mersenne Twister
//typedef std::uniform_int<int> DIST; // Uniform Distribution
//typedef std::variate_generator<ENG&, DIST> GEN; // Variate generator

typedef boost::mt19937 ENG; // Mersenne Twister
typedef boost::uniform_int<int> DIST;
typedef boost::variate_generator<ENG, DIST > GEN;

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
    timespec beg_, end_;
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
#endif	/* UTIITIES_H */

