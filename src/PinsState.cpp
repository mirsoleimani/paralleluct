/* 
 * File:   PinsState.h
 * Author: Alfons Laarman
 * 
 * 
 * 
 * TODO:
 * - internalize random number generators to avoid costly getMoves
 * - pass "non-options" from main.cpp to HRE
 * - add action detection
 * - add invariant detection
 * - guided search
 *
 */

extern "C" {
#define SPINS

#include "hre/config.h"
#include "hre/user.h"
#include <ltsmin-lib/ltsmin-standard.h>
#include <ltsmin-lib/lts-type.h>
#include "pins-lib/pins.h"
#include "pins-lib/pins-impl.h"
#include "pins-lib/pins-util.h"
//#include "pins2lts-mc/parallel/options.h"
#include "mc-lib/cctables.h"
#include "util-lib/util.h"
#include "util-lib/chunk_support.h"
#undef Print
#undef swap
#undef min
#undef max
}



#include "Utilities.h"
#include <assert.h>
#include <random>
#include <cstdlib>
#include <random>
#include <unistd.h>
#include <limits>
#include "paralleluct/state/PinsState.h"

#define FORCE_STRING "force-procs"
static int HRE_PROCS = 0;

static struct poptOption options_mc[] = {
//#ifdef OPAAL
//     {NULL, 0, POPT_ARG_INCLUDE_TABLE, options_timed, 0, NULL, NULL},
//#else
//     {NULL, 0, POPT_ARG_INCLUDE_TABLE, options, 0, NULL, NULL},
//#endif
     {FORCE_STRING, 0, POPT_ARG_VAL | POPT_ARGFLAG_SHOW_DEFAULT | POPT_ARGFLAG_DOC_HIDDEN,
      &HRE_PROCS, 1, "Force multi-process in favor of pthreads", NULL},
     SPEC_POPT_OPTIONS,
     POPT_TABLEEND
};



static table_factory_t    factory = NULL;
static const char        *fname = NULL;

//thread_local

static int                  PLAYOUT_DEPTH;
static int                  PROPERTY;
static model_t              model = NULL;      // PINS implementation
static int                 n;          // nr of state slots
static int                 k;          // nr of transition groups
static int                 g;          // nr of transition guards
static int                 logk;       // log k
static int                 kmask;      // (1 << logk) - 1
static int                 act_label,
                           act_type,
                           act_index;
static ci_list            *vgroups;
static ci_list            *vguards;

static thread_local unsigned long long  seed[2] = {0, 0 };
static thread_local int *guards = NULL;

PinsState::PinsState(){
    if (seed[0] == 0 && seed[0] == seed[1]) {
        std::random_device dev;
        seed[0] = (unsigned long long) dev();
        seed[1] = (unsigned long long) dev();
    }

    cached = std::numeric_limits<float>::max();
    level = 0;
    current = (int *) malloc(sizeof(int[n]));   // Heavy malloc
}

PinsState::PinsState(const char *fileName, int d, int swap) : PinsState() {

    if (factory == NULL) {
        
        PLAYOUT_DEPTH = d;
        PROPERTY = swap;
        
         std::cout << "Starting HRE" << endl;
        // TODO: support multi-threaded code
        fname = fileName;
        
//        /* Init structures */
        HREinitBegin (fname);
//
        HREaddOptions (options_mc, "Parallel UCT options");
//
//        lts_lib_setup ();
//
//        if (false) {
//            HREenableFork (RTnumCPUs(), true);
//        } else {
//            HREenableThreads (RTnumCPUs(), true);
//        }
//
//        // spawns threads:
        
        char *a[1];
        int argc = 2;
        char **p = (char **) malloc(sizeof(char *[2]));
        p[0] = (char *) malloc(strlen("./parallelust2"));
        p[1] = (char *) fname;
        strcpy(*p, "./parallelust2");
        HREinitStart (&argc, &p, 1, 2, (char **)&fname, "");

        std::cout << "Creating model" << endl;
        cct_map_t *tables = cct_create_map (false); // HRE-aware object  
        factory = cct_create_table_factory  (tables);
        fileName = fname;
//    } 
    

//    if (model == NULL) {
        std::cout << "Loading model from "<< fname << endl;
        model = GBcreateBase ();
        GBsetChunkMap (model, factory); //HREgreyboxTableFactory());
        GBloadFile (model, fname, &model);   
        lts_type_t lts = GBgetLTStype(model);
        n = lts_type_get_state_length(lts);
        matrix_t *m = GBgetDMInfo(model);
        k = dm_nrows (m);
        g = dm_nrows( GBgetGuardNESInfo(model) );
        logk = ceil(log(k) / log(2));
        kmask = (1 << logk) - 1;
        assert (n = dm_ncols(m));

        lts_type_t          ltstype = GBgetLTStype (model);
        if (PROPERTY == 1) {
            // table number of first edge label
            act_label = lts_type_find_edge_label_prefix (
                    ltstype, LTSMIN_EDGE_TYPE_ACTION_PREFIX);
            if (act_label == -1) {
                std::cout << "No edge label 'action' for action detection" << endl;
                exit (1);
            }
            act_type = lts_type_get_edge_label_typeno (ltstype, act_label);
            chunk c = chunk_str("assert");
            act_index = pins_chunk_put  (model, act_type, c);
            int *groups = (int *) calloc (k, sizeof(int));
            GBsetPorGroupVisibility (model, groups);
            int *guards = (int *) calloc (g, sizeof(int));
            GBsetPorStateLabelVisibility (model, guards);
            pins_add_edge_label_visible (model, act_label, act_index);
            vgroups = ci_create (k);
            for (int i = 0; i< k; i++) ci_add_if (vgroups, i, groups[i] != 0);
            vguards = ci_create (g);
            for (int i = 0; i< g; i++) ci_add_if (vguards, i, guards[i] != 0);
        }

    }

    GBgetInitialState(model, current);
}

PinsState::PinsState(const PinsState &pins) : PinsState() {
    memcpy (current, pins.current, sizeof(int[n]));
    level = pins.level;
}

typedef struct cb_last_s {
    int     group;
    int     occurence;
} cb_last_t;

static cb_last_t LAST_INIT = { .group = -1, .occurence = -1 };

/**
 * Store transition group _occurences_ in moves, e.g.:
 * <(3,0), (4,0), (4,1), (4,2), (8,0), (8,1), (10,0)  >
 * 
 * Assumes group occurrences are consecutive.
 */
static void
cb_last(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_last_t *ctx = (cb_last_t *) context;
    if (ctx->group == ti->group) {
        ctx->occurence++;
    } else {
        ctx->occurence = 0;
        ctx->group = ti->group;
    }
    (void) cpy; (void) dst;
}

typedef struct cb_moves_s {
    cb_last_t    last;
    vector<int> &moves;
    int          depth;
} cb_moves_t;

static void
cb_moves(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_moves_t *ctx = (cb_moves_t *) context;
    cb_last (&ctx->last, ti, dst, cpy);
    ctx->moves.push_back(ctx->last.occurence << logk | ti->group);
    if (PROPERTY == 1 && ti->labels[act_label] == act_index) {
        std::cout << "Assertion violation found at depth " <<  ctx->depth << endl;
        exit (1);
    }
    (void) cpy; (void) dst;
}


/**
 * Find the untried moves in the current state.
 * @param moves an empty vector
 * @return number of untried moves
 */
int PinsState::GetMoves(vector<int>& moves) {
    assert(moves.size() == 0 && "The moves vector should be empty!\n");
    cb_moves_t ctx = { .last = LAST_INIT, .moves = moves, .depth = level };
    int total = GBgetTransitionsAll (model, current, cb_moves, &ctx);
    assert (total == modes.size());
    return moves.size();
}

float PinsState::GetResult(int plyjm) {
    return cached;
}

void PinsState::Evaluate() {
    if (cache == level) return;

    if (guards == NULL) {
        guards = (int *) malloc (sizeof(int[g]));
    }

    GBgetStateLabelsGroup (model, GB_SL_GUARDS, current, guards);

    float               c = 0;
    ci_list           **g2g = (ci_list **) GBgetGuardsInfo(model);

    if (PROPERTY == 1) {
        for (int *g = ci_begin(vgroups); g != ci_end(vgroups); g++) {
            for (int *n = ci_begin(g2g[*g]); n != ci_end(g2g[*g]); n++) {
                c += guards[*n] == 0;
            }
        }
        for (int *u = ci_begin(vguards); u != ci_end(vguards); u++) {
            c += guards[*u] == 0;
        }
        c /= g;
    } else {
        for (int i = 0; i < k; i++) {
            bool enabled = true;
            for (int *n = ci_begin(g2g[i]); n != ci_end(g2g[i]); n++) {
                enabled &= guards[*n] != 0;
            }
            c += enabled;
        }
        c /= k;
    }

    if (c < cached) {
        cached = c;
    }
    cache = level;
}

static void
cb_dummy(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    (void) context; (void) ti; (void) dst; (void) cpy; 
}

static void
deadlock_check (int c, int level)
{
    if (PROPERTY == 0 && c == 0) {
        std::cout << "Deadlock found at depth " << level << endl;
        exit (1);
    }
}

/*
 * Currently we check only for deadlocks
 * 
 * TODO: other safety properties.
*/
bool PinsState::PropertyViolated() {
    int count = GBgetTransitionsAll (model, current, cb_dummy, NULL);
    deadlock_check (count, level);
    return count == 0;
}

bool PinsState::IsTerminal() {
    return PropertyViolated();
}

int PinsState::GetPlyJM() {
    return WHITE;
}

typedef struct cb_move_s {
    int        *current;
    int         group;
    int         occurence;
    cb_last_t   last;
} cb_move_t;

static void
cb_move(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_move_t *ctx = (cb_move_t *) context;
    cb_last (&ctx->last, ti, dst, cpy);
    if (ctx->group == ctx->last.group && ctx->occurence == ctx->last.occurence) {
        memcpy (ctx->current, dst, sizeof(int[n]));
    }
    (void) cpy; (void) dst;
}

/**
 * ALFONS: Assuming here that the move integer comes from the moves vector
 * supplied by GetMoves.
 */
void PinsState::SetMove(int move) {
    level++;
    cb_move_t ctx = { .current = current,
                      .group = move & kmask,
                      .occurence = move >> logk,
                      .last = LAST_INIT
    };
//    for (int i = 0; i < n; i++) std::cout << current[i] <<", ";
//    std::cout << " <--"<< move <<"-->  ";
    GBgetTransitionsAll (model, current, cb_move, &ctx);
//    for (int i = 0; i < n; i++) std::cout << current[i] <<", ";
//    std::cout << endl;
}


void PinsState::SetPlayoutMoves(vector<int>& moves) {

    while (playout > 0) {
        Evaluate();

        moves.clear();
        GetMoves(moves);
        deadlock_check (moves.size(), level);

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, moves.size() - 1);

        
        SetMove(moves.at(dis(gen)));
        playout--;
    }
    moves.clear();
}

int PinsState::GetPlayoutMoves(vector<int>& moves) {
    playout = PLAYOUT_DEPTH;
    return 0;
}

int PinsState::GetMoveCounter(){
    return 2;
}

void PinsState::UndoMoves(int origMoveCounter) {
    assert (false && "not implemented");
}

void PinsState::Reset() {
    assert (false && "not implemented");
    GBgetInitialState(model, current);
    PLAYOUT_DEPTH = 0;
}

void PinsState::Print() {
    std::cout << std::endl;
}

void PinsState::PrintToFile(char* fileName) {
    std::ofstream ofs(fileName);

    ofs << std::endl;
}
