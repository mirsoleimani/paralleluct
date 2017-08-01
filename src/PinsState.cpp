/* 
 * File:   PinsState.h
 * Author: Alfons Laarman
 *
 */
extern "C" {
#define SPINS

#include "hre/config.h"
#include "hre/user.h"
#include "pins-lib/pins.h"
#include "pins-lib/pins-impl.h"
//#include "pins2lts-mc/parallel/options.h"
#include "mc-lib/cctables.h"
#undef Print
}
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



#define CUTOFF INT_MAX // control with nmoves (-a ?)

static int          lowest = CUTOFF;

static table_factory_t    factory = NULL;
static const char        *fname = NULL;

//thread_local

static model_t model = NULL;      // PINS implementation
static int                 n;          // nr of state slots
static int                 k;          // nr of transition groups
static int                 logk;       // log k
static int                 kmask;      // (1 << logk) - 1


PinsState::PinsState(){
    current = (int *) malloc(sizeof(int[n]));   // Heavy malloc
}

PinsState::PinsState(const char *fileName) : PinsState() {

    if (factory == NULL) {
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
        logk = ceil(log(k) / log(2));
        kmask = (1 << logk) - 1;
        assert (n = dm_ncols(m));
    }

    GBgetInitialState(model, current);
    depth = 0;
}

PinsState::PinsState(const PinsState &pins) : PinsState() {
    memcpy (current, pins.current, sizeof(int[n]));
    depth = pins.depth;
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
    }
    (void) cpy; (void) dst;
}

typedef struct cb_moves_s {
    cb_last_t    last;
    vector<int> &moves;
} cb_moves_t;

static void
cb_moves(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_moves_t *ctx = (cb_moves_t *) context;
    cb_last (&ctx->last, ti, dst, cpy);
    ctx->moves.push_back(ctx->last.occurence << logk | ctx->last.group);
    (void) cpy; (void) dst;
}


/**
 * Find the untried moves in the current state.
 * @param moves an empty vector
 * @return number of untried moves
 */
int PinsState::GetMoves(vector<int>& moves) {
    assert(moves.size() == 0 && "The moves vector should be empty!\n");
    cb_moves_t ctx = { .last = LAST_INIT, .moves = moves };
    int total = GBgetTransitionsAll (model, current, cb_moves, &ctx);
    assert (total == modes.size());
    return moves.size();
}

float PinsState::GetResult(int plyjm) {
   return CUTOFF - lowest;
}

void PinsState::Evaluate() { // done in SetMove (bad?)
}

static void
cb_dummy(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    (void) context; (void) ti; (void) dst; (void) cpy; 
}

/*
 * Currently we check only for deadlocks
 * 
 * TODO: other safety properties.
*/
bool PinsState::PropertyViolated() {
    return GBgetTransitionsAll (model, current, cb_dummy, NULL) == 0;
}

bool PinsState::IsTerminal() {
    return depth == CUTOFF || PropertyViolated();
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
    depth++;
    cb_move_t ctx = { .current = current,
                      .group = move & kmask,
                      .occurence = move >> logk,
                      .last = LAST_INIT
    };
    int total = GBgetTransitionsAll (model, current, cb_move, &ctx);
}

void PinsState::SetPlayoutMoves(vector<int>& moves) {
    if (IsTerminal()) {
        return;
    }
}

int PinsState::GetPlayoutMoves(vector<int>& moves) {
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
    depth = 0;
}

void PinsState::Print() {
    std::cout << std::endl;
}

void PinsState::PrintToFile(char* fileName) {
    std::ofstream ofs(fileName);

    ofs << std::endl;
}
