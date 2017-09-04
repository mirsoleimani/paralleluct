/* 
 * File:   Parser.cpp
 * Author: mirsoleimanisa
 * 
 * Created on March 23, 2016, 10:55 AM
 */

#include "paralleluct/state/Parser.h"
#include <iostream>
#include <fstream>

Parser::Parser() {
}

Parser::Parser(const Parser& orig) {
}

Parser::~Parser() {
}

polynomial Parser::parseFile(const char* filename) {
    std::ifstream f(filename);

    if (!f.is_open()) {
        std::cerr << "Could not read file " << std::string(filename) << std::endl;
        BOOST_ASSERT(false);
        exit(0);
    }

    unsigned int maxvar = 0;
    int c, t, lastvar = 0;
    term cur;
    polynomial pol;

    while ((c = f.get()) != ';') {
        if (c == ' ') { // start of new term
            if (!cur.empty()) {
                pol.push_back(cur);
                cur.clear();
            }
            
            int sign = f.get() == '-' ? -1 : 1; // sign required
            char next = f.peek();
            
            // check if there is a constant
            if (next >= '0' && next <= '9') {
                f >> t;
                cur.push_back(t * sign);
            } else {
                cur.push_back(sign);
            }
            continue;
        }
        if (c == '*') {
            continue;
        }

        if (c == '^') { // only allowed after variable
            f >> t;
            cur[lastvar] += t - 1;
            continue;
        }

        // we have found a variable, variables are in the alphabet
        unsigned int index = c - 'a' + 1;
        if (index >= cur.size()) {
            cur.resize(index + 1);
        }

        cur[index]++;
        lastvar = index;
        
        if (index > maxvar) {
            maxvar = index;
        }
    }

    // last term
    if (!cur.empty()) {
        pol.push_back(cur);
        cur.clear();
    }
    
    // TODO: add support for terms of different size
    for (term& t : pol) {
        t.resize(maxvar + 1);
    }

    return pol;
}

vector<vector<int>> Parser::MakeHexEdgeList(int dim) {
    int size = dim*dim;
    vector<vector<int>> eList;
    eList.resize(size);
    int counter =0;
    for (int row = 0; row < dim; row++) {
        for (int col = 0; col < dim; col++) {
            if (row == 0) {
                if (col == 0) {
                    eList[counter].push_back(POS(row, (col + 1), dim));
                    eList[counter].push_back(POS((row + 1), col, dim));
                } else if (col == dim - 1) {
                    eList[counter].push_back(POS((row + 1), col, dim));
                    eList[counter].push_back(POS((row + 1), (col - 1), dim));
                    eList[counter].push_back(POS((row), (col - 1), dim));
                } else {
                    eList[counter].push_back(POS(row, (col + 1), dim));
                    eList[counter].push_back(POS((row + 1), (col), dim));
                    eList[counter].push_back(POS((row + 1), (col - 1), dim));
                    eList[counter].push_back(POS(row, (col - 1), dim));

                }
            } else if (row == dim - 1) {
                if (col == 0) {
                    eList[counter].push_back(POS((row - 1), (col), dim));
                    eList[counter].push_back(POS((row - 1), (col + 1), dim));
                    eList[counter].push_back(POS((row), (col + 1), dim));
                } else if (col == dim - 1) {
                    eList[counter].push_back(POS((row), (col - 1), dim));
                    eList[counter].push_back(POS((row - 1), (col), dim));
                } else {
                    eList[counter].push_back(POS(row, (col - 1), dim));
                    eList[counter].push_back(POS((row - 1), (col), dim));
                    eList[counter].push_back(POS((row - 1), (col + 1), dim));
                    eList[counter].push_back(POS(row, (col + 1), dim));
                }
            } else {
                if (col == 0) {
                    eList[counter].push_back(POS((row - 1), (col), dim));
                    eList[counter].push_back(POS((row - 1), (col + 1), dim));
                    eList[counter].push_back(POS((row), (col + 1), dim));
                    eList[counter].push_back(POS((row + 1), (col), dim));
                } else if (col == dim - 1) {
                    eList[counter].push_back(POS((row + 1), (col), dim));
                    eList[counter].push_back(POS((row + 1), (col - 1), dim));
                    eList[counter].push_back(POS((row), (col - 1), dim));
                    eList[counter].push_back(POS((row - 1), (col), dim));
                } else {
                    eList[counter].push_back(POS((row), (col + 1), dim));
                    eList[counter].push_back(POS((row + 1), (col), dim));
                    eList[counter].push_back(POS((row + 1), (col - 1), dim));
                    eList[counter].push_back(POS((row), (col - 1), dim));
                    eList[counter].push_back(POS((row - 1), (col), dim));
                    eList[counter].push_back(POS((row - 1), (col + 1), dim));
                }

            }
            counter++;
        }
    }
    return eList;
}

vector<int> Parser::MakeHexLeftPos(int d) {
    int size = d*d;
    vector<int> lefPoslist;

    for (int i = 0; i < size; i++)
        if (i % d == 0)
            lefPoslist.push_back(i);
    return lefPoslist;
}