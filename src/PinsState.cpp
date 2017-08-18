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
#include <type_traits>
#define typeof(x) std::remove_reference<decltype((x))>::type
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
    #include "mc-lib/dbs-ll.h"
    #include "util-lib/fast_set.h"
    #include "util-lib/util.h"
    #include "util-lib/chunk_support.h"
    #undef Print
    #undef swap
    #undef min
    #undef max
}



#include "Utilities.h"
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

static void
Assert (bool b, string s)
{
    if (b) return;
    cout << s << endl;
    exit (1);
}

class StreamFormatter
{
private:

    std::ostringstream stream;

public:

    operator std::string() const
    {
        return stream.str();
    }

    template<typename T>
    StreamFormatter& operator << (const T& value)
    {
        stream << value;
        return *this;
    }
};

static std::string
Err(const std::string& message)
{
    return message;
}


#define Assert(EXPRESSION, MESSAGE) \
    if(!(EXPRESSION)) \
    { \
        cerr << __FILE__ <<":"<< __LINE__ << " " << \
            "Assertion ["<< #EXPRESSION <<"] failed: " << \
            Err(StreamFormatter() << MESSAGE) << endl; \
        exit(1); \
    }

#undef Debug
#define Debug(MESSAGE) \
    if(VERBOSE) \
    { \
        cerr << Err(StreamFormatter() << MESSAGE) << endl; \
    }


static table_factory_t    factory = NULL;
static const char        *fname = NULL;

//thread_local

static bool                 VERBOSE;
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
static treedbs_ll_t       tree;
static thread_local fset_t*fset = nullptr;
static thread_local tree_t             inn = NULL;
static thread_local tree_t             out = NULL;
static thread_local bool check_fset = false;
static tree_ref_t        initial = -1;

static thread_local int *guards = NULL;
static thread_local int  states = 0;

PinsState::PinsState(){
    cached = std::numeric_limits<float>::max();
    level = 0;
}

PinsState::PinsState(const char *fileName, int d, int swap, bool v) : PinsState() {

    if (factory == NULL) {
        
        PLAYOUT_DEPTH = d;
        PROPERTY = swap;
        VERBOSE = v;

        cout << "Detecting " << (PROPERTY == 1 ? "assertion violations" : "deadlocks") <<" "<< PROPERTY << endl;
        
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
        
//        char *a[1];
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
        Assert (n == dm_ncols(m), "Matrix problem?");

        tree = TreeDBSLLcreate_dm (n, 27, 1, m, 0, 0, 0);

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
            chunk c = chunk_str("Assert");
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

    if (fset == nullptr) {
        fset = fset_create (sizeof(int[k]), 0, 4, 10);
    }
    if (inn == NULL) {
        inn = (tree_t) malloc(sizeof(int[n << 1]));   // Heavy malloc
        out = (tree_t) malloc(sizeof(int[n << 1]));   // Heavy malloc
    }

    if (initial == -1) {
        int s[n];
        GBgetInitialState(model, s);
        int found = TreeDBSLLlookup_dm (tree, (const int *)s, (tree_t)NULL, inn, -1);
        Assert (found != DB_LEAFS_FULL && found != DB_ROOTS_FULL, "Tree DB full.");
        initial = TreeDBSLLindex (tree, inn);
    }
    Reset ();
}

PinsState::PinsState(const PinsState &pins) : PinsState() {
    //memcpy (current, pins.current, sizeof(int[n]));
    level = pins.level;
    state = pins.state;
    if (VERBOSE) cout << "Copy: "<< state << endl;
}

PinsState& PinsState::operator =(const PinsState& other) {
    if(&other == this)
        return *this;
    level = other.level;
    state = other.state;
    if (VERBOSE) cout << "Assign: "<< state << endl;
    return *this;
}
//static tree_ref_t
//get_idx (int *dst)
//{
//    int found = TreeDBSLLfind_dm (tree, (const int*) (dst), (tree_t) (NULL), inn, -1);
//    Assert (found == DB_NOT_FOUND, "Reexploring same state!");
//    return TreeDBSLLindex (tree, inn);
//}

typedef struct cb_last_s {
    int     group;
    int     occurence;
} cb_last_t;

static cb_last_t LAST_INIT = { .group = -1, .occurence = -1 };

typedef struct cb_moves_s {
    cb_last_t    last;
    vector<int> &moves;
    int          depth;
} cb_moves_t;

static void
deadlock_check (int c, int level)
{
    if (PROPERTY == 0 && c == 0) {
        std::cout << "Deadlock found at depth " << level << endl;
        //exit (1);
    }
}

static int *
update_state (tree_ref_t state)
{
    //if (TreeDBSLLindex(tree, tmp) != state) {
        inn = TreeDBSLLget(tree, state, inn);
        //Debug ("Update: "<< state);
    //}
    return TreeDBSLLdata (tree, inn);
}

/**
 * Store transition group _occurences_ in moves, e.g.:
 * <(3,0), (4,0), (4,1), (4,2), (8,0), (8,1), (10,0)  >
 *
 * Assumes group occurrences are consecutive.
 */
static inline void
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

static void
cb_moves(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_moves_t *ctx = (cb_moves_t *) context;

    cb_last (&ctx->last, ti, dst, cpy);

    if (TreeDBSLLfind_dm(tree, (const int *)dst, inn, out, ti->group) != DB_NOT_FOUND) {
        return;
    }

    if (check_fset && fset_find(fset, NULL, dst, NULL, false)) return;

    //Debug ("PUSH: "<< ti->group <<","<< ctx->last.occurence);
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
    Assert(moves.size() == 0, "The moves vector should be empty!\n");
    cb_moves_t ctx = { .last = LAST_INIT, .moves = moves, .depth = level };
    int *current = update_state (state);
    int total = GBgetTransitionsAll (model, current, cb_moves, &ctx);
    Assert (moves.size() <= total, "Mopre moves?");
    deadlock_check (total, level);
    return moves.size();
}

typedef struct cb_move_s {
    tree_ref_t  state;
    int         group;
    int         occurence;
    cb_last_t   last;
} cb_move_t;

static void
cb_move(void *context, transition_info_t *ti, int *dst, int *cpy)
{
    cb_move_t *ctx = (cb_move_t *) context;

    cb_last (&ctx->last, ti, dst, cpy);
    //Debug ("Process move: " << ctx->last.group <<","<< ctx->last.occurence <<"   "<< ctx->group <<","<< ctx->occurence);

    if (ctx->group == ctx->last.group && ctx->occurence == ctx->last.occurence) {
        if (check_fset) {
            fset_find(fset, NULL, dst, NULL, true);
        }
        int found = TreeDBSLLfop_dm (tree, (const int*) (dst), inn, out, ti->group, !check_fset);
        Assert (found != DB_LEAFS_FULL && found != DB_ROOTS_FULL, "Tree DB full.");
        //Assert (!store || found == 0, "Tree DB full.");
        states += !check_fset;
        ctx->state = TreeDBSLLindex (tree, out);
    }
    (void) cpy; (void) dst;
}

void PinsState::SetMove(int move) {
    level++;
    cb_move_t ctx = { .state = (tree_ref_t) -1,
                      .group = move & kmask,
                      .occurence = move >> logk,
                      .last = LAST_INIT
    };
    int *current = update_state (state);
    //Debug ("GetMove: "<< state << " move: " << ctx.group <<","<< ctx.occurence);
    int total = GBgetTransitionsAll (model, current, cb_move, &ctx);
    Assert (ctx.state != -1, "Move not found " << total);
    state = ctx.state;
    deadlock_check (total, level);
    //Debug ("Move: "<< state);
}

float PinsState::GetResult(int plyjm) {
        return cached;
//    if (PropertyViolated()) {
//        return 1;
//    } else {
//        return 0;
//    }
}

void PinsState::Evaluate() {
    if (cache == level) return;

    if (guards == NULL) {
        guards = (int *) malloc (sizeof(int[g]));
    }

    int *current = update_state (state);
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
        //c /= g;
    } else {
        for (int i = 0; i < k; i++) {
            bool enabled = true;
            for (int *n = ci_begin(g2g[i]); n != ci_end(g2g[i]); n++) {
                enabled &= guards[*n] != 0;
            }
            c += enabled;
        }
        //c /= k;
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

/*
 * Currently we check only for deadlocks
 * 
 * TODO: other safety properties.
*/
bool PinsState::PropertyViolated() {
//    return false; // Currently, PinsState exits, when an error is found
    int *current = update_state (state);
    int count = GBgetTransitionsAll (model, current, cb_dummy, NULL);
    deadlock_check (count, level);
    return count == 0;    
}

/**
 * it is a terminal state when no more move exist.
 * @return 
 */
bool PinsState::IsTerminal() {
    return PropertyViolated();
}

int PinsState::GetPlyJM() {
    return WHITE;
}

void PinsState::SetPlayoutMoves(vector<int>& moves) {

    if (playout > 1) {
        fset_clear (fset);
    }

    int old = playout;

    if (playout && VERBOSE) cout << "<<<<<<<<<<<" << endl;
    while (playout > 0) {
        Evaluate();

        moves.clear();
        GetMoves(moves);
        //deadlock_check (moves.size(), level);

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, moves.size() - 1);

        if (moves.size()) {
            // not all seen states

            if (playout > 1) {
                check_fset = true;
            }
            SetMove(moves.at(dis(gen)));
            if (playout > 1) {
                check_fset = false;
            }
        }

        playout--;
    }
    if (old && VERBOSE) cout << ">>>>>>>>>>>" << endl;
    moves.clear();
}

int PinsState::GetPlayoutMoves(vector<int>& moves) {
    playout = PLAYOUT_DEPTH;
    return 0;
}

int PinsState::GetMoveCounter(){
    Assert (false, "not implemented");
}

void PinsState::UndoMoves(int origMoveCounter) {
    Assert (false, "not implemented");
}

void PinsState::Reset() {
    state = initial;
    //Debug ("Initial: "<< state);
    playout = 0;
}

void PinsState::Print() {
    std::cout << std::endl;
}

void PinsState::PrintToFile(char* fileName) {
    std::ofstream ofs(fileName);

    ofs << std::endl;
}

void PinsState::Stats() {
    cout << "Number of states: "<< states << endl;
}

int PinsState::GetScore(){
    return k;
}