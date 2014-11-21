/* 
 * File:   HexState.cpp
 * Author: mirsoleimanisa
 * 
 * Created on 6 november 2014, 16:23
 */

#include "GameTree.h"


#include "HexState.h"

#define TOPDOWN 11
#define LEFTRIGHT 22
#define BLACK 1
#define WHITE 2
#define CLEAR 3

HexGameState::HexGameState(int d) {
    dim = d;
    size = d*d;
    pjm = 2;
    moveCounter = 0;
    MakeBoard();
    ClearBoard();
    GetMoves(currMoves);
    path = new long int[size];
    src.resize(size,0);
    dest.resize(size,0);
}

HexGameState::HexGameState(const HexGameState& orig) {
}

HexGameState::~HexGameState() {
    delete(path);
}

void HexGameState::NewGame() {
    pjm = 2;
    moveCounter = 0;
    ClearBoard();
    GetMoves(currMoves);
    src.resize(size,0);
    dest.resize(size,0);
}

bool HexGameState::GameOver() {
    if (BLACK == EvaluateBoard(BLACK, TOPDOWN) || WHITE == EvaluateBoard(WHITE, LEFTRIGHT)) {
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
    board.resize(dim * dim);
    edges.resize(dim * dim);

    int count = 0;
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            MakeEdges(i, j, edges[count++]);
        }
    }
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

void HexGameState::DoMove(int move) {
    assert(move >= 0 && move < size && board[move] == 0 && "can not put stone on board!\n");
    pjm = 3 - pjm;
    PutStone(move);
    currMoves.erase(std::remove(currMoves.begin(), currMoves.end(), move), currMoves.end());
    path[moveCounter] = move;
    moveCounter++;
}

int HexGameState::UndoMove() {
    moveCounter--;
    int m = path[moveCounter];
    ClearStone(m);
    currMoves.push_back(m);
    pjm = 3 - pjm;
    return 1;
}

int HexGameState::CurrIndicator() {
    return moveCounter;
}

int HexGameState::PlyJustMoved() {
    return pjm;
}

int HexGameState::PutStone(int pos) {
    board[pos] = pjm;
    return pos;
}

int HexGameState::GetRandMove() {
    return currMoves[RandInt(currMoves.size() - 1)];
}

int HexGameState::GetMoves(vector<int>& moves) {
    moves.clear();
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            if (board[POS(i, j, dim)] == 0) {
                moves.push_back(POS(i, j, dim));
            }
        }
    }
}

void HexGameState::ClearStone(int pos) {
    board[pos] = 0;
}

void HexGameState::ClearBoard() {
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            ClearStone(POS(i, j, dim));
        }
    }
}

float HexGameState::GetResult(int plyjm) {
    if (plyjm == BLACK) {
        return (plyjm == EvaluateBoard(BLACK, TOPDOWN)) ? 1.0 : 0.0;
    } else {
        return (plyjm == EvaluateBoard(WHITE, LEFTRIGHT)) ? 1.0 : 0.0;
    }

}

float HexGameState::EvaluateBoard(int ply, int direction) {
    src.clear();
    int pos = 0;
    for (int i = 0; i < size; i++) {
        dest[i] = false;
    }
    if (direction == TOPDOWN) {
        for (int col = 0; col < dim; col++) {
            if (board[POS(0, col, dim)] == ply) {
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
                    if (dest[edges[pos].at(j)] == false && board[edges[pos].at(j)] == ply) {
                        src.push_back(edges[pos].at(j));
                        dest[edges[pos].at(j)] = true;
                    }
                }

            }
        }
    } else if (direction == LEFTRIGHT) {
        for (int row = 0; row < dim; row++) {
            if (board[POS(row, 0, dim)] == ply) {
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
                    if (dest[edges[pos].at(j)] == false && board[edges[pos].at(j)] == ply) {
                        src.push_back(edges[pos].at(j));
                        dest[edges[pos].at(j)] = true;
                    }
                }

            }
        }
    }
    return CLEAR;
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
            if (board[POS(i, j, dim)] == 0) {
                cout << setw(4) << POS(i, j, dim); // POS(i, j, dim); //POS(i, j, dim);
                cout << "|";
            } else if (board[POS(i, j, dim)] == WHITE) {
                cout << setw(4) << "W ";
                cout << "|";
            } else if (board[POS(i, j, dim)] == BLACK) {
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
}
