/* 
 * File:   Parser.cpp
 * Author: mirsoleimanisa
 * 
 * Created on March 23, 2016, 10:55 AM
 */

#include "Parser.h"
#include <iostream>
#include <fstream>

Parser::Parser() {
}

Parser::Parser(const Parser& orig) {
}

Parser::~Parser() {
}

polynomial Parser::parseFile(std::string filename) {
    std::ifstream f(filename.c_str());

    if (!f.is_open()) {
        std::cerr << "Could not read file " << std::string(filename) << std::endl;
        BOOST_ASSERT(false);
        exit(0);
    }

    int maxvar = 0;
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
        int index = c - 'a' + 1;
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

