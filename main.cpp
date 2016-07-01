/* 
 * File:   main.cpp
 * Author: SAM
 *
 * Created on July 8, 2014, 5:19 PM
 */

#include <cstdlib>
#include "UCT.h"
#include "PGameState.h"
#include "HexState.h"
#include "PolyState.h"
#include "GemPuzzleState.h"
#include "Parser.h"
//#include <cilk/cilk.h>
//#include <cilktools/cilkview.h>
using namespace std;

template <typename T>
void UCTPlayPGame(T &rstate, PlyOptions optplya, PlyOptions optplyb, int ngames, int nmoves, int swap, int verbose, bool twoPly) {
    if (twoPly) {
        //cilkview_data_t d;
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

        while (i < ngames) {
            T state(rstate);

            if (swap) {
                if (i % 2 == 0) {
                    black = "plya(B)";
                    //plya = "plya(B)";
                    white = "plyb(W)";
                    //plyb = "plyb(W)";
                } else {
                    black = "plyb(B)";
                    //plyb = "plyb(B)";
                    white = "plya(W)";
                    //plya = "plya(W)";
                }
            } else {
                black = "plya(B)";
                //plya = "plya(B)";
                white = "plyb(W)";
                //plyb = "plyb(W)";
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
            while (!state.IsTerminal() && j < nmoves) {
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
                if (state.GetPlyJM() == BLACK) {
                    ply = white;
                    if (white == "plya(W)") {
                        
                        //cin>>move;
                        //__cilkview_query(d);
                        plya.Run(state, move, log, log2);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    } else {
                        
                        //__cilkview_query(d);
                        plyb.Run(state, move, log, log2);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    }
                } else {
                    ply = black;
                    if (black == "plya(B)") {
                        
                        //cin>>move;
                        //__cilkview_query(d);
                        plya.Run(state, move, log, log2);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    } else {

                        //__cilkview_query(d);
                        plyb.Run(state, move, log, log2);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                    }
                }

                state.SetMove(move);

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

            float r = state.GetResult(state.GetPlyJM());
            if (r == WIN) {
                const char *c = state.GetPlyJM() == BLACK ? "+B " : "+W ";
                sprintf(buffer, c);

                if (i % 2 == 0) {
                    state.GetPlyJM() == BLACK ? plywina[state.GetPlyJM()]++ : plywinb[state.GetPlyJM()]++;
                } else {
                    state.GetPlyJM() == BLACK ? plywinb[state.GetPlyJM()]++ : plywina[state.GetPlyJM()]++;
                }
                plywin[state.GetPlyJM()]++;
            } else if (r == LOST) {
                const char *c = (CLEAR - state.GetPlyJM() == BLACK ? "+B " : "+W ");
                sprintf(buffer, c);

                if (i % 2 == 0) {
                    (CLEAR - state.GetPlyJM() == BLACK) ? plywina[CLEAR - state.GetPlyJM()]++ : plywinb[CLEAR - state.GetPlyJM()]++;
                } else {
                    (CLEAR - state.GetPlyJM() == BLACK) ? plywinb[CLEAR - state.GetPlyJM()]++ : plywina[CLEAR - state.GetPlyJM()]++;
                }
                plywin[CLEAR - state.GetPlyJM()]++;
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
            state.Reset();
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
    } else {

    }
}

template <typename T>
void UCTPlayHorner(T &rstate, PlyOptions optplya, int ngames, int verbose) {

    vector<unsigned int> seeda(optplya.nthreads);
    std::random_device dev;
    std::stringstream strVisit;
    std::vector<int> res(ngames);
    char fileName[100];

    for (int i = 0; i < ngames; i++) {
        T state(rstate);
        string log = " ";
        string log2 = " ";
        int move;
        for (int j = 0; j < optplya.nthreads; j++) {
            seeda[j] = (unsigned int) dev();
            assert(seeda[j] > 0 && "seed can not be negative\n");
        }
        optplya.seed = (unsigned int) dev();
        if (verbose) {
            cout << "# start game" << ","
                    << setw(8) << i << ","
                    << setw(10) << BLACK << ","
                    << setw(10) << WHITE << ","
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
                    << setw(10) << "move" << ","
                    << setw(10) << "reward" << endl;
        }
        if (verbose == 3) {
            strVisit.str().clear();
            strVisit.str(std::string());
            strVisit << "# start visit,"
                    << setw(9) << i << ","
                    << setw(10) << BLACK << ","
                    << setw(10) << WHITE << endl;
        }

        UCT<T> plya(optplya, verbose, seeda);
        int j = 0;
        T bestState;
        while (!state.IsTerminal()&& j < 1) {
            if (verbose) {
                cout << setw(9) << j << ",";
            }
            if (verbose == 3) {
                vector<MOVE> moves;
                state.GetMoves(moves);

                strVisit << setw(9) << "move no." << ","
                        << setw(10) << "player" << ",";
                for (int i = 0; i < moves.size(); i++)
                    strVisit << moves[i] << ",";
                strVisit << endl;
            }
            
            //__cilkview_query(d);
            bestState = plya.Run(state, move, log, log2);
            //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
            
            state.SetMove(move);
            if (verbose) {
                cout << setw(10) << state.GetPlyJM() << ",";
                cout << log;
                cout << setw(10) << move << ",";
                cout <<setw(10)<<bestState.GetResult(WHITE)<<endl;
            }
            if (verbose == 2)
                state.Print();
            if (verbose == 3) {
                strVisit << setw(9) << j << ","
                        << setw(10) << state.GetPlyJM() << ","
                        << log2 << endl;
            }
            if (verbose == 4) {
                std::sprintf(fileName, "%d.csv", move);
                state.PrintToFile(fileName);
            }
            //TODO dynamic cp  more exploitation as the tree become smaller
//            if(optplya.cp > 0.1){
//            optplya.cp=optplya.cp-0.1;
//            }
            j++;
        }
        state.Evaluate();
        res[i] = state.GetResult(WHITE);

        state.Reset();
        if (verbose) {
            cout << "# end game" << ","
                    << setw(10) << i << ","
                    << setw(10) << BLACK << ","
                    << setw(10) << WHITE << ","
                    << setw(10) << res[i] << endl;

            std::cout << "# start result" << std::endl;
            std::cout << "# game no" << ","
                    << setw(10) << "player" << ","
                    << setw(10) << "result" << std::endl;
            std::cout << setw(10) << i << ","
                    << setw(10) << WHITE << ","
                    << setw(10) << res[i] << std::endl;
            std::cout << "# end result" << std::endl;
        }
        if (verbose == 3) {
            cout << strVisit.str()
                    << "# end visit,"
                    << setw(10) << i << ","
                    << setw(10) << BLACK << ","
                    << setw(10) << WHITE << endl;
        }
    }

    std::cout << "# start statistic" << std::endl;
    std::cout << "# Avg" << ","
            << setw(5) << "Std" << ","
            << setw(5) << "Err" << std::endl;
    std::cout << setw(5) << getAverage(res) << ","
            << setw(5) << getStdDev(res) << ","
            << setw(5) << getStdDev(res) / sqrt(ngames) << std::endl;
    std::cout << "# end statistic" << std::endl;
}

// <editor-fold defaultstate="collapsed" desc="Negamax">
//void NegamaxPlayGame(PGameState &s, int b, int d, int itr, int ntry, int seed, int verbose) {
//    PGameState state(s);
//
//    Alphabeta pl1;
//    Alphabeta pl2;
//    int i = 0, pl1wins = 0, pl2wins = 0;
//    cout << "\nAB Play:\n";
//    while (i < ntry) {
//        while (!state.GameOver()) {
//            int move = -1, bestMove = -1;
//            if (state.GetPlyJM() == 1) {
//                move = pl2.ABNegamax2(state, -INF, +INF, -1, bestMove, verbose);
//            } else {
//                move = pl1.ABNegamax2(state, -INF, +INF, 1, bestMove, verbose);
//            }
//
//            state.SetMove(bestMove);
//            cout << "\nPlayer " << state.GetPlyJM() << " moved to child " << bestMove << " with index " << state.currIdx << endl;
//            //state.Print();
//        }
//
//        if (state.GetResult(state.GetPlyJM()) == 1.0) {
//            cout << "Player " << state.GetPlyJM() << " Wins!" << endl;
//            if (state.GetPlyJM() == 1) {
//                pl1wins++;
//            } else {
//                pl2wins++;
//            }
//        } else if (state.GetResult(state.GetPlyJM()) == 0.0) {
//            cout << "Player " << 3 - state.GetPlyJM() << " Wins!" << endl;
//            if ((3 - state.GetPlyJM()) == 1) {
//                pl1wins++;
//            } else {
//                pl2wins++;
//            }
//        } else {
//            cout << "It is a draw!" << endl;
//        }
//        i++;
//    }
//    //cout<<"player 1 wins:"<<pl1wins<<" Player 2 wins:"<<pl2wins<<endl;
//}
// </editor-fold>

static void ShowUsage(std::string name) {
    cerr << "Usage: " << name << " <option(s)> \n"
            << "Options:\n"
            //<< "\t-x\t\tThe game to play (default=0,Hex=1,P-Game=2,Horner=3,15-puzzle=4)\n"
            << "\t-p\t\tThe game to play (default=0,hex=1,p-Game=2,horner=3,15-puzzle=4)\n"
            << "\t-b\t\tBreath of the tree\n"
            << "\t-d\t\tDepth of the tree or dimension of the board\n"
            << "\t-o\t\tMax number of playouts(default=5000) for player a\n"
            << "\t-t\t\tMax number of playouts(default=5000) for player b\n"
            << "\t-n\t\tNumber of repeats\n"
            << "\t-m\t\tNumber of threads(default=1) for player a\n"
            << "\t-q\t\tNumber of threads(default=1) for player b\n"
            << "\t-y\t\tParallel method(default=0 tree=1,root=2) for player a\n"
            << "\t-w\t\tParallel method(default=0 tree=1,root=2) for player b\n"
            << "\t-x\t\tNumber of seconds(default=1) for player a\n"
            << "\t-z\t\tNumber of seconds(default=1) for player b\n"
            << "\t-e\t\tThe value of cp(default=0) for player a\n"
            << "\t-f\t\tThe value of cp(default=0) for player b\n"
            << "\t-v\t\tShow the output on screen(default=0)\n"
            << "\t-s\t\tSeed to be used by random number generator\n"
            << "\t-r\t\tThreading runtime for both players (default=0 c++11=1, boost threadpool=2, cilk_spawn=2, Tbb_taskgroup=4, cilk_for=5)\n"
            << "\t-a\t\tNumber of moves in a game\n"
            << "\t-c\t\tTurn off swap rule(set to 0)\n"
            //<< "\t-j\t\tHorner(3),15puzzle(4)\n"
            << "\t-i\t\tInput file\n"
            << endl;
}

int main(int argc, char** argv) {

    int b = 4, d = 6, seed = -1, ngames = 1, vflag = 0, nmoves = 99999, swap = 1,opt=0,hflag=0;
    char* game=const_cast<char*>("");
    char* par_a=const_cast<char*>("");
    char* par_b=const_cast<char*>("");
    char* threadlib=const_cast<char*>("");
    char* fileName=const_cast<char*>("");
    PlyOptions optplya, optplyb;

    // <editor-fold defaultstate="collapsed" desc="pars the arguments">
    while ((opt = getopt(argc, argv, "hg:b:d:o:t:m:q:y:w:x:z:n:v:s:e:f:r:a:c:i:p:")) != -1) {
        switch (opt) {
            case 'h':
                hflag = 1;
                break;
            case 'p':
                optplya.game=atoi(optarg);
                optplyb.game=atoi(optarg);
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
                nmoves = atoi(optarg);
                break;
            case 'c':
                swap = atoi(optarg);
                break;
            case 'i':
                fileName = const_cast<char*>(optarg);
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
        ShowUsage(argv[0]);
        exit(1);
    }

    switch (optplya.game) {
        case 1:
            game = const_cast<char*>("hex");
            break;
        case 2:
            game = const_cast<char*>("p-game");
            break;
        case 3:
            game = const_cast<char*>("horner");
            break;
        case 4:
            game = const_cast<char*>("15-puzzle");
            break;
        default:
            std::cerr << "No game is specified to be played!\n";
    }

    switch (optplya.par) {
        case 1:
            par_a = const_cast<char*>("tree");
            break;
        case 2:
            par_a = const_cast<char*>("root");
            break;
        default:
            par_a = const_cast<char*>("seq");
            break;
    }
    
        switch (optplyb.par) {
        case 1:
            par_b = const_cast<char*>("tree");
            break;
        case 2:
            par_b = const_cast<char*>("root");
            break;
        default:
            par_b = const_cast<char*>("seq");
            break;
    }

    if (optplya.par == 0) {
        optplya.nthreads = 1;
    }
    if (optplyb.par == 0) {
        optplyb.nthreads = 1;
    }

    switch (optplya.threadruntime) {
        case TBBSPSPIPELINE:
            threadlib = const_cast<char*> ("tbb_sps_pipeline");
            break;
        case THPOOL:
            threadlib = const_cast<char*> ("threadpool");
            break;
        case CILKPSPAWN:
            threadlib = const_cast<char*> ("cilk_spawn");
            break;
        case TBBTASKGROUP:
            threadlib = const_cast<char*> ("tbb_taskgroup");
            break;
        case CILKPFOR:
            threadlib = const_cast<char*> ("cilk_for");
            break;
        case CPP11:
            threadlib = const_cast<char*> ("c++11");
            break;
        default:
            threadlib = const_cast<char*> ("none");
            break;
    }
    
    if (nmoves == 0) {
        cout << "Can not run a game with zero moves\n";
        exit(0);
    }

    if (optplya.game == HEX) {
        printf("# plya game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s, cp=%0.3f\n",
                game, d, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a, optplya.cp);
        printf("# plyb game=%s, dim=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s, cp=%0.3f\n",
                game, d, optplyb.nsims, optplyb.nthreads, optplyb.nsecs, ngames, par_b, optplyb.cp);
    } else if (optplya.game == PGAME) {
        printf("# plya game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s\n",
                game, b, d, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a);
        printf("# plyb game=%s, breath=%d, depth=%d, nsims=%d, nthreads=%d, nsecs=%0.2f, ngames=%d, par=%s\n",
                game, b, d, optplyb.nsims, optplyb.nthreads, optplyb.nsecs, ngames, par_b);
    } else if (optplya.game == HORNER) {
        printf("# ply,game,input,nplayouts,nthreads,nsecs,nrepeats,par,threadlib,cp\n");
        printf("%s,%s,%s,%d,%d,%0.2f,%d,%s,%s,%0.2f\n",
                "a", game, fileName, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a,threadlib, optplya.cp);
    } else if (optplya.game == GEMPUZZLE) {
        printf("# ply,a\n# game,%s\n# input,%s\n# nplayouts,%d\n# nthreads,%d\n# nsecs,%0.2f\n# nrepeats,%d\n# par,%s\n",
                game, fileName, optplya.nsims, optplya.nthreads, optplya.nsecs, ngames, par_a);
    }

    for (int index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]); // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="play game">
    if (optplya.game == HEX) {
        HexGameState state(d);
        UCTPlayPGame<HexGameState>(state, optplya, optplyb, ngames, nmoves, swap, vflag, 1);
    } else if (optplya.game == PGAME) {
        //        PGameState state(b, d, 0x80, seed);
        //        UCTPlayPGame<PGameState>(state, optplya, optplyb, ngames,nmoves,swap, vflag,1);
    } else if (optplya.game == HORNER) {
        //pars input file for polynomial
        Parser parser;
        polynomial poly = parser.parseFile(fileName);
        PolyState state(poly);

        if (vflag == 4)
            state.PrintToFile(const_cast<char*> ("orig.csv"));

        optplya.minReward = 4100;
        UCTPlayHorner<PolyState>(state, optplya, ngames, vflag);
    } else if (optplya.game == GEMPUZZLE) {
        std::cerr << "15-puzzle is not implemented!\n";
        exit(0);
        //pars input file for puzzle
        //GemPuzzleState state("14,1,9,6,4,8,12,5,7,2,3,0,10,11,13,15");

        //UCTPlayGemPuzzle<GemPuzzleState>(state,optplya, ngames,vflag);
    } else {
        std::cerr << "Select a game to play!\n";
        exit(0);
    }
    //TODO a new method to get input from user is required. 

    //NegamaxPlayGame(state,b,d,1,1,seed,false);// </editor-fold>

    return 0;
}
