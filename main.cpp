/* 
 * File:   main.cpp
 * Author: SAM
 *
 * Created on July 8, 2014, 5:19 PM
 */

#include <cstdlib>
#include "GameTree.h"
#include "Alphabeta.h"
#include "UCT.h"
#include "PGameState.h"
#include "HexState.h"
#include <sys/time.h>
#include <thread>
using namespace std;

/*
 * 
 */
template <typename T>
void UCTPlayPGame(T &state,const PlyOptions optplya, const PlyOptions optplyb, int ngames, int seed,int verbose) {

    UCT<T> plya(optplya,verbose);
    UCT<T> plyb(optplyb,verbose);
    vector<int> plywin(3);
    vector<int> plywina(3);
    vector<int> plywinb(3);
    int move, i = 0, ply=0;
    string str;
    char buffer[100];

    while (i < ngames) {
        if(verbose){
        if (i % 2 == 0) {
            //sprintf(buffer, "#game no. %d plya:Black plyb:White ", i);
            cout << "\n#start playing,\n";
            cout << setw(9) << "#game no." << "," << setw(10) << "plya(1)" << "," << setw(10) << "plyb(2)" << "," << endl;
            cout << setw(9) << i << "," << setw(10) << "Black(B)" << "," << setw(10) << "White(W)" << "," << endl;
        } else {
            //sprintf(buffer, "#game no. %d plya:White plyb:Black ", i);
            cout << "\n#start playing,\n";
            cout << setw(9) << "#game no." << "," << setw(10) << "plya(1)" << "," << setw(10) << "plyb(2)" << "," << endl;
            cout << setw(9) << i << "," << setw(10) << "White(W)" << "," << setw(10) << "Black(B)" << "," << endl;
        }
        cout << setw(9) << "#move no." << "," << setw(10) << "player" << "," << setw(10) << "games/sec" << "," <<
                setw(10) << "time" << "," << setw(10) << "select(%)" << "," << setw(10) << "expand(%)" << "," <<
                setw(10) << "playout(%)" << "," << setw(10) << "backup(%)" << "," << endl;
        }
        //str.append(buffer);
        int j = 0;
        while (!state.GameOver()) {
            if (verbose) {
                //cout << buffer << endl;
                cout << setw(9) << j << ",";
                
            }
            if (i % 2 == 0) {
                if (state.PlyJustMoved() == 1) {
                    ply = 2;
                    if(verbose)
                        cout << setw(10) << ply << ",";
                    plyb.Run(state, move);
                } else {
                    
                    //cin>>move;
                    ply=1;
                    if(verbose)
                        cout<<setw(10)<<ply<<",";
                    plya.Run(state,move);
                }
            } else {
                if (state.PlyJustMoved() == 1) {
                    ply=1;
                    if(verbose)
                        cout<<setw(10)<<ply<<",";
                    plya.Run(state,move);
                } else {
                    ply=2;
                    if(verbose)
                        cout<<setw(10)<<ply<<",";
                    plyb.Run(state,move);
                }
            }
            
            state.DoMove(move);
            if (verbose) {
                const char *c = (state.PlyJustMoved() == 1 ? "Black " : "White ");
                //plya.PrintTree();
                str.append(buffer);
                //cout << c << " moved to " << move << endl; // << " with Index " << state.currIdx << endl;
                //state.Print();
//                float w=state.EvaluateBoardDSet(BLACK,TOPDOWN);
//                cout<<w<<endl;
//                w=state.EvaluateBoardDSet(WHITE,LEFTRIGHT);
//                cout<<w<<endl;
            }
            j++;
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
            sprintf(buffer, "+draw");
            str.append(buffer);
        }
        str.append("\n");
        if (verbose) {
            cout << "\n#end playing,\n";
            cout << buffer << endl;
        }
        state.NewGame();
        i++;
    }
    cout << "\n#results,\n";
    cout << setw(6) << "#player" << "," << setw(10) << "wins(%)" << "," << setw(10) << "wins" << "," << setw(10) << "Black" << "," << setw(10) << "White" << endl;
    cout << setw(6) << 1 << "," << setw(10) <<((plywina[1] + plywina[2]) / (float) ngames)*100  << "," << setw(10) << (plywina[1] + plywina[2])
            << "," << setw(10) << plywina[1] << "," << setw(10) << plywina[2] << "," << endl;
    cout << setw(6) << 2 << "," << setw(10) <<((plywinb[1] + plywinb[2]) / (float) ngames)*100  << "," << setw(10) << (plywinb[1] + plywinb[2])
            << "," << setw(10) << plywinb[1] << "," << setw(10) << plywinb[2] << "," << endl;
    cout << setw(6) << " " << "," << setw(10) << " " << "," << setw(10) << " " << "," << setw(10) << plywin[1] << "," << setw(10) << plywin[2] << "," << endl;
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

    int b = 4, d = 6, seed = -1,ngames = 1, vflag = 0;
    char* game;
    PlyOptions optplya,optplyb;
    
    int opt;

    timeval time;
    gettimeofday(&time, NULL);
    seed = time.tv_sec + time.tv_usec;

    char* gflag = "x";
    int hflag = 0;
    while ((opt = getopt(argc, argv, "hpg:b:d:o:t:m:q:y:w:x:z:n:vs:")) != -1) {
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
                optplya.nsims = atoi(optarg);
                break;
            case 't':
                optplyb.nsims = atoi(optarg);
                break;
            case 'm':
                optplya.nthreads = atoi(optarg);
                break;
            case 'q':
                optplyb.nthreads = atoi(optarg);
                break;
            case 'y':
                if(0<atoi(optarg)<3)
                    optplya.par = atoi(optarg);
                break;
            case 'w':
                if(0<atoi(optarg)<3)
                    optplyb.par = atoi(optarg);
                break;
                case 'x':
                if(0<atoi(optarg))
                    optplya.nsecs = atoi(optarg);
                break;
                case 'z':
                if(0<atoi(optarg))
                    optplyb.nsecs = atoi(optarg);
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
                << "\t-o\t\tMax number of simulations(default=5000) for the player a\n"
                << "\t-t\t\tMax number of simulations(default=5000) for the player b\n"
                << "\t-n\t\tNumber of games in the tournament\n"
                << "\t-m\t\tNumber of threads(default=1) for the player a\n"
                << "\t-q\t\tNumber of threads(default=1) for the player b\n"
                << "\t-y\t\tParallel method(default=0 tree=1,root=2) for the player a\n"
                << "\t-w\t\tParallel method(default=0 tree=1,root=2) for the player b\n"
                << "\t-x\t\tNumber of seconds(default=0) for the player a\n"
                << "\t-z\t\tNumber of seconds(default=0) for the player b\n"
                << "\t-v\t\tShow the output on screen\n"
                << "\t-s\t\tSeed to be used by random number generator\n"
                << endl;
        exit(1);
    }
    if (gflag == "x") {
        game="Hex";
    }else if(gflag=="p"){
        game="p-game";
    }
    if(gflag=="x"){
        printf("#plya game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, seed=%d\n",
                game, d, optplya.nsims,optplya.nthreads, optplya.nsecs, ngames, seed);
        printf("#plyb game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, seed=%d\n",
                game, d, optplyb.nsims,optplyb.nthreads, optplyb.nsecs, ngames, seed);
    } else if (gflag == "p") {
        printf("#plya game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, seed=%d\n",
                game,b, d, optplya.nsims,optplya.nthreads, optplya.nsecs, ngames, seed);
        printf("#plyb game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, seed=%d\n",
                game,b, d, optplyb.nsims,optplyb.nthreads, optplyb.nsecs, ngames, seed);
    }

    for (int index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);

    srand(seed);

    if (gflag == "x") {
        HexGameState state(d);
        UCTPlayPGame<HexGameState>(state, optplya,optplyb, ngames, seed, vflag);
    } else if (gflag == "p") {
        PGameState state(b, d, 0x80, seed);
        UCTPlayPGame<PGameState>(state,optplya,optplyb, ngames, seed, vflag);
    }

    //NegamaxPlayGame(state,b,d,1,1,seed,false);

    return 0;
}
