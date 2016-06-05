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
    
    polynomial parseFile(std::string filename);
private:

};

#endif	/* PARSER_H */

