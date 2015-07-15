/* 
 * File:   HexState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on 6 november 2014, 16:23
 */

#include <boost/nondet_random.hpp>

#include "GameTree.h"


#include "HexState.h"

HexGameState::HexGameState(int d) {
    dim = d;
    size = d*d;
    pjm = 2;
    moveCounter = 0;
    MakeBoard();
    ClearBoard();
}

HexGameState::HexGameState(const HexGameState& orig) {
    dim = orig.dim;
    size = orig.size;
    pjm = orig.pjm;
    moveCounter = orig.moveCounter;
    edges = orig.edges;
    dsboard = orig.dsboard;
    lefPos = orig.lefPos;

}

HexGameState& HexGameState::operator=(const HexGameState& orig) {
    if (this == &orig) {
        return *this;
    }
    dim = orig.dim;
    size = orig.size;
    pjm = orig.pjm;
    moveCounter = orig.moveCounter;
    edges = orig.edges;
    dsboard = orig.dsboard;
    lefPos = orig.lefPos;

    return *this;
}

HexGameState::~HexGameState() {
    dsboard.clear();
    edges.clear();
    lefPos.clear();
}

void HexGameState::NewGame() {
    pjm = 2;
    moveCounter = 0;
    ClearBoard();
}

bool HexGameState::GameOver() {
    if (moveCounter >= (2 * dim - 1) && (BLACK == EvaluateBoard(BLACK, TOPDOWN) || WHITE == EvaluateBoard(WHITE, LEFTRIGHT))) {
        return true;
    } else {
        return false;
    }
}

/**
 * 
 * @param row 
 * @param col
 * @param dim the x dimension of the hex board
 * @param eList the list of edges of cell (row,col)
 */
void HexGameState::MakeBoard() {
    dsboard.resize(dim * dim);
    edges.resize(dim * dim);
    //lefPos.resize(dim);

    int count = 0;
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            MakeEdges(i, j, edges[count++]);
//            if (POS(i,j,dim) % dim == 0)
//                lefPos.push_back(POS(i,j,dim));
        }
    }
    
    for(int i=0;i<size;i++)
        if( i % dim == 0 )
            lefPos.push_back(i);

}

void HexGameState::ClearBoard() {
    for (int pos = 0; pos < size; pos++)
        ClearStone(pos);
}

void HexGameState::MakeEdges(int row, int col, vector<int>& eList) {
    if (row == 0) {
        if (col == 0) {
            eList.push_back(POS(row, (col + 1), dim));
            eList.push_back(POS((row + 1), col, dim));
        } else if (col == dim - 1) {
            eList.push_back(POS((row + 1), col, dim));
            eList.push_back(POS((row + 1), (col - 1), dim));
            eList.push_back(POS((row), (col - 1), dim));
        } else {
            eList.push_back(POS(row, (col + 1), dim));
            eList.push_back(POS((row + 1), (col), dim));
            eList.push_back(POS((row + 1), (col - 1), dim));
            eList.push_back(POS(row, (col - 1), dim));

        }
    } else if (row == dim - 1) {
        if (col == 0) {
            eList.push_back(POS((row - 1), (col), dim));
            eList.push_back(POS((row - 1), (col + 1), dim));
            eList.push_back(POS((row), (col + 1), dim));
        } else if (col == dim - 1) {
            eList.push_back(POS((row), (col - 1), dim));
            eList.push_back(POS((row - 1), (col), dim));
        } else {
            eList.push_back(POS(row, (col - 1), dim));
            eList.push_back(POS((row - 1), (col), dim));
            eList.push_back(POS((row - 1), (col + 1), dim));
            eList.push_back(POS(row, (col + 1), dim));
        }
    } else {
        if (col == 0) {
            eList.push_back(POS((row - 1), (col), dim));
            eList.push_back(POS((row - 1), (col + 1), dim));
            eList.push_back(POS((row), (col + 1), dim));
            eList.push_back(POS((row + 1), (col), dim));
        } else if (col == dim - 1) {
            eList.push_back(POS((row + 1), (col), dim));
            eList.push_back(POS((row + 1), (col - 1), dim));
            eList.push_back(POS((row), (col - 1), dim));
            eList.push_back(POS((row - 1), (col), dim));
        } else {
            eList.push_back(POS((row), (col + 1), dim));
            eList.push_back(POS((row + 1), (col), dim));
            eList.push_back(POS((row + 1), (col - 1), dim));
            eList.push_back(POS((row), (col - 1), dim));
            eList.push_back(POS((row - 1), (col), dim));
            eList.push_back(POS((row - 1), (col + 1), dim));
        }

    }
}

void HexGameState::DoMove(int pos) {
    assert(pos >= 0 && pos < size && dsboard[pos].val == CLEAR && "can not put stone on board!\n");
    pjm = CLEAR - pjm;
    PutStone(pos);
    moveCounter++;
}

int HexGameState::UndoMove() {
    //    moveCounter--;
    //    int m = moves[moveCounter];
    //    ClearStone(m);
    //    pjm = CLEAR - pjm;
    //    return 1;
}

void HexGameState::UndoMoves(int beg) {
    //    int i=moveCounter;
    //
    //    while (i > beg) {
    //            UndoMove();
    //            i--;
    //    }
    //    
    //    int pos=moveCounter;
    //    while(pos>=0){
    //    dsboard[moves[pos]].parent=moves[pos];
    //    dsboard[moves[pos]].rank=0;
    //    pos--;
    //    }
    //    
    //    UpdateSet();
}

int HexGameState::CurrIndicator() {
    return moveCounter;
}

int HexGameState::PlyJustMoved() {
    return pjm;
}

void HexGameState::DoRandGame(GEN& engine) {
    vector<int> moves;
    GetMoves(moves, engine);

    int m = 0;
    while (!GameOver()) {
        DoMove(moves[m]);
        m++;
    }
}

/**
 * Find the empty position on the board and store them in a vector.
 * Apply a random shuffle on the vector.
 * @param moves empty vector
 * @param engine random number generator engine
 * @return a shuffled vector of moves
 */
int HexGameState::GetMoves(vector<int>& moves, GEN& gen) {
    assert(moves.size() == 0 && "The moves vector is not empty");
    for (int pos = 0; pos < size; pos++) {
        if (dsboard[pos].val == CLEAR)
            moves.push_back(pos);
    }
    assert(moves.size() <= size && "The number of untried moves is out of bound!\n");

    //DIST dist(0, moves.size() - 1);
//    DIST dist(0,1);
//    GEN gen(engine, dist);
    
    //boost::random_shuffle(moves, gen);
    std::random_shuffle(moves.begin(),moves.end(),gen);

    return moves.size();
}

/**
 * Find the empty position on the board.
 * @param moves an empty vector
 * @return a vector of moves
 */
int HexGameState::GetMoves(vector<int>& moves) {
    assert(moves.size() == 0 && "The moves vector is not empty");
    for (int pos = 0; pos < size; pos++) {
        if (dsboard[pos].val == CLEAR)
            moves.push_back(pos);
    }
    assert(moves.size() <= size && "The number of untried moves is out of bound!\n");
    return moves.size();
}

int HexGameState::GetMoves() {
    vector<int> moves;
    for (int pos = 0; pos < size; pos++) {
        if (dsboard[pos].val == CLEAR)
            moves.push_back(pos);
    }
    assert(moves.size() <= size && "The number of untried moves is out of bound!\n");
    return moves.size();
}

int HexGameState::PutStone(int pos) {
    dsboard[pos].val = pjm;
    MakeSet(pos);
    for (int j = 0; j < edges[pos].size(); j++) {
        Union(pos, edges[pos].at(j));
    }

    return pos;
}

void HexGameState::ClearStone(int pos) {
    dsboard[pos].val = CLEAR;
    ClearSet(pos);
}

float HexGameState::GetResult(int plyjm) {
    if (plyjm == BLACK) {
        return (plyjm == EvaluateBoard(BLACK, TOPDOWN)) ? 1.0 : 0.0;
    } else {
        return (plyjm == EvaluateBoard(WHITE, LEFTRIGHT)) ? 1.0 : 0.0;
    }
}

float HexGameState::EvaluateBoard(int ply, int direction) {
    float m, n;
    //m= EvaluateDijkstra(ply,direction);
    n = EvaluateDSet(ply, direction);
    //assert(m==n&&"evaluation incorrect!\n");
    return n;
}

float HexGameState::EvaluateDSet(int ply, int direction) {

    int player = CLEAR;
    if (direction == TOPDOWN) {
//#pragma unroll(8)
        for (int pos1 = 0; pos1 < dim; pos1++) {

        //cilk_for(int pos1 = 0; pos1 < dim; pos1++) {
            if (dsboard[pos1].val == ply) {
                int p = Find(pos1);
                if (dsboard[p].top && dsboard[p].bottom) {
                    //return ply;
                    player = ply;
                }
            }
        }
    } else if (direction == LEFTRIGHT) {
        //for (int pos1 = 0; pos1 < size; pos1++) {
        for(int pos1 = 0 ;pos1<dim;pos1++){
            //if (pos1 % dim == 0 && dsboard[pos1].val == ply) {
             if(dsboard[lefPos[pos1]].val == ply){
                //int p = Find(pos1);
                 int p = Find(lefPos[pos1]);
                if (dsboard[p].left && dsboard[p].right) {
                    //return ply;
                    player = ply;
                }
            }
        }
    }
    //return CLEAR;
    return player;
}

float HexGameState::EvaluateDijkstra(int ply, int direction) {
    vector<int> src;
    vector<int> dest;
    src.resize(size, 0);
    dest.resize(size, 0);
    src.clear();
    int pos = 0;
    for (int i = 0; i < size; i++) {
        dest[i] = false;
    }
    if (direction == TOPDOWN) {
        for (int col = 0; col < dim; col++) {
            if (dsboard[POS(0, col, dim)].val == ply) {
                src.push_back(POS(0, col, dim));
                dest[POS(0, col, dim)] = true;
            }
        }
        while (!src.empty()) {
            pos = src.back();
            src.pop_back();
            if ((size - dim) <= pos && pos < size) {
                return ply;
            } else {
                for (int j = 0; j < edges[pos].size(); j++) {
                    if (dest[edges[pos].at(j)] == false && dsboard[edges[pos].at(j)].val == ply) {
                        src.push_back(edges[pos].at(j));
                        dest[edges[pos].at(j)] = true;
                    }
                }

            }
        }
    } else if (direction == LEFTRIGHT) {
        for (int row = 0; row < dim; row++) {
            if (dsboard[POS(row, 0, dim)].val == ply) {
                src.push_back(POS(row, 0, dim));
                dest[POS(row, 0, dim)] = true;
            }
        }
        while (!src.empty()) {
            pos = src.back();
            src.pop_back();
            if ((pos + 1) % dim == 0) {
                return ply;
            } else {
                for (int j = 0; j < edges[pos].size(); j++) {
                    if (dest[edges[pos].at(j)] == false && dsboard[edges[pos].at(j)].val == ply) {
                        src.push_back(edges[pos].at(j));
                        dest[edges[pos].at(j)] = true;
                    }
                }

            }
        }
    }
    src.clear();
    dest.clear();
    return CLEAR;
}

void HexGameState::MakeSet(int pos) {
    dsboard[pos].parent = pos;
    dsboard[pos].rank = 0;

    if (pos < dim) {
        dsboard[pos].top = 1;
        dsboard[pos].bottom = 0;
    } else if ((size - dim) <= pos && pos < size) {
        dsboard[pos].bottom = 1;
        dsboard[pos].top = 0;
    }

    if (pos % dim == 0) {
        dsboard[pos].left = 1;
        dsboard[pos].right = 0;
    } else if ((pos + 1) % dim == 0) {
        dsboard[pos].right = 1;
        dsboard[pos].left = 0;
    }
}

void HexGameState::ClearSet(int pos) {
    dsboard[pos].parent = -1;
    dsboard[pos].rank = 0;
    for (int i = 0; i < dsboard.size(); i++) {
        if (dsboard[i].parent == pos) {
            dsboard[i].parent = i;
            dsboard[pos].rank = 0;
        }
    }
    if (pos < dim) {
        dsboard[pos].top = 1;
        dsboard[pos].bottom = 0;
    } else if ((size - dim) <= pos && pos < size) {
        dsboard[pos].bottom = 1;
        dsboard[pos].top = 0;
    }

    if (pos % dim == 0) {
        dsboard[pos].left = 1;
        dsboard[pos].right = 0;
    } else if ((pos + 1) % dim == 0) {
        dsboard[pos].right = 1;
        dsboard[pos].left = 0;
    }

}

void HexGameState::UpdateSet() {
    for (int pos = 0; pos < dsboard.size(); pos++) {
        if (dsboard[pos].val != CLEAR) {
            for (int j = 0; j < edges[pos].size(); j++) {
                Union(pos, edges[pos].at(j));
            }
        }
    }
}

int HexGameState::Find(int pos) {
    assert(pos != -1 && "position of -1 is wrong!\n");
    if (dsboard[pos].parent != pos)
        dsboard[pos].parent = Find(dsboard[pos].parent);
    return dsboard[pos].parent;
}

void HexGameState::Union(int pos1, int pos2) {
    assert(pos1 != -1 && "pos1 of -1 is wrong!\n");
    assert(pos2 != -1 && "pos2 of -1 is wrong!\n");
    if (dsboard[pos1].val == dsboard[pos2].val) {
        int xroot = Find(pos1);
        int yroot = Find(pos2);
        if (xroot == yroot) {
            return;
        }

        //merge two positions which does not belong to the same set. 
        if (dsboard[xroot].rank < dsboard[yroot].rank) {
            dsboard[xroot].parent = yroot;
            dsboard[yroot].top |= dsboard[xroot].top;
            dsboard[yroot].bottom |= dsboard[xroot].bottom;
            dsboard[yroot].left |= dsboard[xroot].left;
            dsboard[yroot].right |= dsboard[xroot].right;
        } else if (dsboard[xroot].rank > dsboard[yroot].rank) {
            dsboard[yroot].parent = xroot;
            dsboard[xroot].top |= dsboard[yroot].top;
            dsboard[xroot].bottom |= dsboard[yroot].bottom;
            dsboard[xroot].left |= dsboard[yroot].left;
            dsboard[xroot].right |= dsboard[yroot].right;
        } else {
            dsboard[yroot].parent = xroot;
            dsboard[xroot].top |= dsboard[yroot].top;
            dsboard[xroot].bottom |= dsboard[yroot].bottom;
            dsboard[xroot].left |= dsboard[yroot].left;
            dsboard[xroot].right |= dsboard[yroot].right;
            dsboard[xroot].rank++;
        }
    }
}

void HexGameState::Print() {
    cout << "  ";
    for (int j = 0; j < dim; j++)
        cout << "/  \\ ";
    cout << endl;
    for (int i = 0; i < dim; i++) {
        for (int k = 0; k < i; k++)
            cout << "  ";
        cout << " |";
        for (int j = 0; j < dim; j++) {
            if (dsboard[POS(i, j, dim)].val == CLEAR) {
                cout << setw(4) << POS(i, j, dim); // POS(i, j, dim); //POS(i, j, dim);
                cout << "|";
            } else if (dsboard[POS(i, j, dim)].val == WHITE) {
                cout << setw(4) << "W ";
                cout << "|";
            } else if (dsboard[POS(i, j, dim)].val == BLACK) {
                cout << setw(4) << "B ";
                cout << "|";
            }
        }
        cout << endl;
        if (i < dim - 1) {
            for (int k = 0; k < i + 1; k++)
                cout << "  ";
            for (int j = 0; j < dim; j++)
                cout << "\\  / ";
            cout << "\\  ";
            cout << endl;
        }

    }
    for (int k = 0; k < dim; k++)
        cout << "  ";
    for (int j = 0; j < dim; j++)
        cout << "\\  / ";
    cout << "\n";

    //PrintDSet();
}

void HexGameState::PrintDSet() {

    for (int pos = 0; pos < dsboard.size(); pos++) {
        cout << pos;
        cout << " child of " << dsboard[pos].parent << endl; //<<" top "<<dsboard[pos].top<<" botom "<<dsboard[pos].bottom
        //<<" left "<<dsboard[pos].left<<" right "<<dsboard[pos].right<<endl;
    }

}

void HexGameState::PrintDSet2(int pos) {
    if (dsboard[pos].parent == pos)
        cout << " child of " << pos; //<<" top "<<dsboard[pos].top<<" botom "<<dsboard[pos].bottom
        //<<" left "<<dsboard[pos].left<<" right "<<dsboard[pos].right<<endl;
    else
        PrintDSet2(dsboard[pos].parent);
}
