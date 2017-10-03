/* 
 * File:   main.cpp
 * Author: SAM
 *
 * Created on July 8, 2014, 5:19 PM
 */

#include <cstdlib>
#include <valarray>
#include "paralleluct/UCT.h"
#include "paralleluct/state/PGameState.h"
#include "paralleluct/state/HexState.h"
#include "paralleluct/state/PolyState.h"
#include "paralleluct/state/GemPuzzleState.h"
#include "paralleluct/state/Parser.h"
//#include <cilk/cilk.h>
//#include <cilktools/cilkview.h>
using namespace std;

static const vector<string> THREADLIBNAME = {"none","c++11","threadpool","cilk_spawn","tbb_task_group","cilk_for","tbb_sps_pipeline"};
static const vector<string> PARMETHODNAME = {"sequential","tree","root","pipe-5","pipe-6-3","pipe-6-4-s","pipe-6-4-b"};
static const vector<string> GAMENAME = {"none","hex","pgame","horner","gem-puzzle"};
static const vector<string> LOCKMETHODNAME = {"lock-free","fine-lock","coarse-lock"};
static const vector<string> BACKUPDIRECTNAME= {"bottom-up","top-down"};

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
        double time=0;

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
                        plya.Run(state, move, log, log2,time);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    } else {
                        
                        //__cilkview_query(d);
                        plyb.Run(state, move, log, log2,time);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    }
                } else {
                    ply = black;
                    if (black == "plya(B)") {
                        
                        //cin>>move;
                        //__cilkview_query(d);
                        plya.Run(state, move, log, log2,time);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);
                        
                    } else {

                        //__cilkview_query(d);
                        plyb.Run(state, move, log, log2,time);
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
void UCTPlayHorner(T &rstate, PlyOptions optplya, int ngames, int verbose,bool initgame) {

    vector<unsigned int> seeda(optplya.nthreads);
    std::random_device dev;
    std::stringstream strVisit;
    std::vector<int> result(ngames);
    std::vector<vector<int>> nplayouts(optplya.nmoves);
    std::vector<vector<int>> reward(optplya.nmoves);
    std::vector<vector<double>> time(optplya.nmoves);
    std::vector<vector<int>> plyamaxdepth(optplya.nmoves);
    double ttime;
    char fileName[100];

    for (int i = 0; i < ngames; i++) {
        T state(rstate);
        string log = " ";
        string log2 = " ";
        int move;
#ifdef MKLRNG
        optplya.seed = (unsigned int) dev();
#else
        for (int j = 0; j < optplya.nthreads; j++) {
            seeda[j] = (unsigned int) dev();
            assert(seeda[j] > 0 && "seed can not be negative\n");
        }
#endif
        if (verbose) {
            cout << "# start game" << ","
                    << setw(8) << i << endl;
//                    << setw(10) << BLACK << ","
//                    << setw(10) << WHITE << ","
//                    << setw(10) << "?" << endl;

            cout << setw(9) << "# move no." << ","
                    << setw(10) << "player" << ","
                    << setw(10) << "playout" << ","
                    << setw(10) << "time" << ","
                    << setw(10) << "select(%)" << ","
                    << setw(10) << "expand(%)" << ","
                    << setw(10) << "playout(%)" << ","
                    << setw(10) << "backup(%)" << ","
                    << setw(10) << "nrandvec" << ","
                    << setw(10) << "depth" << ","
                    << setw(10) << "move" << ","
                    << setw(10) << "reward" << endl;
        }
        if (verbose == 3) {
            strVisit.str().clear();
            strVisit.str(std::string());
            strVisit << "# start visit"<<","
                    << setw(9) << i << endl;
//                    << setw(10) << BLACK << ","
//                    << setw(10) << WHITE << endl;
        }

        UCT<T> plya(optplya, verbose, seeda);
        int j = 0;
        T bestState(state);
        while (!state.IsTerminal()&& j < optplya.nmoves && bestState.GetResult(WHITE) > optplya.bestreward) {
            if (verbose) {
                cout << setw(9) << j << ",";
            }
            if (verbose == 3) {
                vector<MOVE> moves;
                state.GetMoves(moves);

                strVisit << setw(9) << "move no." << ","
                        << setw(10) << "player" << ",";
                for (unsigned int i = 0; i < moves.size(); i++)
                    strVisit << moves[i] << ",";
                strVisit << endl;
            }
            
            //__cilkview_query(d);
            bestState = plya.Run(state, move, log, log2,ttime);
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
            //if (i > 0) {
                nplayouts[j].push_back(plya.NumPlayoutsRoot());
                reward[j].push_back(bestState.GetResult(WHITE));
                time[j].push_back(ttime);
                plyamaxdepth[j].push_back(plya.GetMaxDepthofTree());
            //}
            j++;
        }
        state.Evaluate();
        result[i] = state.GetResult(WHITE);

        state.Reset();
        if (verbose) {
            cout << "# end game" << endl;
//                    << setw(10) << i << ","
//                    << setw(10) << BLACK << ","
//                    << setw(10) << WHITE << ","
//                    << setw(10) << res[i] << endl;

            std::cout << "# start result" << std::endl;
            std::cout << "# game no" << ","
                    << setw(10) << "player" << ","
                    << setw(10) << "seed"   << ","
                    << setw(10) << "result" << std::endl;
            std::cout << setw(10) << i << ","
                    << setw(10) << WHITE << ","
                    << setw(10)  << optplya.seed << ","
                    << setw(10) << result[i] << std::endl;
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
    if(!initgame){
    std::cout << "# start statistic" << std::endl;
//    std::cout << "# avg(result)" << ","
//            << setw(10) << "atd(result)" << ","
//            << setw(10) << "err(result)" << std::endl;
//    std::cout << setw(10) << getAverage(result) << ","
//            << setw(10) << getStdDev(result) << ","
//            << setw(10) << getStdDev(result) / sqrt(ngames) << std::endl;
            std::cout << "# move no." << ","
                << setw(10) << "player"<< ","
                << setw(15) << "avg(playout)" << ","
                << setw(10) << "std(playout)" << ","
                << setw(10) << "err(playout)" << ","
                << setw(10) << "avg(time)" << ","
                << setw(10) << "std(time)" << ","
                << setw(10) << "err(time)" << ","
                << setw(10) << "avg(depth)" << ","
                << setw(10) << "std(depth)" << ","
                << setw(10) << "err(depth)" << ","
                << setw(10) << "avg(reward)" << ","
                << setw(10) << "std(reward)" << ","
                << setw(10) << "err(reward)"  << endl;
        for (int i=0; i<nplayouts.size();i++) {
            if(nplayouts[i].size()>1)
                std::cout << setw(10) << i << ","
                    << setw(10) << "plya" << ","  
                    << setw(15) << getAverage(nplayouts[i]) << ","
                    << setw(10) << getStdDev(nplayouts[i]) << ","
                    << setw(10) << getStdDev(nplayouts[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(time[i]) << ","
                    << setw(10) << getStdDev(time[i]) << ","
                    << setw(10) << getStdDev(time[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(plyamaxdepth[i]) << ","
                    << setw(10) << getStdDev(plyamaxdepth[i]) << ","
                    << setw(10) << getStdDev(plyamaxdepth[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(reward[i]) << ","
                    << setw(10) << getStdDev(reward[i]) << ","
                    << setw(10) << getStdDev(reward[i]) / sqrt(ngames) << endl;
        }
    std::cout << "# end statistic" << std::endl;
    }
}

template <typename T>
void UCTPlayGame(T &rstate, PlyOptions optplya, PlyOptions optplyb, int ngames, int nmoves, int swap, int verbose, bool twoPly,bool initgame) {

    //cilkview_data_t d;
#ifdef THREADPOOL
    boost::threadpool::pool thread_pool(NTHREADS);
#endif
    std::random_device dev;
    std::stringstream strVisit;

    std::vector<int> result(ngames);
    std::vector<vector<int>> nplayouts(optplya.nmoves);
    std::vector<vector<int>> plyanplayouts(optplya.nmoves);
    std::vector<vector<int>> plybnplayouts(optplya.nmoves);
    std::vector<vector<double>> plyatime(optplya.nmoves);
    std::vector<vector<double>> plybtime(optplya.nmoves);
    std::vector<vector<int>> reward(optplya.nmoves);
    std::vector<vector<double>> time(optplya.nmoves);
    std::vector<vector<int>> plyamaxdepth(optplya.nmoves);
    std::vector<vector<int>> plybmaxdepth(optplya.nmoves);
    string ply = "none";
    string black;
    string white;
    vector<int> plywin(3,0);
    vector<int> plywina(3,0);
    vector<int> plywinb(3,0);
    
    double ttime = 0;
    char fileName[100];
    int move = 0;
    int i = 0;
    char buffer[100];
                string log;
                string log2;
    if (twoPly) {
        vector<unsigned int> seeda(optplya.nthreads);
        vector<unsigned int> seedb(optplyb.nthreads);

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

#ifdef MKLRNG
            optplya.seed = (unsigned int) dev();
            optplyb.seed = (unsigned int) dev();
#else
            for (int j = 0; j < optplya.nthreads; j++) {
                seeda[j] = (unsigned int) dev();
                assert(seeda[j] > 0 && "seed can not be negative\n");
            }
            for (int j = 0; j < optplyb.nthreads; j++) {
                seedb[j] = (unsigned int) dev();
                assert(seedb[j] > 0 && "seed can not be negative\n");
            }
#endif
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
                        << setw(10) << "depth" << ","
                        << setw(10) << "move" << endl;               
            }
            if (verbose == 3) {
                strVisit.str().clear();
                strVisit.str(std::string());
                strVisit << "# start visit,"
                        << setw(9) << i << ","
                        << setw(10) << black << ","
                        << setw(10) << white << endl;
                                strVisit.str().clear();
            }

            UCT<T> plya(optplya, verbose, seeda);
            UCT<T> plyb(optplyb, verbose, seedb);
            T bestState;
            int j = 0;
            while (!state.IsTerminal() && j < optplya.nmoves) {
                log.clear();
                log2.clear();
                int plyno=0;
                if (verbose) {
                    cout << setw(9) << j << ",";
                }
                if (verbose == 3) {
                    vector<MOVE> moves;
                    state.GetMoves(moves);

                    strVisit << setw(9) << "move no." << ","
                            << setw(10) << "player" << ",";
                    for (unsigned int i = 0; i < moves.size(); i++)
                        strVisit << moves[i] << ",";
                    strVisit << endl;
                }
                
                if (state.GetPlyJM() == BLACK) {
                    ply = white;
                    if (white == "plya(W)") {
                        plyno = 0;
                        //cin>>move;
                        //__cilkview_query(d);
                        bestState = plya.Run(state, move, log, log2, ttime);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);

                    } else {
                        plyno=1;
                        //__cilkview_query(d);
                        bestState = plyb.Run(state, move, log, log2, ttime);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);

                    }
                } else {
                    ply = black;
                    if (black == "plya(B)") {
                        plyno=0;
                        //cin>>move;
                        //__cilkview_query(d);
                        bestState = plya.Run(state, move, log, log2, ttime);
                        //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);

                    } else {
                        plyno=1;
                        //__cilkview_query(d);
                        bestState = plyb.Run(state, move, log, log2, ttime);
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
                //if (i > 0) {
                    if(plyno==0){
                        plyanplayouts[j].push_back(plya.NumPlayoutsRoot());
                        plyatime[j].push_back(ttime);
                        plyamaxdepth[j].push_back(plya.GetMaxDepthofTree());
                    }else{
                        plybnplayouts[j].push_back(plyb.NumPlayoutsRoot());
                        plybtime[j].push_back(ttime);
                        plybmaxdepth[j].push_back(plyb.GetMaxDepthofTree());
                    }
                    //reward[j].push_back(bestState.GetResult(WHITE));
                    //time[j].push_back(ttime);
                //}
                j++;
            }

            state.Evaluate();
            int pjm = state.GetPlyJM();
            result[i] = state.GetResult(pjm);
            
            state.Reset();            
            if (result[i] == WIN) {
                const char *c = pjm == BLACK ? "+B " : "+W ";
                sprintf(buffer, c);
                if(swap){
                if (i % 2 == 0) {
                    pjm == BLACK ? plywina[pjm]++ : plywinb[pjm]++;
                } else {
                    pjm == BLACK ? plywinb[pjm]++ : plywina[pjm]++;
                }
                } else {
                    pjm == BLACK ? plywina[BLACK]++ : plywinb[WHITE]++;
                }
                plywin[pjm]++;
//            } else if (result[i] == LOST) {
//                const char *c = (CLEAR - pjm == BLACK ? "+B " : "+W ");
//                sprintf(buffer, c);
//                if(swap){
//                if (i % 2 == 0) {
//                    (CLEAR - pjm == BLACK) ? plywina[CLEAR - pjm]++ : plywinb[CLEAR - pjm]++;
//                } else {
//                    (CLEAR - pjm == BLACK) ? plywinb[CLEAR - pjm]++ : plywina[CLEAR - pjm]++;
//                }
//                } else {
//                    (CLEAR - pjm == BLACK) ? plywinb[CLEAR - pjm]++ : plywina[CLEAR - pjm]++;
//                }
//                plywin[CLEAR - pjm]++;
            } else if (result[i] == DRAW) {
                sprintf(buffer, "+Draw");
            }
            if (verbose) {
                cout << "# end game" << ","
                        << setw(10) << i << ","
                        << setw(10) << black << ","
                        << setw(10) << white << ","
                        << setw(10) << buffer << endl;

                std::cout << "# start result" << std::endl;
                std::cout << "# game no" << ","
                        << setw(10) << "player" << ","
                        << setw(10) << "seed" << ","
                        << setw(10) << "result" << std::endl;
                std::cout << setw(10) << i << ","
                        << setw(10) << "plya" << ","
                        << setw(10) << optplya.seed << ","
                        << setw(10) << plywina[BLACK]+plywina[WHITE] << std::endl;
                std::cout << setw(10) << i << ","
                        << setw(10) << "plyb" << ","
                        << setw(10) << optplyb.seed << ","
                        << setw(10) << plywinb[BLACK]+plywinb[WHITE]  << std::endl;
                std::cout << "# end result" << std::endl;
            }
            if (verbose == 3) {
                cout << strVisit.str()
                        << "# end visit,"
                        << setw(10) << i << ","
                        << setw(10) << black << ","
                        << setw(10) << white << endl;
            }
            i++;
        }
    } else {
        vector<unsigned int> seeda(optplya.nthreads);

        for (i = 0; i < ngames; i++) {
            T state(rstate);
            log.clear();
            log2.clear();
            int move;
#ifdef MKLRNG
            optplya.seed = (unsigned int) dev();
#else
            for (int j = 0; j < optplya.nthreads; j++) {
                seeda[j] = (unsigned int) dev();
                assert(seeda[j] > 0 && "seed can not be negative\n");
            }
#endif
            if (verbose) {
                cout << "# start game" << ","
                        << setw(8) << i << endl;

                cout << setw(9) << "# move no." << ","
                        << setw(10) << "player" << ","
                        << setw(10) << "playout" << ","
                        << setw(10) << "time" << ","
                        << setw(10) << "select(%)" << ","
                        << setw(10) << "expand(%)" << ","
                        << setw(10) << "playout(%)" << ","
                        << setw(10) << "backup(%)" << ","
                        << setw(10) << "nrandvec" << ","
                        << setw(10) << "depth" << ","
                        << setw(10) << "move" << ","
                        << setw(10) << "reward" << endl;
            }
            if (verbose == 3) {
                strVisit.str().clear();
                strVisit.str(std::string());
                strVisit << "# start visit" << ","
                        << setw(9) << i << endl;
            }

            UCT<T> plya(optplya, verbose, seeda);
            int j = 0;
            T bestState;
            while (!state.IsTerminal() && j < optplya.nmoves) {
                if (verbose) {
                    cout << setw(9) << j << ",";
                }
                if (verbose == 3) {
                    vector<MOVE> moves;
                    state.GetMoves(moves);

                    strVisit << setw(9) << "move no." << ","
                            << setw(10) << "player" << ",";
                    for (unsigned int i = 0; i < moves.size(); i++)
                        strVisit << moves[i] << ",";
                    strVisit << endl;
                }

                //__cilkview_query(d);
                bestState = plya.Run(state, move, log, log2, ttime);
                //__cilkview_report(&d, NULL, "main_tag", CV_REPORT_WRITE_TO_RESULTS);

                state.SetMove(move);
                if (verbose) {
                    cout << setw(10) << state.GetPlyJM() << ",";
                    cout << log;
                    cout << setw(10) << move << ",";
                    cout << setw(10) << bestState.GetResult(WHITE) << endl;
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
                //if (i > 0) {
                    nplayouts[j].push_back(plya.NumPlayoutsRoot());
                    reward[j].push_back(bestState.GetResult(WHITE));
                    time[j].push_back(ttime);
                    plyamaxdepth[j].push_back(plya.GetMaxDepthofTree());
                //}
                j++;
            }
            state.Evaluate();
            result[i] = state.GetResult(WHITE);

            state.Reset();
            if (verbose) {
                cout << "# end game" << endl;

                std::cout << "# start result" << std::endl;
                std::cout << "# game no" << ","
                        << setw(10) << "player" << ","
                        << setw(10) << "seed" << ","
                        << setw(10) << "result" << std::endl;
                std::cout << setw(10) << i << ","
                        << setw(10) << WHITE << ","
                        << setw(10) << optplya.seed << ","
                        << setw(10) << result[i] << std::endl;
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
    }
    if(!initgame){
    if (twoPly) {
        std::cout << "# start statistic" << std::endl;

        std::cout << "# move no." << ","
                << setw(10) << "player"<< ","
                << setw(15) << "avg(playout)" << ","
                << setw(10) << "std(playout)" << ","
                << setw(10) << "err(playout)" << ","
                << setw(10) << "avg(time)" << ","
                << setw(10) << "std(time)" << ","
                << setw(10) << "err(time)" << ","
                << setw(10) << "avg(depth)" << ","
                << setw(10) << "std(depth)" << ","
                << setw(10) << "err(depth)" << ","
                << setw(10) << "wins(%)" << ","
                << setw(10) << "wins" << ","
                << setw(10) << "Black" << ","
                << setw(10) << "White" << endl;
        for (int i=0; i<plyanplayouts.size();i++) {
            if(plyanplayouts[i].size()>1)
                std::cout << setw(10) << i << ","
                    << setw(10) << "plya" << ","  
                    << setw(15) << getAverage(plyanplayouts[i]) << ","
                    << setw(10) << getStdDev(plyanplayouts[i]) << ","
                    << setw(10) << getStdDev(plyanplayouts[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(plyatime[i]) << ","
                    << setw(10) << getStdDev(plyatime[i]) << ","
                    << setw(10) << getStdDev(plyatime[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(plyamaxdepth[i]) << ","
                    << setw(10) << getStdDev(plyamaxdepth[i]) << ","
                    << setw(10) << getStdDev(plyamaxdepth[i]) / sqrt(ngames) << ","
                    << setw(10) << ((plywina[1] + plywina[2]) / (float) ngames)*100 << ","
                    << setw(10) << (plywina[1] + plywina[2]) << ","
                    << setw(10) << plywina[1] << ","
                    << setw(10) << plywina[2] << endl;
        }
        for (int i=0; i<plybnplayouts.size();i++) {
            if(plybnplayouts[i].size()>1)
                std::cout << setw(10)<< i << ","
                    << setw(10) << "plyb" << ","  
                    << setw(15) << getAverage(plybnplayouts[i]) << ","
                    << setw(10) << getStdDev(plybnplayouts[i]) << ","
                    << setw(10) << getStdDev(plybnplayouts[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(plybtime[i]) << ","
                    << setw(10) << getStdDev(plybtime[i]) << ","
                    << setw(10) << getStdDev(plybtime[i]) / sqrt(ngames) << ","
                    << setw(10) << getAverage(plybmaxdepth[i]) << ","
                    << setw(10) << getStdDev(plybmaxdepth[i]) << ","
                    << setw(10) << getStdDev(plybmaxdepth[i]) / sqrt(ngames) << ","
                    << setw(10) << ((plywinb[1] + plywinb[2]) / (float) ngames)*100 << ","
                    << setw(10) << (plywinb[1] + plywinb[2]) << ","
                    << setw(10) << plywinb[1] << ","
                    << setw(10) << plywinb[2] << endl;
        }

        std::cout << "# end statistic" << std::endl;
    } else {
        std::cout << "# start statistic" << std::endl;
        std::cout << "# avg(result)" << ","
                << setw(10) << "atd(result)" << ","
                << setw(10) << "err(result)" << std::endl;
        std::cout << setw(10) << getAverage(result) << ","
                << setw(10) << getStdDev(result) << ","
                << setw(10) << getStdDev(result) / sqrt(ngames - 1) << std::endl;
        int i = 0;
        std::cout << "# avg(playout)" << ","
                << setw(10) << "std(playout)" << ","
                << setw(10) << "srr(playout)" << ","
                << "move no." << std::endl;
        for (auto itr : nplayouts) {
            std::cout << setw(10) << getAverage(itr) << ","
                    << setw(10) << getStdDev(itr) << ","
                    << setw(10) << getStdDev(itr) / sqrt(ngames - 1) << ","
                    << setw(10) << i++ << std::endl;
        }
        std::cout << "# avg(reward)" << ","
                << setw(10) << "std(reward)" << ","
                << setw(10) << "err(reward)" << ","
                << "move no." << std::endl;
        i = 0;
        for (auto itr : reward) {
            std::cout << setw(10) << getAverage(itr) << ","
                    << setw(10) << getStdDev(itr) << ","
                    << setw(10) << getStdDev(itr) / sqrt(ngames - 1) << ","
                    << setw(10) << i++ << std::endl;
        }
        std::cout << "# avg(time)" << ","
                << setw(10) << "std(time)" << ","
                << setw(10) << "err(time)" << ","
                << "move no." << std::endl;
        i = 0;
        for (auto itr : time) {
            std::cout << setw(10) << getAverage(itr) << ","
                    << setw(10) << getStdDev(itr) << ","
                    << setw(10) << getStdDev(itr) / sqrt(ngames - 1) << ","
                    << setw(10) << i++ << std::endl;
        }
        std::cout << "# end statistic" << std::endl;
    }
    }
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
            << "\t-p\t\tThe game to play (default=none,hex=1,p-game=2,horner=3,gem-puzzle=4)\n"
            << "\t-b\t\tBreath of the tree\n"
            << "\t-d\t\tDepth of the tree or dimension of the board\n"
            << "\t-o\t\tMax number of playouts for player a (default=1048576) \n"
            << "\t-t\t\tMax number of playouts for player b (default=1048576)\n"
            << "\t-n\t\tNumber of repeats\n"
            << "\t-m\t\tNumber of threads for player a (default=1) \n"
            << "\t-q\t\tNumber of threads for player b (default=1)\n"
            << "\t-y\t\tParallel method for player a (default=sequential, tree=1, root=2, pipe(depth 5)=3, pipe(depth 6-3)=4, "
            "pipe(depth 6-4, select parallel))=5, pipe(depth 6-4, backup parallel)=6\n"
            << "\t-w\t\tParallel method for player b (default=sequential, tree=1, root=2, pipe(depth 5)=3, pipe(depth 6-3)=4, "
            "pipe(depth 6-4, select parallel))=5, pipe(depth 6-4, backup parallel)=6\n"
            << "\t-x\t\tNumber of seconds for player a (default=1)\n"
            << "\t-z\t\tNumber of seconds for player b (default=1)\n"
            << "\t-e\t\tThe value of cp for player a (default=1)\n"
            << "\t-f\t\tThe value of cp for player b (default=1)\n"
            << "\t-v\t\tShow the output on screen(default=0,save search tree in file=3)\n"
//            << "\t-s\t\tSeed to be used by random number generator\n"
            << "\t-r\t\tThreading runtime for both players (default=none c++11=1, boost threadpool=2, cilk_spawn=2, Tbb_taskgroup=4, cilk_for=5, tbb_sps_pipeline=6)\n"
            << "\t-a\t\tNumber of moves in the game\n"
            << "\t-c\t\tswap rule(default=on, off=0)\n"
            << "\t-l\t\tvirtual loss(default=off, on=1)\n"
            << "\t-i\t\tInput file\n"
            << "\t-k\t\tLocking method(default=lock-free,fine-lock=1,coarse-lock=2)\n"
            << "\t-B\t\tBackup direction for player a (default=bottom up,top down=1)\n"
            << "\t-D\t\tBackup direction for player b (default=bottom up,top down=1)\n"
            << endl;
}

void PrintMetadata(PlyOptions optplya, PlyOptions optplyb) {
    int w = 10;
    cout << "# start metadata\n";
    std::cout << setw(3) << right << "ply" << ","
            << setw(10) << right << "game" << ","
            << setw(10) << right << "input" << ","
            << setw(10) << right << "nplayouts" << ","
            << setw(10) << right << "nthreads" << ","
            << setw(10) << right << "nsecs" << ","
            << setw(10) << right << "nrepeats" << ","
            << setw(10) << right << "par" << ","
            << setw(15) << right << "threadlib" << ","
            << setw(10) << right << "cp" << ","
            << setw(10) << right << "virtualloss" << ","
            << setw(10) << right << "locking" << ","
            << setw(15) << right << "backup-direct" << ","
            << setw(6) << right << ((optplya.game == GAME::PGAME) ? ("depth,") : "")
            << setw(7) << right << ((optplya.game == GAME::PGAME) ? ("breath,") : "")
            << setw(5) << right << ((optplya.game == GAME::HEX) ? ("dim") : "")
    << "\n";
    std::cout << setw(3) << right << "a" << ","
            << setw(10) << right << GAMENAME[optplya.game] << ","
            << setw(10) << right << optplya.fileName.substr(optplya.fileName.find_last_of("/\\") + 1) << ","
            << setw(10) << right << optplya.nsims << ","
            << setw(10) << right << optplya.nthreads << ","
            << setw(10) << right << optplya.nsecs << ","
            << setw(10) << right << optplya.ngames << ","
            << setw(10) << right << PARMETHODNAME[optplya.par] << ","
            << setw(15) << right << THREADLIBNAME[optplya.threadruntime] << ","
            << setw(10) << right << optplya.cp << ","
            << setw(10) << right << optplya.virtualloss << ","
            << setw(10) << right << LOCKMETHODNAME[optplya.locking] << ","
            << setw(15) << right << BACKUPDIRECTNAME[optplya.backupDirection] << ","
            << setw(6) << right << ((optplya.game == GAME::PGAME) ? NumToStr(optplya.depth)+"," : "") 
            << setw(7) << right << ((optplya.game == GAME::PGAME) ? NumToStr(optplya.breath)+"," : "")
            << setw(5) << right << ((optplya.game == GAME::HEX) ? NumToStr(optplya.dim) : "")
    << "\n";
    std::cout << setw(3) << right << "b" << ","
            << setw(10) << right << GAMENAME[optplyb.game] << ","
            << setw(10) << right << optplya.fileName.substr(optplya.fileName.find_last_of("/\\") + 1) << ","
            << setw(10) << right << optplyb.nsims << ","
            << setw(10) << right << optplyb.nthreads << ","
            << setw(10) << right << optplyb.nsecs << ","
            << setw(10) << right << optplyb.ngames << ","
            << setw(10) << right << PARMETHODNAME[optplyb.par] << ","
            << setw(15) << right << THREADLIBNAME[optplyb.threadruntime] << ","
            << setw(10) << right << optplyb.cp << ","
            << setw(10) << right << optplyb.virtualloss << ","
            << setw(10) << right << LOCKMETHODNAME[optplyb.locking] << ","
            << setw(15) << right << BACKUPDIRECTNAME[optplyb.backupDirection] << ","
            << setw(6) << right << ((optplyb.game == GAME::PGAME) ? NumToStr(optplyb.depth)+"," : "")
            << setw(7) << right << ((optplyb.game == GAME::PGAME) ? NumToStr(optplyb.breath)+"," : "")
            << setw(5) << right << ((optplyb.game == GAME::HEX) ? NumToStr(optplyb.dim) : "")
    << "\n";
    cout << "# end metadata\n";
}

int main(int argc, char** argv) {

    int opt=0,hflag=0;
    PlyOptions optplya, optplyb;
              
    // <editor-fold defaultstate="collapsed" desc="pars the arguments">
    while ((opt = getopt(argc, argv, "hp:g:b:d:o:t:m:q:y:w:x:z:n:v:s:e:f:r:a:c:i:l:k:B:D:")) != -1) {
        switch (opt) {
            case 'h':
                hflag = 1;
                break;
            case 'p':
                if (GAME::NOGAME < atoi(optarg) & atoi(optarg) < GAME::LASTGAME) {
                    optplya.game = atoi(optarg);
                    optplyb.game = atoi(optarg);
                } else {
                    std::cerr << "ERROR:(-p) No game is specified to be played!\n";
                    exit(0);
                }
                break;
            case 'b':
                if(optplya.game == GAME::PGAME){                    
                    optplya.breath = atoi(optarg);
                    optplyb.breath = atoi(optarg);
                }
                break;
            case 'd':
                //if(optplya.game == GAME::PGAME){
                    optplya.depth = atoi(optarg);
                    optplyb.depth = atoi(optarg);
                //} else if (optplya.game == GAME::HEX){
                    optplya.dim = atoi(optarg);
                    optplyb.dim = atoi(optarg);
                //}
                break;
            case 'o':
                if (atoi(optarg) > 1) {
                    optplya.nsims = atoi(optarg);
                } else {
                    std::cerr << "ERROR:(-o) number of playouts should be more than 1.\n";
                    exit(0);
                }
                break;
            case 't':
                if (atoi(optarg) > 1) {
                    optplyb.nsims = atoi(optarg);
                } else {
                    std::cerr << "ERROR:(-t) number of playouts should be more than 1.\n";
                    exit(0);
                }
                break;
            case 'm':
                optplya.nthreads = atoi(optarg);
                break;
            case 'q':
                optplyb.nthreads = atoi(optarg);
                break;
            case 'y':
                if (THREADLIB::NONE < atoi(optarg) && atoi(optarg) < THREADLIB::LASTTHREADLIB){
                    optplya.par = atoi(optarg);
                } else {
                    std::cerr << "ERROR:(-y) No runtime thread library is specified!\n";
                    exit(0);
                }
                break;
            case 'w':
                if ( THREADLIB::NONE < atoi(optarg) && atoi(optarg) < THREADLIB::LASTTHREADLIB){
                    optplyb.par = atoi(optarg);
                } else {
                    std::cerr << "ERROR:(-w) No runtime thread library is specified!\n";
                    exit(0);
                }
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
                optplya.ngames = atoi(optarg);
                optplyb.ngames = atoi(optarg);
                break;
            case 'v':
                optplya.verbose = atoi(optarg);
                optplyb.verbose = atoi(optarg);
                break;
//            case 's':
//                seed = atoi(optarg);
//                break;
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
                if (atoi(optarg) > 0) {
                    optplya.nmoves = atoi(optarg);
                    optplyb.nmoves = atoi(optarg);
                } else {
                    std::cerr << "Can not run a game with zero moves\n";
                    exit(0);
                }
                break;
            case 'c':
                optplya.swap = atoi(optarg);
                optplyb.swap = atoi(optarg);
                break;
            case 'i':
                optplya.fileName=optarg;
                optplyb.fileName=optarg;
                break;
            case 'l':
                optplya.virtualloss = atoi(optarg);
                optplyb.virtualloss = atoi(optarg);
                break;
            case 'k':
                if(-1 < atoi(optarg) && atoi(optarg)<LOCKMETHOD::LASTLOCKMETHOD){
                    optplya.locking = atoi(optarg);
                    optplyb.locking = atoi(optarg);
                } else {
                    std::cerr << "The locking method is not supported.\n";
                    exit(0);
                }
                break;
            case 'B':
                if(-1 < atoi(optarg) && atoi(optarg)<BACKUPDIRCT::LASTBACKUPDIRECT){
                    optplya.backupDirection = atoi(optarg);
                } else {
                    std::cerr << "The locking method is not supported.\n";
                    exit(0);
                }
                break;
            case 'D':
                if(-1 < atoi(optarg) && atoi(optarg)<BACKUPDIRCT::LASTBACKUPDIRECT){
                    optplyb.backupDirection = atoi(optarg);
                } else {
                    std::cerr << "The locking method is not supported.\n";
                    exit(0);
                }
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
    if (optplya.par == SEQUENTIAL) {
        optplya.nthreads = 1;
    }
    if (optplyb.par == SEQUENTIAL) {
        optplyb.nthreads = 1;
    }
    

#ifdef LOCKFREE
    optplya.locking=LOCKMETHOD::FREELOCK;
    optplyb.locking=LOCKMETHOD::FREELOCK;
#elif defined(COARSEGRAINED)
    optplya.locking=LOCKMETHOD::COARSEGRAINLOCK;
    optplyb.locking=LOCKMETHOD::COARSEGRAINLOCK;
#elif FINEGRAINED
    optplya.locking=LOCKMETHOD::FINEGRAINLOCK;
    optplyb.locking=LOCKMETHOD::FINEGRAINLOCK;
#endif
    
    PrintMetadata(optplya,optplyb);
    for (int index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]); // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="play game">
    if (optplya.game == HEX) {
        optplya.twoply=1;
        optplyb.twoply=1;
        if(optplya.nmoves == 0 || optplyb.nmoves == 0){
            optplya.nmoves = optplya.dim*optplya.dim;
            optplyb.nmoves = optplya.dim*optplya.dim;
        }
#ifdef HEXSTATICBOARD
        Parser parser;
        vector<vector<int>> edgeList = parser.MakeHexEdgeList(optplya.dim);
        vector<int> leftPosList = parser.MakeHexLeftPos(optplya.dim);
        HexGameState state(optplya.dim,edgeList,leftPosList);
#else
        HexGameState state(optplya.dim);
#endif
        cout<<"# start warmup game...\n";
        PlyOptions optplyainit = optplya;
        PlyOptions optplybinit = optplyb;
        optplyainit.nsecs = 10;
        optplybinit.nsecs = 10;
        UCTPlayGame<HexGameState>(state, optplyainit, optplybinit, 1, optplyainit.nmoves, optplya.swap, optplya.verbose, 1,true);
        cout<<"# end warmup game.\n";
        UCTPlayGame<HexGameState>(state, optplya, optplyb, optplya.ngames, optplya.nmoves, optplya.swap, optplya.verbose, 1,false);
    } else if (optplya.game == PGAME) {
        std::cerr << "pgame is not implemented!\n";
        exit(0);
        //        PGameState state(b, d, 0x80, seed);
        //        UCTPlayPGame<PGameState>(state, optplya, optplyb, ngames,nmoves,swap, vflag,1);
    } else if (optplya.game == HORNER) {
        //pars input file for polynomial
        Parser parser;
        polynomial poly = parser.parseFile(optplya.fileName.c_str());
        PolyState state(poly);
        if(optplya.nmoves == 0){
            vector<int> moves;
            optplya.nmoves=state.GetMoves(moves);
        }
        optplya.twoply=0;    
        if (optplya.verbose == 4)
            state.PrintToFile(const_cast<char*> ("orig.csv"));
        cout<<"# start warmup game...\n";
        PlyOptions optplyainit = optplya;
        optplyainit.nsecs = 10;
        UCTPlayHorner<PolyState>(state, optplya, 1, optplya.verbose,true);
        cout<<"# end warmup game.\n";
        UCTPlayHorner<PolyState>(state, optplya, optplya.ngames, optplya.verbose,false);
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
