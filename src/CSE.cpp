/* 
 * File:   CSE.cpp
 * Author: mirsoleimanisa
 * 
 * Created on May 17, 2016, 3:22 PM
 */


#include "paralleluct/state/CSE.h"

CSE::CSE() {
}

CSE::CSE(const CSE& orig) {
}

CSE::~CSE() {
    // TODO: free
}

void sortedInsert(std::vector<CSE::Node*>& s, CSE::Node*& ins) {
    s.resize(s.size() + 1);

    int i = s.size() - 1;
    for (; i > 0 && ins->compare(s[i - 1]) < 0; i--) {
        s[i] = s[i - 1];
    }

    s[i] = ins;
}

// process global order and extract gcds

CSE::Node* CSE::buildTree(const polynomial& pol, const std::vector<int>& order, int orderindex) {
    polynomial constants, highorder; // TODO: references?
    int mincount = 0, termcount = 0, gcd = 0;

    if (orderindex >= order.size()) {
        return buildTree(pol); // build tree from term
    }

    // find parts that are constant in order[i]
    for (int i = 0; i < pol.size(); i++) {
        if (pol[i][order[orderindex]] == 0) {
            constants.push_back(pol[i]);
        } else {
            if (mincount == 0 || mincount > pol[i][order[orderindex]]) {
                mincount = pol[i][order[orderindex]];
            }

            if (gcd == 0) gcd = abs(pol[i][0]);
            gcd = GCD(gcd, abs(pol[i][0]));

            highorder.push_back(pol[i]);
            termcount++;
        }
    }

    if (termcount < 2) { // do not split if it is not there or if it is 1, go to next
        return buildTree(pol, order, orderindex + 1);
    }

    for (int i = 0; i < highorder.size(); i++) {
        highorder[i][order[orderindex]] -= mincount;
        highorder[i][0] /= gcd;
    }

    Node* right = buildTree(highorder, order, orderindex);

    Node* mulNode = new Variable(order[orderindex] - 1);

    if (gcd > 1) {
        mulNode = new Operation({new Constant(gcd), mulNode}, Operation::MUL);
    }

    for (int i = 1; i < mincount; i++) {
        mulNode = new Operation({mulNode, new Variable(order[orderindex] - 1)}, Operation::MUL);
    }

    mulNode = new Operation({mulNode, right}, Operation::MUL);
    Node* topNode = mulNode;

    if (constants.size() > 0) {
        Node* left = buildTree(constants, order, orderindex + 1);
        topNode = new Operation({left, mulNode}, Operation::ADD);
    }

    return topNode;
}

// build tree without horner

CSE::Node* CSE::buildTree(const polynomial& pol) {
    if (pol.size() == 1) { // one term
        Node* term = new Constant(pol[0][0]); // TODO: filter 1 and -1?

        for (int i = 1; i < pol[0].size(); i++) {
            if (pol[0][i] > 0) {
                for (int j = 0; j < pol[0][i]; j++) {
                    term = new Operation({term, new Variable(i - 1)}, Operation::MUL);
                }
            }
        }

        return term;
    }

    polynomial p = pol;
    p.pop_back();
    polynomial r;
    r.push_back(pol.back());

    return new Operation({buildTree(p), buildTree(r)}, Operation::ADD);
}

CSE::Node* CSE::buildTree(const polynomial& pol, const std::vector<int> order) {
    // the root is either a plus or a prod, depending on whether there is a rest
    // term. I could also make it always a plus with a 0 as rest term
    return buildTree(pol, order, 0);
}

// Builds an ordered, commutative associative tree

CSE::Node* CSE::buildComAssocTree(Node* tree) {
    Operation* tt = dynamic_cast<Operation*> (tree);
    assert(tt != NULL);

    Operation::Operator curOp = tt->op;
    std::vector<Node*> list;
    std::vector<Node*> queue;

    queue.push_back(tree);

    while (!queue.empty()) {
        Node* n = queue.back();
        queue.pop_back();

        Operation* o = dynamic_cast<Operation*> (n);

        if (o != NULL && o->op == curOp) {
            for (Node*& a : o->children) {
                queue.push_back(a);
            }

            o->children.clear(); // prevent deletion of children
            delete n;
        } else {
            if (o != NULL) { // we have another op, recursively sort it
                n = buildComAssocTree(n);
            }

            list.push_back(n);
        }
    }

    // sort list, TODO: improve comparator
    std::sort(list.begin(), list.end(), [](Node* a, Node * b) {
        return a->compare(b) < 0;
    });

    assert(list.size() > 1);

    return new Operation(list, curOp);
}

int CSE::countOperators(const Node* tree) {
    const Operation* tt = dynamic_cast<const Operation*> (tree);

    if (tt == NULL) {
        return 0;
    }

    int count = tt->children.size() - 1;

    // check for *1 or *-1, this is not an operation
    Constant* c = dynamic_cast<Constant*> (tt->children[0]);
    if (c != NULL && (c->getVal() == 1 || c->getVal() == -1)) {
        count--;
    }


    for (Node* t : tt->children) {
        count += countOperators(t);
    }

    return count;
}

// replaces target in root by replaceBy. This is a pointer replacement.
// target is freed. Stores the number of times the replacement has been done in count.

CSE::Node* CSE::findAndReplaceAll(Node* root, Node* target, Node* replaceBy, int* count) {
    if (root->compare(target) == 0) {
        (*count)++;
        return replaceBy;
    }

    Operation* tt = dynamic_cast<Operation*> (root);
    Operation* tat = dynamic_cast<Operation*> (target);
    assert(tat != NULL);

    if (tt == NULL) {
        return root;
    }

    bool recheck = true;

    while (recheck) {
        recheck = false;

        // does a subset match?
        std::vector<Node*> temp(tat->children.size());
        std::vector < std::vector < Node* >> subsets; // TODO: allocate (n ncr k) units?
        genSubsets(tt->children, tat->children.size(), temp, subsets); // only pick subsets of target size

        // TODO: check if subsets is ordered, else the algorithm fails!
        for (auto s : subsets) {
            // create new node
            Operation* newNode = new Operation(s, tt->op);

            if (target->compare(newNode) == 0) {
                // do replace here

                std::vector<Node*> newchildren;

                int posVer = 0;

                for (Node*& c : tt->children) {
                    if (posVer < s.size() && c == s[posVer]) {
                        posVer++;

                        //std::cout << "Deleted " << c << std::endl;
                        delete c; // remove from the tree, FIXME: this breaks the target!
                    } else {
                        newchildren.push_back(c);
                    }
                }

                assert(newchildren.size() == tt->children.size() - tat->children.size());

                tt->children.clear();
                tt->children.insert(tt->children.end(), newchildren.begin(),
                        newchildren.end());


                // at least one element should be left, else it is not a subset
                assert(tt->children.size() > 0);
                // insert the new variable, sorted
                sortedInsert(tt->children, replaceBy);

                (*count)++;
                // then recheck other subsets, since some are replaced!
                recheck = true;
                break;
            }

            newNode->children.clear(); // prevent destruction of children
            delete newNode;
        }
    }


    // continue to check the children
    for (int i = 0; i < tt->children.size(); i++) {
        tt->children[i] = findAndReplaceAll(tt->children[i], target, replaceBy, count);
    }

    // sort children
    std::sort(tt->children.begin(), tt->children.end(), [&](Node* a, Node * b) {
        return a->compare(b) < 0;
    });


    return root;
}

// expects sorted by size and by frequency
// replaces CSE in tree and in its own tree. Removes CSEs that are not used.

CSE::Node* CSE::replaceCSEsInTree(Node* tree, std::vector<CSE::Node*>& cses) {
    std::vector<char> used(cses.size()); // is a CSE used?

    // work backwards, since we want to make the biggest changes first
    // walk through all the subsets again... Could we do all the cses at the same
    // time?
    for (int i = cses.size() - 1; i >= 0; i--) {
        int count = 0;
        tree = findAndReplaceAll(tree, cses[i], new Variable(i + 100), &count);

        if (count > 0) {
            used[i] = true;
        }

#ifdef DEBUG_BOGUS
        static int a = 0;
        if (count == 1) {
            std::cout << "Bogus substitution " << a << std::endl; // only when it is not used in the tree itself
            a++;
        }
#endif
    }

    tree->print();
    std::cout << std::endl;

    // now find matches of smaller in bigger ones
    // start with biggest subexps
    for (int i = cses.size() - 1; i >= 0; i--) {
        Node* tmp = new Variable(i + 100); // the ith CSE

        for (int j = i + 1; j < cses.size(); j++) {
            if (used[j]) {
                int count = 0;
                cses[j] = findAndReplaceAll(cses[j], cses[i], tmp, &count);

                if (count > 0) {
                    used[i] = true;
                }
            }
        }
    }

    std::vector<Node*> newtree;

    for (int i = 0; i < cses.size(); i++) {
        if (used[i]) {
            newtree.push_back(cses[i]);
        }
    }

    std::cout << "Removed " << cses.size() - newtree.size() << " unused CSEs" << std::endl;

    // FIXME: this messes up the number of the variables! Fix the numbering
    cses = newtree; // replace tree

    for (int i = 0; i < cses.size(); i++) {
        std::cout << "T" << (i + 100) << " = ";
        cses[i]->print();
        std::cout << std::endl;
    }

    return tree;
}

// generalised local Horner on a CSE tree
// After CSE we know that the common Horner factors must be Constants or Variables.
// So count single terms in a PLUS expression
// can we use this to our advantage? We could use the variable int instead of a hashtree
// TODO: does not remove powers, so x*x + x*x*z => x(x + x*z) 
// FIXME: make sure the list is sorted
// FIXME: merge operators! Can this happen?

CSE::Node* CSE::doGeneralisedHorner(Node* root) {
    Operation* tt = dynamic_cast<Operation*> (root);

    if (tt == NULL || tt->op != Operation::ADD) {
        return root;
    }

    // check all single expression of children
    auto my_hash = [](Node * const& a) {
        return a->hash();
    };

    auto my_eq = [](Node * const& a, Node * const& b) {
        return a->compare(b) == 0;
    };

    std::unordered_map < Node*, int, decltype(my_hash), decltype(my_eq) > csemap(0, my_hash, my_eq);
    std::unordered_set < Node*, decltype(my_hash), decltype(my_eq) > localmap(0, my_hash, my_eq);

    assert(tt->children.size() > 1);

    for (Node* c : tt->children) {
        Operation* cc = dynamic_cast<Operation*> (c);

        if (cc == NULL) {
            csemap[c]++;
            continue;
        }

        assert(cc->op == Operation::MUL);

        localmap.clear();
        for (Node* k : cc->children) {
            if (localmap.find(k) != localmap.end()) {
                continue;
            }
            csemap[k]++;
            localmap.insert(k);
        }
    }

    assert(csemap.size() > 0);

    int max = 0;
    std::vector<Node*> cses;

    // filter out highest occurring cses
    for (auto a : csemap) {
        if (a.second > max) {
            cses.clear();
            max = a.second;
        }

        if (a.second == max) {
            cses.push_back(a.first);
        }
    }

    if (max == 0) {
        std::cout << csemap.size() << std::endl;
    }

    csemap.clear(); // free some memory

    assert(max > 0);

    if (max == 1) { // not applicable
        return root;
    }

    // split up structure
    Node* a = cses[rand() % cses.size()]; // pick random node

    int removalcount = 1; // TODO: support powers

    std::vector<Node*> rest; // rest + a * sub
    std::vector<Node*> sub;

    for (Node* c : tt->children) {
        if (c->compare(a) == 0) {
            sub.push_back(new Constant(1));
            continue;
        }

        Operation* ttt = dynamic_cast<Operation*> (c);

        if (ttt == NULL) { // non-op term that is not equiv to a
            rest.push_back(c);
            continue;
        }

        assert(ttt->op == Operation::MUL);

        int powcount = 0; // count occurrences
        for (Node* cc : ttt->children) {
            if (cc->compare(a) == 0) {
                powcount++;
            }
        }

        if (powcount == 0) {
            rest.push_back(c);
        } else {
            // remove removalcount occurrences
            // what is the max occurrence count after CSE? I would say second power or third power.
            // imagine x^n, n > 2. Then split up:
            // EVEN: x^(n/2) * x^(n/2) = T^2
            // UNEVEN: x^(n//2) * x^(n//2) * x = T^2x
            // third power only happens if x^3 occurs only once in the entire expression.

            // only remove one power for now.
            int i = 0;
            for (; i < ttt->children.size(); i++) {
                if (ttt->children[i]->compare(a) == 0) {
                    break;
                }
            }
            assert(i < ttt->children.size());
            // FIXME delete without ruining setup; delete ttt->children[i];
            ttt->children.erase(ttt->children.begin() + i);
            // if there is one left, extract it!

            if (ttt->children.size() == 1) {
                sub.push_back(ttt->children[0]);
                ttt->children.clear();
                delete c;
            } else {
                sub.push_back(c);
            }
        }

    }

    assert(sub.size() > 1);

    // should sub be sorted? YES it should
    // it could be that the doGeneralisedHorner yields a MUL, merge!
    Node* genHornSub = doGeneralisedHorner(new Operation(sub, Operation::ADD));
    Operation* genHornSubOp = dynamic_cast<Operation*> (genHornSub);

    Node* mulTerm = genHornSub;
    if (genHornSubOp->op == Operation::MUL) {
        sortedInsert(genHornSubOp->children, a);
    } else {
        // sort for generic a?
        mulTerm = new Operation({a, genHornSub}, Operation::MUL);
    }

    root = mulTerm;
    //return root; // FIXME: for testing

    if (rest.size() > 0) {
        if (rest.size() == 1) {
            if (rest[0]->compare(mulTerm) < 0) {
                root = new Operation({rest[0], mulTerm}, Operation::ADD); // one term -> no Horner possible
            } else {
                root = new Operation({mulTerm, rest[0]}, Operation::ADD);
            }
        } else {
            Node* restHornered = doGeneralisedHorner(new Operation(rest, Operation::ADD));
            restHornered->print();
            std::cout << std::endl;

            // make sure there is no + stacking
            Operation* restOp = dynamic_cast<Operation*> (restHornered);
            assert(restOp != NULL);

            if (restOp->op == Operation::ADD) {
                sortedInsert(restOp->children, mulTerm);
                root = restHornered;
            } else {
                if (restHornered->compare(mulTerm) < 0) {
                    root = new Operation({restHornered, mulTerm}, Operation::ADD);
                } else {
                    root = new Operation({mulTerm, restHornered}, Operation::ADD);
                }
            }

        }
    }

    return root;
}

/**
 * Add and multiply constants if there are many, so 1+2=3 and 2*4=5
 * FIXME: what happens if the term only has constants? Then replace the sum!
 * @return 
 */
CSE::Node* CSE::doMaths(Node* tree) {
    Operation* tt = dynamic_cast<Operation*> (tree);

    if (tt == NULL) {
        return tree;
    }

    // collect constants
    int i = 0;
    while (dynamic_cast<Constant*> (tt->children[i]) != NULL) {
        i++;
    }

    if (i == tt->children.size()) {
        std::cout << "OP: " << tt->op << std::endl;
        tt->print();
        std::cout << std::endl;
    }

    assert(i < tt->children.size()); // this case is still unsupported

    if (i > 1) {
        int count = tt->op == Operation::MUL ? 1 : 0;

        for (int k = 0; k < i; k++) {
            Constant* c = dynamic_cast<Constant*> (tt->children[k]);
            if (tt->op == Operation::MUL) {
                count *= c->getVal();
            } else {
                count += c->getVal();
            }

            delete tt->children[k]; // free memory
        }

        Node* newConstant = new Constant(count);

        tt->children.erase(tt->children.begin(), tt->children.begin() + i - 1); // TODO: check if bounds are correct
        tt->children[0] = newConstant;
        std::cout << "Merged constants" << std::endl;
    }

    // now do the children
    for (int i = 0; i < tt->children.size(); i++) {
        tt->children[i] = doMaths(tt->children[i]);
    }

    return tree;
}

//  Add up same terms, so 2a+3a=5a
// Could also be done by Horner and doMaths? 2a+3a=(2+3)a = 5a

CSE::Node* CSE::addSameTerms(Node* tree) {
    Operation* tt = dynamic_cast<Operation*> (tree);

    if (tt == NULL) {
        return tree;
    }

    if (tt->op == Operation::ADD) {
        auto my_hash = [](Node * const& a) {
            return a->hash();
        };

        auto my_eq = [](Node * const& a, Node * const& b) {
            return a->compare(b) == 0;
        };

        std::vector<Node*> cses;

        std::unordered_map < Node*, int, decltype(my_hash), decltype(my_eq) > csemap(0, my_hash, my_eq);

        // TODO: do the following for each child of tt

        // collect constants
        int i = 0;
        int count = 0;
        while (dynamic_cast<Constant*> (tt->children[i]) != NULL) {
            count += dynamic_cast<Constant*> (tt->children[i])->getVal();
            i++;
        }

        bool same = true;


        if (i > 1) {
            int count = tt->op == Operation::MUL ? 1 : 0;

            for (int k = 0; k < i; k++) {
                Constant* c = dynamic_cast<Constant*> (tt->children[k]);
                if (tt->op == Operation::MUL) {
                    count *= c->getVal();
                } else {
                    count += c->getVal();
                }

                delete tt->children[k]; // free memory
            }

            Node* newConstant = new Constant(count);

            tt->children.erase(tt->children.begin(), tt->children.begin() + i - 1); // TODO: check if bounds are correct
            tt->children[0] = newConstant;
            std::cout << "Merged constants" << std::endl;
        }
    }

    // now do the children
    for (int i = 0; i < tt->children.size(); i++) {
        tt->children[i] = addSameTerms(tt->children[i]);
    }

    return tree;
}

// do a very quick CSE on a binary tree, works from bottom-up
// this will create lots of new variables
// does not modify original tree, but build new list with replaced parts
// TODO: filter +0 and *1
CSE::Node* CSE::doSimpleCSE(Node* tree,
        std::map < Node*, Node*, bool(*)(Node * const&, Node * const&) >& replaceMap,
        std::vector<Node*>& instrTree) {
    Operation* o = dynamic_cast<Operation*> (tree);

    if (o == NULL) {
        // not an operator, we don't want to add individual vars/constants
        return tree;
    }

    auto it = replaceMap.find(tree); // t from original tree
    if (it == replaceMap.end()) { // TODO: get correct order in instrTree
        assert(o->children.size() == 2);

        std::vector<Node*> children;
        for (int i = 0; i < o->children.size(); i++) { // first go to children
            children.push_back(doSimpleCSE(o->children[i], replaceMap, instrTree));
        }

        // then build actual structure, bottom-up
        // add at pos newvar in replace tree: replaced expression
        instrTree.push_back(new Operation(children, o->op));
        Node* newVar = new Variable(100 + instrTree.size() - 1);
        replaceMap[tree] = newVar; // make label here

        return newVar;
    }

    return it->second;
}

// TODO: sort beforehand?
// TODO: take -100*x and 100x into account as same term?
// only works on binary trees!
// goes top-down

unsigned int CSE::countSimpleCSE(Node* tree) {
    // use sorted map, hashset takes too long since it has to traverse the entire tree
    auto my_cmp = [](Node * const& a, Node * const& b) {
        return a->compare(b) < 0;
    };

    std::set < Node*, decltype(my_cmp) > replaceMap(my_cmp);

    int newOpCount = 0;

    std::vector<Node*> stack;
    stack.push_back(tree);

    while (!stack.empty()) {
        Node* t = stack.back();
        stack.pop_back();

        Operation* o = dynamic_cast<Operation*> (t);

        if (o == NULL) {
            continue; // not an operator, skip
        }

        assert(o->children.size() == 2); // only works on binary trees

        if (replaceMap.find(t) == replaceMap.end()) {
            replaceMap.insert(t);

            // check for *1 or *-1, this is not an operation
            Constant* c = dynamic_cast<Constant*> (o->children[0]);
            if (c == NULL || (c->getVal() != 1 && c->getVal() != -1)) {
                newOpCount++;
            }

            for (int i = 0; i < o->children.size(); i++) {
                stack.push_back(o->children[i]);
            }
        }
    }

    return newOpCount;
}

// CSE can do:
// 2xy + 3xy => 2T + 3T
// cannot do:
// 2 + 3 + 2x + 3x => T + Tx
// So: first Horner, then CSE, then Horner again
// does not handle distributivity of multiplication
// TODO: memory management
// Subsets scales like 2^n, where n is the number of children. Therefore
// Horner has to be done before CSE, or else the number of + subsets in the root is too high
// TODO: store hashes of the children so they need not be recomputed every time

CSE::Node* CSE::doCSE(Node* tree) {
    Node* newtree = buildComAssocTree(tree);

    //newtree = doGeneralisedHorner(newtree);
    std::cout << "Result: " << std::endl;
    //newtree->print();
    std::cout << std::endl;
    std::cout << "Num ops: " << countOperators(newtree) << std::endl;

    auto my_hash = [](Node * const& a) {
        return a->hash();
    };

    auto my_eq = [](Node * const& a, Node * const& b) {
        return a->compare(b) == 0;
    };

    std::vector<Node*> cses;

    // hashmap with custom hash and equals function that counts number of times a CSE is found
    std::unordered_map < Node*, int, decltype(my_hash), decltype(my_eq) > csemap(0, my_hash, my_eq);
    std::vector<Node*> stack;
    stack.push_back(newtree);

    while (!stack.empty()) {
        Node* t = stack.back();
        stack.pop_back();

        Operation* o = dynamic_cast<Operation*> (t);

        if (o == NULL) {
            continue; // not an operator, we don't want to add individual vars/constants
        }

        // TODO: merge with code below?
        auto f = csemap.find(t);
        if (f != csemap.end()) {
            /*std::cout << "Collision found: ";
            t->print();
            std::cout << std::endl;*/
            f->second++; // increase count

            continue; // do not visit children, already done
        } else {
            csemap[t] = 1;
        }

        // keep track of which children are visited, and skip them later on. This is because
        // they will be replaced completely by the found, bigger structure.
        std::unordered_set < Node*, decltype(my_hash), decltype(my_eq) > visited(0, my_hash, my_eq);

        // generate subsets bigger than 1 and smaller than total
        for (int k = 2; k < o->children.size(); k++) {
            std::vector<Node*> temp(k);
            std::vector < std::vector < Node* >> subsets; // TODO: allocate (n ncr k) units?
            genSubsets(o->children, k, temp, subsets);

            std::unordered_set < Node*, decltype(my_hash), decltype(my_eq) > localset(0, my_hash, my_eq);
            for (auto s : subsets) {
                //create new node. Ordered because subsets are ordered
                Node* newNode = new Operation(s, o->op);

                auto r = localset.find(newNode);
                if (r != localset.end()) {
                    // TODO: check if this is correct, if expression is x*x, where x is a structure
                    // only visited one of them. The other should be skipped, but still be counted as
                    // an occurrence of x. This last bit does not happen?
                    for (Node* ch : s) {
                        visited.insert(ch);
                    }

                    continue; // skip it, local match, FIXME: this also skips x^4, which can be simplified!!!
                }
                localset.insert(newNode);

                auto f = csemap.find(newNode);
                if (f != csemap.end()) {
                    for (Node* ch : s) {
                        visited.insert(ch);
                    }

                    /*std::cout << "Collision found: ";
                    newNode->print();
                    std::cout << std::endl;*/
                    f->second++;
                } else {
                    csemap[newNode] = 1;
                }
            }
        }

        if (o->op == Operation::MUL) { // take care of powers of random objects
            int count = 0;
            for (int i = 1; i < o->children.size(); i++) {
                // sorted, so equiv should be side by side
                if (o->children[i]->compare(o->children[i - 1]) == 0) {
                    count++;
                } else {
                    std::vector<Node*> reps = {o->children[i - 1]};
                    for (int j = 0; j < count; j++) {
                        reps.push_back(o->children[i - 1]);

                        // TODO: overcounting by 1 because this already is counted above?
                        csemap[new Operation(reps, Operation::MUL)]++;
                    }

                    count = 0;
                }
            }
        }

        for (Node*& a : o->children) {
            if (visited.find(a) == visited.end()) {
                stack.push_back(a); // push children
            }
        }
    }

    for (auto cse : csemap) {
        if (cse.second > 1) {
            cses.push_back(cse.first);
        }
    }


    // sort by size and frequency
//    std::sort(cses.begin(), cses.end(), [&](Node* a, Node* b) 
//    {
//        if (countOperators(a) != countOperators(b)) {
//            return countOperators(a) < countOperators(b);
//        }
//
//        return csemap[a] < csemap[b];
//    });

    /* for (Node* cse : cses) {
         std::cout << "CSE: ";
         cse->print();
         std::cout << " : " << csemap[cse] << std::endl;
     }*/


    // TODO: also delete memory of subsets
    csemap.clear(); // not needed anymore


    // make deep copies of the structure to prevent problems later on
    for (int i = 0; i < cses.size(); i++) {
        cses[i] = cses[i]->clone();
    }

    // first replace the CSEs in the tree and then find out of the CSEs
    // themselves contain other smaller CSEs
    newtree = replaceCSEsInTree(newtree, cses);

    int cseOpCount = 0; // count all CSE operators.

    for (Node* cse : cses) {
        cseOpCount += countOperators(cse);
    }

    std::cout << "Number of operators in new tree: " << countOperators(newtree) << std::endl;
    std::cout << "Number of operators in CSE tree: " << cseOpCount << std::endl;
    std::cout << "Number of total operators      : " << countOperators(newtree) + cseOpCount << std::endl;

    newtree = doMaths(doGeneralisedHorner(newtree));
    std::cout << "Number of total operators after Horner : " << countOperators(newtree) + cseOpCount << std::endl;

    return newtree;
}


