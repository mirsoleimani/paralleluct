/* 
 * File:   main.cpp
 * Author: SAM
 *
 * Created on July 8, 2014, 5:19 PM
 */

#include <cstdlib>
#include "GameTree.h"
#include "Alphabeta.h"
#include "MCTS.h"
#include "UCT.h"
#include "PGameState.h"
#include "HexState.h"
#include <sys/time.h>
using namespace std;

/*
 * 
 */
template <typename T>
void UCTPlayPGame(T &state, int nsima, int nsimb, int ntry, int seed, int verbose) {

    UCT<T> plya;
    UCT<T> plyb;
    vector<int> plywin(3);
    vector<int> plywina(3);
    vector<int> plywinb(3);
    int v = 0;
    int move, i = 0;
    string str;
    char buffer[100];

    while (i < ntry) {
        if (i % 2 == 0) {
            sprintf(buffer, "#game no. %d plya:Black plyb:White ", i);
        } else {
            sprintf(buffer, "#game no. %d plya:White plyb:Black ", i);
        }
        str.append(buffer);
        while (!state.GameOver()) {
            if (verbose)
                cout << buffer << endl;
            if (i % 2 == 0) {
                if (state.PlyJustMoved() == 1) {
                    move = plyb.UCTSearch(state, nsimb, verbose);
                } else {
                    //cout<<"please enter your move\n";
                    //cin>>move;
                    move = plya.UCTSearch(state, nsima, verbose);
                }
            } else {
                if (state.PlyJustMoved() == 1) {
                    move = plya.UCTSearch(state, nsima, verbose);
                } else {
                    //cout<<"please enter your move\n";
                    //cin>>move;
                    move = plyb.UCTSearch(state, nsimb, verbose);
                }
            }

            state.DoMove(move);
            if (verbose) {
                const char *c = (state.PlyJustMoved() == 1 ? "Black " : "White ");
                cout << c << " moved to " << move << endl; // << " with Index " << state.currIdx << endl;
                state.Print();
            }
        }

        if (state.GetResult(state.PlyJustMoved()) == 1.0) {
            const char *c = state.PlyJustMoved() == 1 ? "+B " : "+W ";
            sprintf(buffer, c);
            str.append(buffer);

            if (i % 2 == 0) {
                state.PlyJustMoved() == 1 ? plywina[state.PlyJustMoved()]++ : plywinb[state.PlyJustMoved()]++;
            } else {
                state.PlyJustMoved() == 1 ? plywinb[state.PlyJustMoved()]++ : plywina[state.PlyJustMoved()]++;
            }
            plywin[state.PlyJustMoved()]++;
        } else if (state.GetResult(state.PlyJustMoved()) == 0.0) {
            const char *c = (3 - state.PlyJustMoved() == 1 ? "+B " : "+W ");
            sprintf(buffer, c);
            str.append(buffer);

            if (i % 2 == 0) {
                (3 - state.PlyJustMoved() == 1) ? plywina[3 - state.PlyJustMoved()]++ : plywinb[3 - state.PlyJustMoved()]++;
            } else {
                (3 - state.PlyJustMoved() == 1) ? plywinb[3 - state.PlyJustMoved()]++ : plywina[3 - state.PlyJustMoved()]++;
            }
            plywin[3 - state.PlyJustMoved()]++;
        } else {
            sprintf(buffer, "It is a draw! ");
            str.append(buffer);
        }
        str.append("\n");
        if (verbose) {
            cout << buffer << endl;
        }
        state.NewGame();
        i++;
    }
    cout << setw(10) << "Player" << "," << setw(10) << "Wins(%)" << "," << setw(10) << "Wins" << "," << setw(10) << "Black" << "," << setw(10) << "White" << endl;
    cout << setw(10) << "plya" << "," << setw(10) <<((plywina[1] + plywina[2]) / (float) ntry)*100  << "," << setw(10) << (plywina[1] + plywina[2])
            << "," << setw(10) << plywina[1] << "," << setw(10) << plywina[2] << "," << endl;
    cout << setw(10) << "plyb" << "," << setw(10) <<((plywinb[1] + plywinb[2]) / (float) ntry)*100  << "," << setw(10) << (plywinb[1] + plywinb[2])
            << "," << setw(10) << plywinb[1] << "," << setw(10) << plywinb[2] << "," << endl;
    cout << setw(10) << " " << "," << setw(10) << " " << "," << setw(10) << " " << "," << setw(10) << plywin[1] << "," << setw(10) << plywin[2] << "," << endl;
    //cout << str << endl;
}

void NegamaxPlayGame(PGameState &s, int b, int d, int itr, int ntry, int seed, int verbose) {
    PGameState state(s);

    Alphabeta pl1;
    Alphabeta pl2;
    int i = 0, pl1wins = 0, pl2wins = 0;
    cout << "\nAB Play:\n";
    while (i < ntry) {
        while (!state.GameOver()) {
            int move = -1, bestMove = -1;
            if (state.PlyJustMoved() == 1) {
                move = pl2.ABNegamax2(state, -INF, +INF, -1, bestMove, verbose);
            } else {
                move = pl1.ABNegamax2(state, -INF, +INF, 1, bestMove, verbose);
            }

            state.DoMove(bestMove);
            cout << "\nPlayer " << state.PlyJustMoved() << " moved to child " << bestMove << " with index " << state.currIdx << endl;
            //state.Print();
        }

        if (state.GetResult(state.PlyJustMoved()) == 1.0) {
            cout << "Player " << state.PlyJustMoved() << " Wins!" << endl;
            if (state.PlyJustMoved() == 1) {
                pl1wins++;
            } else {
                pl2wins++;
            }
        } else if (state.GetResult(state.PlyJustMoved()) == 0.0) {
            cout << "Player " << 3 - state.PlyJustMoved() << " Wins!" << endl;
            if ((3 - state.PlyJustMoved()) == 1) {
                pl1wins++;
            } else {
                pl2wins++;
            }
        } else {
            cout << "It is a draw!" << endl;
        }
        i++;
    }
    //cout<<"player 1 wins:"<<pl1wins<<" Player 2 wins:"<<pl2wins<<endl;
}

int main(int argc, char** argv) {

    int b = 4, d = 6, seed = -1, nsima = 5000, nsimb = 5000, ngames = 1, vflag = 0;
    int opt;

    timeval time;
    gettimeofday(&time, NULL);
    seed = time.tv_sec + time.tv_usec;

    char* gflag = "x";
    int hflag = 0;
    while ((opt = getopt(argc, argv, "hpg:b:d:o:t:n:vs:")) != -1) {
        switch (opt) {
            case 'h':
                hflag = 1;
                break;
            case 'p':
                gflag = "p";
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'd':
                d = atoi(optarg);
                break;
            case 'o':
                nsima = atoi(optarg);
                break;
            case 't':
                nsimb = atoi(optarg);
                break;
            case 'n':
                ngames = atoi(optarg);
                break;
            case 'v':
                vflag = 1;
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case '?':
                if (optopt == 'g' || optopt == 'b' || optopt == 'd' || optopt == 'n' || optopt == 's')
                    fprintf(stderr, "Option -%opt requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%opt'.\n", optopt);
                else
                    fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                return 1;
            default:
                abort();

        }
    }
    if (hflag) {
        cerr << "Usage: " << argv[0] << " <option(s)> \n"
                << "Options:\n"
                << "\t-x\t\tHex\n"
                << "\t-p\t\tP-Game(p)\n"
                << "\t-b\t\tBreath of the tree\n"
                << "\t-d\t\tDepth of the tree or dimension of the board\n"
                << "\t-o\t\tNumber of simulations for the player a\n"
                << "\t-t\t\tNumber of simulations for the player b\n"
                << "\t-n\t\tNumber of games in the tournament\n"
                << "\t-v\t\tShow the output on screen\n"
                << "\t-s\t\tSeed to be used by random number generator\n"
                << endl;
        exit(1);
    }
    if (gflag == "x") {
        printf("game = %s, dim = %d, nsim plya = %d,nsim plyb=%d, ngames=%d,seed=%d\n",
                "Hex", d, nsima, nsimb, ngames, seed);
    } else if (gflag == "p") {
        printf("game = %s,breath=%d, depth = %d, nsim plya = %d,nsim plyb=%d, ngames=%d,seed=%d\n",
                "p-game", b, d, nsima, nsimb, ngames, seed);
    }

    for (int index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);

    srand(seed);

    cout << "\n#Start Playing:\n";

    if (gflag == "x") {
        HexGameState state(d);
        UCTPlayPGame<HexGameState>(state, nsima, nsimb, ngames, seed, vflag);
    } else if (gflag == "p") {
        PGameState state(b, d, 0x80, seed);
        UCTPlayPGame<PGameState>(state, nsima, nsimb, ngames, seed, vflag);
    }

    //NegamaxPlayGame(state,b,d,1,1,seed,false);

    return 0;
}
