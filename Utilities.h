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
#include <sys/time.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/range/algorithm/random_shuffle.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

using namespace std;

#ifndef UTILITIES_H
#define	UTILITIES_H

#define POS(i,j,dim) i*dim+j       
#define RandInt(n) ((int)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandChar(n) ((char)((float)(n)*rand()/(RAND_MAX+1.0)))
#define RandFloat(n) ((float)(v)*rand()/(RAND_MAX+1.0))
#define MAX(n,m) ((n)>(m)?(n):(m))
#define INF 10000

#define TOPDOWN 11
#define LEFTRIGHT 22
#define BLACK 1
#define WHITE 2
#define CLEAR 3

//boost::mt19937 gen;

template <typename T>
string NumToStr(T num){
    ostringstream ss;
    ss<<std::setprecision(4)<<num;
    return ss.str();
}

class Timer
{
public:
    Timer() { clock_gettime(CLOCK_REALTIME, &beg_); }

    double elapsed() {
        clock_gettime(CLOCK_REALTIME, &end_);
        return end_.tv_sec - beg_.tv_sec +
            (end_.tv_nsec - beg_.tv_nsec) / 1000000000.;
    }

    void reset() { clock_gettime(CLOCK_REALTIME, &beg_); }

private:
    timespec beg_, end_;
};

template <class T>
inline void PRINT_ELEMENTS (const T& coll, const char* optcstr="")
{
    typename T::const_iterator pos;

    std::cout << optcstr;
    for (pos=coll.begin(); pos!=coll.end(); ++pos) {
        std::cout << *pos << ' ';
    }
    std::cout << std::endl;
}
#endif	/* UTIITIES_H */

