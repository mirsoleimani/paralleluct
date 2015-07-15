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
#include <thread>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
//#include <boost/nondet_random.hpp>
//#include <boost/random/uniform_int.hpp>
using namespace std;

/*
 * 
 */
template <typename T>
void UCTPlayPGame(T &rstate, PlyOptions optplya, PlyOptions optplyb, int ngames, int nmoves, int swap, int verbose) {
     cilkview_data_t d;
#ifdef THREADPOOL
    boost::threadpool::pool thread_pool(NTHREADS);
#endif
    vector<int> plywin(3);
    vector<int> plywina(3);
    vector<int> plywinb(3);
    int move = 0, i = 0;
    string ply = "none";
    string black;
    string white;
    std::stringstream strVisit;
    char buffer[100];
    std::random_device dev;
    vector<unsigned int> seeda(optplya.nthreads);
    vector<unsigned int> seedb(optplyb.nthreads);
    

    //Timer tmr;
//    for (int j = 0; j < optplya.nthreads; j++) {
////        seeda[j] = (unsigned int) dev();
//        seeda[j]=1;
//        assert(seeda[j] > 0 && "seed can not be negative\n");
//        if (verbose)
//         printf("#plya thread %d, seed=%u\n", j, seeda[j]);
//    }
//    for (int j = 0; j < optplyb.nthreads; j++) {
////        seedb[j] = (unsigned int) dev();
//        seedb[j]=1;
//        assert(seedb[j] > 0 && "seed can not be negative\n");
//        if (verbose)
//          printf("#plyb thread %d, seed=%u\n", j, seedb[j]);
//    }
//    
//    UCT<T> plya(optplya, verbose,seeda);
//    UCT<T> plyb(optplyb, verbose,seedb);
        
    while (i < ngames) {
        T state(rstate);

        if (swap) {
            if (i % 2 == 0) {
                black = "plya(B)";
                white = "plyb(W)";
            } else {
                black = "plyb(B)";
                white = "plya(W)";
            }
        } else {
            black = "plya(B)";
            white = "plyb(W)";
        }
        //
        for (int j = 0; j < optplya.nthreads; j++) {
            seeda[j] = (unsigned int) dev();
            assert(seeda[j] > 0 && "seed can not be negative\n");
            //            if (verbose)
            //             printf("#plya thread %d, seed=%u\n", j, seeda[j]);
        }
        for (int j = 0; j < optplyb.nthreads; j++) {
            seedb[j] = (unsigned int) dev();
            assert(seedb[j] > 0 && "seed can not be negative\n");
            //            if (verbose)
            //              printf("#plyb thread %d, seed=%u\n", j, seedb[j]);
        }
        cout << "# plya seed = " << seeda[0] << endl;
        UCT<T> plya(optplya, verbose, seeda);
        cout << "# plyb seed = " << seedb[0] << endl;
        UCT<T> plyb(optplyb, verbose, seedb);

        if (verbose) {
            cout << "# start game" << ","
                    << setw(8) << i << ","
                    << setw(10) << black << ","
                    << setw(10) << white << ","
                    << setw(10) << "?" << endl;

            cout << setw(9) << "# move no." << ","
                    << setw(10) << "player" << ","
                    << setw(10) << "playout" << ","
                    << setw(10) << "time" << ","
                    << setw(10) << "select(%)" << ","
                    << setw(10) << "expand(%)" << ","
                    << setw(10) << "playout(%)" << ","
                    << setw(10) << "backup(%)" << ","
                    << setw(10) << "nrandvec" << ","
                    << setw(10) << "move" << endl;
        }
        if (verbose == 3) {
            strVisit.str().clear();
            strVisit.str(std::string());
            strVisit << "# start visit,"
                    << setw(9) << i << ","
                    << setw(10) << black << ","
                    << setw(10) << white << endl;
        }

        int j = 0;
        while (!state.GameOver()&& j < nmoves) {
            string log = " ";
            string log2 = " ";

            if (verbose) {
                cout << setw(9) << j << ",";
            }
            if (verbose == 3) {
                vector<MOVE> moves;
                state.GetMoves(moves);

                strVisit << setw(9) << "# move no." << ","
                        << setw(10) << "player" << ",";
                for (int i = 0; i < moves.size(); i++)
                    strVisit << moves[i] << ",";
                strVisit << endl;
            }
            if (state.PlyJustMoved() == BLACK) {
                ply = white;
                if (white == "plya(W)") {
                    //cin>>move;
                    if (optplya.threadruntime == 0)
                        plya.Run(state, move, log, log2);
#ifdef THREADPOOL
                    else if (optplya.threadruntime == 1)
                        plya.RunThreadPool(state, move, log, log2, thread_pool);
#endif
                    else if (optplya.threadruntime == 2)
                        plya.RunCilk(state, move, log, log2);
                    else if (optplya.threadruntime == 3)
                        plya.RunTBB(state, move, log, log2);
                    else if (optplya.threadruntime == 4)
                        plya.RunCilkFor(state, move, log, log2);
                } else {
                    if (optplyb.threadruntime == 0)
                        plyb.Run(state, move, log, log2);
#ifdef THREADPOOL
                    else if (optplyb.threadruntime == 1)
                        plyb.RunThreadPool(state, move, log, log2, thread_pool);
#endif
                    else if (optplyb.threadruntime == 2){
                        __cilkview_query(d);
                        plya.RunCilk(state, move, log, log2);
                        __cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                    }else if (optplyb.threadruntime == 3)
                        plyb.RunTBB(state, move, log, log2);
                    else if (optplyb.threadruntime == 4)
                        plyb.RunCilkFor(state, move, log, log2);

                }
            } else {
                ply = black;
                if (black == "plya(B)") {
                    //cin>>move;
                    if (optplya.threadruntime == 0)
                        plya.Run(state, move, log, log2);
#ifdef THREADPOOL
                    else if (optplya.threadruntime == 1)
                        plya.RunThreadPool(state, move, log, log2, thread_pool);
#endif
                    else if (optplya.threadruntime == 2){
                        __cilkview_query(d);
                        plya.RunCilk(state, move, log, log2);
                        __cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                    }else if (optplya.threadruntime == 3)
                        plya.RunTBB(state, move, log, log2);
                    else if (optplya.threadruntime == 4)
                        plya.RunCilkFor(state, move, log, log2);
                } else {
                    
                    if (optplyb.threadruntime == 0)
                        plyb.Run(state, move, log, log2);
#ifdef THREADPOOL
                    else if (optplyb.threadruntime == 1)
                        plyb.RunThreadPool(state, move, log, log2, thread_pool);
#endif
                    else if (optplyb.threadruntime == 2)
                        plyb.RunCilk(state, move, log, log2);
                    else if (optplyb.threadruntime == 3)
                        plyb.RunTBB(state, move, log, log2);
                    else if (optplyb.threadruntime == 4)
                        plyb.RunCilkFor(state, move, log, log2);

                }

            }

            state.DoMove(move);

            if (verbose) {
                cout << setw(10) << ply << ",";
                cout << log;
                cout << setw(10) << move << endl;
            }
            if (verbose == 2)
                state.Print();
            if (verbose == 3) {
                strVisit << setw(9) << j << ","
                        << setw(10) << ply << ","
                        << log2 << endl;
            }
            j++;
        }

        float r = state.GetResult(state.PlyJustMoved());
        if (r == WIN) {
            const char *c = state.PlyJustMoved() == BLACK ? "+B " : "+W ";
            sprintf(buffer, c);

            if (i % 2 == 0) {
                state.PlyJustMoved() == BLACK ? plywina[state.PlyJustMoved()]++ : plywinb[state.PlyJustMoved()]++;
            } else {
                state.PlyJustMoved() == BLACK ? plywinb[state.PlyJustMoved()]++ : plywina[state.PlyJustMoved()]++;
            }
            plywin[state.PlyJustMoved()]++;
        } else if (r == LOST) {
            const char *c = (CLEAR - state.PlyJustMoved() == BLACK ? "+B " : "+W ");
            sprintf(buffer, c);

            if (i % 2 == 0) {
                (CLEAR - state.PlyJustMoved() == BLACK) ? plywina[CLEAR - state.PlyJustMoved()]++ : plywinb[CLEAR - state.PlyJustMoved()]++;
            } else {
                (CLEAR - state.PlyJustMoved() == BLACK) ? plywinb[CLEAR - state.PlyJustMoved()]++ : plywina[CLEAR - state.PlyJustMoved()]++;
            }
            plywin[CLEAR - state.PlyJustMoved()]++;
        } else if (r == DRAW) {
            sprintf(buffer, "+Draw");
        }
        if (verbose) {
            cout << "# end game" << ","
                    << setw(10) << i << ","
                    << setw(10) << black << ","
                    << setw(10) << white << ","
                    << setw(10) << buffer << endl;
        }
        if (verbose == 3) {
            cout << strVisit.str()
                    << "# end visit,"
                    << setw(10) << i << ","
                    << setw(10) << black << ","
                    << setw(10) << white << endl;
        }
        state.NewGame();
        i++;
    }
    
    cout << "# results,\n";
    cout << setw(6) << "# player" << ","
            << setw(10) << "wins(%)" << ","
            << setw(10) << "wins" << ","
            << setw(10) << "Black" << ","
            << setw(10) << "White" << endl;
    cout << setw(6) << "plya" << ","
            << setw(10) << ((plywina[1] + plywina[2]) / (float) ngames)*100 << ","
            << setw(10) << (plywina[1] + plywina[2]) << ","
            << setw(10) << plywina[1] << ","
            << setw(10) << plywina[2] << "," << endl;
    cout << setw(6) << "plyb" << ","
            << setw(10) << ((plywinb[1] + plywinb[2]) / (float) ngames)*100 << ","
            << setw(10) << (plywinb[1] + plywinb[2]) << ","
            << setw(10) << plywinb[1] << ","
            << setw(10) << plywinb[2] << "," << endl;
    cout << setw(6) << " " << ","
            << setw(10) << " " << ","
            << setw(10) << " " << ","
            << setw(10) << plywin[1] << ","
            << setw(10) << plywin[2] << "," << endl;
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

    int b = 4, d = 6, seed = -1, ngames = 1, vflag = 0,nmoves=99999, swap=1;
    char* game;
    char* par_a;
    char* par_b;
    PlyOptions optplya, optplyb;

    int opt;


    char* gflag = "x";
    int hflag = 0;
    while ((opt = getopt(argc, argv, "hpg:b:d:o:t:m:q:y:w:x:z:n:v:s:e:f:r:a:c:")) != -1) {
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
                if (0 < atoi(optarg) < 3)
                    optplya.par = atoi(optarg);
                break;
            case 'w':
                if (0 < atoi(optarg) < 3)
                    optplyb.par = atoi(optarg);
                break;
            case 'x':
                if (0 < atoi(optarg))
                    optplya.nsecs = atoi(optarg);
                break;
            case 'z':
                if (0 < atoi(optarg))
                    optplyb.nsecs = atoi(optarg);
                break;
            case 'n':
                ngames = atoi(optarg);
                break;
            case 'v':
                vflag = atoi(optarg);
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'e':
                optplya.cp = atof(optarg);
                break;
            case 'f':
                optplyb.cp = atof(optarg);
                break;
            case 'r':
                optplya.threadruntime=atoi(optarg);
                optplyb.threadruntime=atoi(optarg);
                break;
            case 'a':
                nmoves=atoi(optarg);
                break;
            case 'c':
                swap=atoi(optarg);
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
                << "\t-x\t\tNumber of seconds(default=1) for the player a\n"
                << "\t-z\t\tNumber of seconds(default=1) for the player b\n"
                << "\t-e\t\tThe value of cp(default=0) for the player a\n"
                << "\t-f\t\tThe value of cp(default=0) for the player b\n"
                << "\t-v\t\tShow the output on screen(default=0)\n"
                << "\t-s\t\tSeed to be used by random number generator\n"
                << "\t-r\t\tThreading runtime for both player (default=2 none=0, boost thread pool=1, cilkplus=2, TBB=3)\n"
                << "\t-a\t\tNumber of moves in a game\n"
                << "\t-c\t\tTurn off swap rule(set to 0)\n"
                << endl;
        exit(1);
    }
    if (gflag == "x") {
        game = "Hex";
    } else if (gflag == "p") {
        game = "p-game";
    }
    if (optplya.par == 1) {
        par_a = "tree";
    } else if (optplya.par == 2) {
        par_a = "root";
    } else {
        par_a = "seq";
    }
    if (optplyb.par == 1) {
        par_b = "tree";
    } else if (optplyb.par == 2) {
        par_b = "root";
    } else {
        par_b = "seq";
    }
    if(optplya.par==0){
        optplya.nthreads=1;
    }
    if(optplyb.par==0){
        optplyb.nthreads=1;
    }
    if(nmoves == 0 ){
        cout<<"Can not run a game with zero moves\n";
        exit(0);
    }
    
    if (gflag == "x") {
        printf("# plya game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s, cp=%0.3f\n",
                game, d, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a, optplya.cp);
        printf("# plyb game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s, cp=%0.3f\n",
                game, d, optplyb.nsims, optplyb.nthreads, optplyb.nsecs, ngames, par_b, optplyb.cp);
    } else if (gflag == "p") {
        printf("# plya game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s\n",
                game, b, d, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a);
        printf("# plyb game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s\n",
                game, b, d, optplyb.nsims, optplyb.nthreads, optplyb.nsecs, ngames, par_b);
    }

    for (int index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);
    
//        timeval time;
//        gettimeofday(&time, NULL);
//        seed = time.tv_sec + time.tv_usec;
    //    srand(seed);
//    ENG engine(seed);
//    DIST dist(0,1);
//    GEN gen(engine, dist);
//    cout<<"***"<<rand()<<endl;
    
    if (gflag == "x") {
        HexGameState state(d);
        UCTPlayPGame<HexGameState>(state, optplya, optplyb, ngames,nmoves,swap, vflag);
    } else if (gflag == "p") {
        PGameState state(b, d, 0x80, seed);
        UCTPlayPGame<PGameState>(state, optplya, optplyb, ngames,nmoves,swap, vflag);
    }

    //NegamaxPlayGame(state,b,d,1,1,seed,false);

    return 0;
}
