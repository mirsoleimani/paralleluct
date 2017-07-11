/* 
 * File:   PinsState.h
 * Author: Alfons Laarman
 *
 */

#include "PinsState.h"

#include "ltsmin/src/hre/config.h"
#include "ltsmin/src/pins-lib/pins.h"
#include "ltsmin/src/mc-lib/cctables.h"
#undef Print

model_t             model;

PinsState::PinsState(){
}

PinsState::PinsState(const char *fileName) {
    model = GBcreateBase ();

    
    cct_map_t *tables = cct_create_map (false); // HRE-aware object
    
    table_factory_t     factory = cct_create_table_factory  (tables);
    GBsetChunkMap (model, factory); //HREgreyboxTableFactory());

    std::cout << "Loading model from "<< fileName;

    GBloadFile (model, fileName, &model);
}

int PinsState::CountMultiplications() {
    return 1;
}

/**
 * Find the untried moves in the current state.
 * @param moves an empty vector
 * @return number of untried moves
 */
int PinsState::GetMoves(vector<int>& moves) {
    assert(moves.size() == 0 && "The moves vector should be empty!\n");
    return moves.size();
}

float PinsState::GetResult(int plyjm) {
    if (plyjm == WHITE)
        return 1;
    else
        return 1;
}

void PinsState::Evaluate() {
    //Tree building assumes that var 0 is constant
}

bool PinsState::IsTerminal() {
    if (true) {
        return true;
    } else {
        return false;
    }
}

int PinsState::GetPlyJM() {
    return 1;
}

void PinsState::SetMove(int move) {

}

void PinsState::SetPlayoutMoves(vector<int>& moves) {
    if (IsTerminal()) {
        return;
    }
}

int PinsState::GetPlayoutMoves(vector<int>& moves) {
    return GetMoves(moves);
}

int PinsState::GetMoveCounter(){
    return 2;
}

void PinsState::UndoMoves(int origMoveCounter) {
}
void PinsState::Reset() {
}

void PinsState::Print() {
    std::cout << std::endl;
}

void PinsState::PrintToFile(char* fileName) {
    std::ofstream ofs(fileName);

    ofs << std::endl;
}
