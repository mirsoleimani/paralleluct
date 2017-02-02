/* 
 * File:   SampleGameState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on February 2, 2017, 6:28 PM
 */

#include "SampleGameState.h"

SampleGameState::SampleGameState() {
    _moves = new (_Offload_shared_malloc(sizeof (vector<int>)))
            _Cilk_shared
            vector<int, __offload::shared_allocator<int> >;
    for (unsigned int i = 0; i < 1000; i++) {
        (*_moves).push_back(i);
    }
}

SampleGameState::SampleGameState(const SampleGameState& orig) {
    
}

SampleGameState::~SampleGameState() {
}


