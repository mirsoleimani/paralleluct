/* 
 * File:   CSE.h
 * Author: mirsoleimanisa
 *
 * Created on May 17, 2016, 3:22 PM
 */

#ifndef CSE_H
#define	CSE_H

#include <map>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>
#include "Utilities.h"

class CSE {
public:
    CSE();
    CSE(const CSE& orig);
    virtual ~CSE();

    class Operation;
    class Constant;
    class Variable;

    class Node {
    public:

        enum TYPE : signed int {
            CON, VAR, OP
        };
        TYPE type;

        Node(TYPE type) : type(type) {

        }

        virtual ~Node() {

        }

        virtual Node* clone() = 0;

        virtual void print() = 0;

        virtual int compare(Node* b) const = 0;

        virtual int hash() = 0;

        virtual int evaluate(const std::vector<int>& varvals) = 0;
    };

    /**
     * N-ary node that contains all subnodes that use the same operator.
     * This conveniently groups commutative and associative nodes.
     */
    class Operation : public Node {
    public:

        enum Operator : signed int {
            MUL, ADD
        };

        Operation(const std::vector<Node*>&& children, Operator op) : children(children), op(op), Node(OP) {
            assert(children.size() > 1);
        }

        Operation(const std::vector<Node*>& children, Operator op) : children(children), op(op), Node(OP) {
            assert(children.size() > 1);
        }

        virtual ~Operation() {
            for (int i = 0; i < children.size(); i++) {
                delete children[i];
            }
        }

        virtual Node* clone() {
            std::vector<Node*> cChildren(children.size());

            for (int i = 0; i < children.size(); i++) {
                cChildren[i] = children[i]->clone();
            }

            return new Operation(cChildren, op);
        }

        void print() {
            std::cout << "(";

            for (int i = 0; i < children.size() - 1; i++) {
                children[i]-> print();
                std::cout << (op == ADD ? " + " : " * ");
            }
            children[children.size() - 1]->print();
            std::cout << ")";
        }

        virtual int compare(Node* bb) const {
            if (type != bb->type) {
                return type - bb->type;
            }

            Operation* b = (Operation*) bb;

            if (b->op != op) {
                return op - b->op;
            }

            if (children.size() != b->children.size()) {
                return (signed int) children.size() - (signed int) b->children.size();
            }

            // FIXME : assumes elements are sorted.
            for (int i = 0; i < children.size(); i++) {
                int comp = children[i]->compare(b->children[i]);

                if (comp != 0) {
                    return comp;
                }
            }

            return 0;
        }

        int hash() {
            int hashcode = op * 1001;

            // should be commutative
            for (Node*& c : children) {
                hashcode = hashcode + c->hash() * 31;
            }

            return hashcode;
        }

        int evaluate(const std::vector<int>& varvals) {
            int res = op == MUL ? 1 : 0;

            for (Node*& c : children) {
                res = op == MUL ? res * c->evaluate(varvals) : res + c->evaluate(varvals);
            }

            return res;
        }

        std::vector<Node*> children;
        Operator op;
    };

    class Variable : public Node {
    private:
        int var;
    public:

        Variable(int var) : Node(VAR), var(var) {
        }

        virtual ~Variable() {
        }

        virtual Node* clone() {
            return new Variable(var);
        }

        int getVar() {
            return var;
        }

        void print() {
            std::string varnames = "abcdefghijklmnopqrstuvwxyz";
            assert(var < varnames.size() || var >= 100);

            if (var < 100) {
                std::cout << varnames[var];
            } else {
                std::cout << "T" << var;
            }
        }

        virtual int compare(Node* bb) const {
            if (type != bb->type) {
                return type - bb->type;
            }

            Variable* b = (Variable*) bb;

            if (var == b->var) return 0;
            return var - b->var;
        }

        int hash() {
            return 10001 + var; // TODO: improve
        }

        int evaluate(const std::vector<int>& varvals) {
            assert(var < varvals.size());
            return varvals[var];
        }
    };

    class Constant : public Node {
    private:
        int val;
    public:

        Constant(int val) : val(val), Node(CON) {
        }

        virtual ~Constant() {
        }

        virtual Node* clone() {
            return new Constant(val);
        }

        void print() {
            std::cout << val;
        }

        int compare(Node* bb) const {
            if (type != bb->type) {
                return type - bb->type;
            }

            Constant* b = (Constant*) bb;

            if (val == b->val) return 0;
            return val - b->val;
        }

        int hash() {
            return val;
        }

        int getVal() {
            return val;
        }

        int evaluate(const std::vector<int>& varvals) {
            return val;
        }
    };

    // TOOD: build routine for local ordering
    Node* buildTree(const polynomial& pol, const std::vector<int> order);
    static Node* buildTree(const polynomial& pol);
    Node* doCSE(Node* tree);

    Node* doSimpleCSE(Node* tree,
            std::map< Node*, Node*, bool(*)(Node * const&, Node * const&) >& replaceMap,
            std::vector<Node*>& instrTree);

    unsigned int countSimpleCSE(Node* tree);
    Node* buildComAssocTree(Node* tree);
    int countOperators(const Node* tree);
private:
    Node* buildTree(const polynomial& pol, const std::vector<int>& order, int orderindex);

    int countOperation(const Node* tree);
    Node* findSubset(Node* root, Node* target, std::vector<Node*>& ptrs);
    Node* replaceCSEsInTree(Node* tree, std::vector<CSE::Node*>& cses);
    Node* findAndReplaceAll(Node* root, Node* target, Node* replaceBy, int* count);
    Node* doMaths(Node* tree);
    Node* addSameTerms(Node* tree);
    Node* doGeneralisedHorner(Node* root);
};

#endif	/* CSE_H */

