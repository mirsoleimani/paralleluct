/* 
 * File:   Parser.h
 * Author: mirsoleimanisa
 *
 * Created on March 23, 2016, 10:55 AM
 */

#ifndef PARSER_H
#define	PARSER_H

#include "Utilities.h"

class Parser {
public:
    Parser();
    Parser(const Parser& orig);
    virtual ~Parser();
    
    polynomial parseFile(const char* filename);
    vector<vector<int>> MakeHexEdgeList(int d);
    vector<int> MakeHexLeftPos(int d);
private:

};

#endif	/* PARSER_H */

