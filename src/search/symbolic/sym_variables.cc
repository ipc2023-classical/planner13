#include "sym_variables.h"

#include <iostream>
#include <memory>
#include <string>
#include "../task_representation/labels.h"

#include "../utils/debug_macros.h"

#include "../options/options.h"
#include "../options/option_parser.h"
#include "opt_order.h"
#include "../task_representation/transition_system.h"
#include "../utils/rng_options.h"


using namespace std;
using options::Options;

namespace symbolic {
SymVariables::SymVariables(const Options &opts, const shared_ptr<task_representation::FTSTask> &_task) :
    cudd_init_nodes(opts.get<int>("cudd_init_nodes")),
    cudd_init_cache_size(opts.get<int>("cudd_init_cache_size")),
    cudd_init_available_memory(opts.get<int>("cudd_init_available_memory")),
    gamer_ordering(opts.get<bool>("gamer_ordering")),
    rng(utils::parse_rng_from_options(opts)),
    task(_task) { cout << "Creating symvariables" << endl;}

void SymVariables::init() {
    vector <int> _var_order;
    if (gamer_ordering) {
        InfluenceGraph::compute_gamer_ordering(_var_order, task, rng);
    } else {
        for (size_t i = 0; i < size_t(task->get_size()); ++i) {
            _var_order.push_back(i);
        }
    }
    cout << "Sym variable order: ";
    for (int v : _var_order)
        cout << v << " ";
    cout << endl;

    init(_var_order);
}

//Constructor that makes use of global variables to initialize the symbolic_search structures
void SymVariables::init(const vector <int> &v_order) {
    cout << "Initializing Symbolic Variables" << endl;
    var_order = vector<int>(v_order);
    int num_fd_vars = var_order.size();

    //Initialize binary representation of variables.
    numBDDVars = 0;
        bdd_index_src = vector<vector<int>>(v_order.size());
        bdd_index_target = vector<vector<int>>(v_order.size());

    int _numBDDVars = 0;
    for (int var : var_order) {
        int var_len = ceil(log2(task->get_ts(var).get_size()));
        numBDDVars += var_len;
        for (int j = 0; j < var_len; j++) {
            bdd_index_src[var].push_back(_numBDDVars);
            bdd_index_target[var].push_back(_numBDDVars + 1);
            _numBDDVars += 2;
        }
    }
    cout << "Num variables: " << var_order.size() << " => " << numBDDVars << endl;

    //Initialize manager
    cout << "Initialize Symbolic Manager(" << _numBDDVars << ", "
         << cudd_init_nodes / _numBDDVars << ", "
         << cudd_init_cache_size << ", "
         << cudd_init_available_memory << ")" << endl;
    _manager = std::make_unique<Cudd> (_numBDDVars, 0,
                                          cudd_init_nodes / _numBDDVars,
                                          cudd_init_cache_size,
                                          cudd_init_available_memory);

    _manager->setHandler(exceptionError);
    _manager->setTimeoutHandler(exceptionError);
    _manager->setNodesExceededHandler(exceptionError);

    cout << "Generating binary variables" << endl;
    //Generate binary_variables
    for (int i = 0; i < _numBDDVars; i++) {
        variables.push_back(_manager->bddVar(i));
    }

    DEBUG_MSG(cout << "Generating predicate BDDs: " << num_fd_vars << endl;);
    source_state_BBDs.resize(num_fd_vars);
    target_state_BDDs.resize(num_fd_vars);
    biimpBDDs.resize(num_fd_vars);
    validValues.resize(num_fd_vars);
    validBDD = oneBDD();
    //Generate predicate (precondition (s) and effect (s')) BDDs
    for (int var : var_order) {
        for (int j = 0; j < task->get_ts(var).get_size(); j++) {
            source_state_BBDs[var].push_back(createPreconditionBDD(var, j));
            target_state_BDDs[var].push_back(createEffectBDD(var, j));
        }
        validValues[var] = zeroBDD();
        for (int j = 0; j < task->get_ts(var).get_size(); j++) {
            validValues[var] += source_state_BBDs[var][j];
        }
        validBDD *= validValues[var];
        biimpBDDs[var] = createBiimplicationBDD(bdd_index_src[var], bdd_index_target[var]);
    }

    binState.resize(_numBDDVars, 0);
    cout << "Symbolic Variables... Done." << endl;
}

BDD SymVariables::getStateBDD(const std::vector<int> &state) const {
    BDD res = _manager->bddOne();
    for (int i = int(var_order.size()) - 1; i >= 0; i--) {
        res = res * source_state_BBDs[var_order[i]][state[var_order[i]]];
    }
    return res;
}

BDD SymVariables::getInitialStateBDD() const {
    BDD res = _manager->bddOne();

    for (int i = var_order.size() - 1; i >= 0 ; --i)
        res *= source_state_BBDs[var_order[i]][task->get_ts(var_order[i]).get_init_state()];

    return res;
}

BDD SymVariables::getGoalBDD() const {
    BDD res = _manager->bddOne();

    for (int i = int(var_order.size()) - 1; i >= 0 ; --i) {
        BDD lts_res = _manager->bddZero();

        task_representation::TransitionSystem ts = task->get_ts(var_order[i]);
        std::vector<int> goals = ts.get_goal_states();
        for (auto goal_state : goals) {
            lts_res += source_state_BBDs[var_order[i]][goal_state];
        }
        res *= lts_res;
    }

    return res;
}

    BDD SymVariables::getGoalBDD(const set<int>& relevantVars) const { // TODO check with Alvaro
        BDD res = _manager->bddOne();

        for (const auto& rel_var : relevantVars) {
            BDD lts_res = _manager->bddZero();

            task_representation::TransitionSystem ts = task->get_ts(var_order[rel_var]);
            std::vector<int> goals = ts.get_goal_states();
            for (auto goal_state : goals) {
                lts_res += source_state_BBDs[var_order[rel_var]][goal_state];
            }
            res *= lts_res;
        }

        return res;
    }

bool SymVariables::isIn(const std::vector<int> &state, const BDD &bdd) const {
    BDD sBDD = getStateBDD(state);
    return !((sBDD * bdd).IsZero());
}

double SymVariables::numStates(const BDD &bdd) const {
    return bdd.CountMinterm(numBDDVars);
}

double SymVariables::numStates() const {
    return numStates(validBDD);
}

double SymVariables::numStates(const Bucket &bucket) const {
    double sum = 0;
    for (const BDD &bdd : bucket) {
        sum += numStates(bdd);
    }
    return sum;
}

BDD SymVariables::generateBDDVar(const std::vector<int> &_bddVars, int value) const {
    BDD res = _manager->bddOne();
    for (int v : _bddVars) {
        if (value % 2) { //Check if the binary variable is asserted or negated
            res = res * variables[v];
        } else {
            res = res * (!variables[v]);
        }
        value /= 2;
    }
    return res;
}

BDD SymVariables::createBiimplicationBDD(const std::vector<int> &vars, const std::vector<int> &vars2) const {
    BDD res = _manager->bddOne();
    for (size_t i = 0; i < vars.size(); i++) {
        res *= variables[vars[i]].Xnor(variables[vars2[i]]);
    }
    return res;
}

vector <BDD> SymVariables::getBDDVars(const vector <int> &vars, const vector<vector<int>> &v_index) const {
    vector<BDD> res;
    for (int v : vars) {
        for (int bddv : v_index[v]) {
            res.push_back(variables[bddv]);
        }
    }
    return res;
}

BDD SymVariables::getCube(int var, const vector<vector<int>> &v_index) const {
    BDD res = oneBDD();
    for (int bddv : v_index[var]) {
        res *= variables[bddv];
    }
    return res;
}

BDD SymVariables::getCube(const set <int> &vars, const vector<vector<int>> &v_index) const {
    BDD res = oneBDD();
    for (int v : vars) {
        for (int bddv : v_index[v]) {
            res *= variables[bddv];
        }
    }
    return res;
}


void
exceptionError(string /*message*/) {
    //cout << message << endl;
    throw BDDError();
}


void SymVariables::print() {
    ofstream file("variables.txt");

    for (int v : var_order) {
        file << "vars: ";
        for (int j : bdd_index_src[v])
            cout << j << " ";
        file << endl;
//        for (auto fact : g_fact_names[v]) // TODO ask
//            file << fact << endl;
    }
}

void SymVariables::print_options() const {
    cout << "CUDD Init: nodes=" << cudd_init_nodes <<
        " cache=" << cudd_init_cache_size <<
        " max_memory=" << cudd_init_available_memory <<
        " ordering: " << (gamer_ordering ? "gamer" : "fd") << endl;
}

void SymVariables::add_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int> ("cudd_init_nodes", "Initial number of nodes in the cudd manager.",
                             "16000000");

    parser.add_option<int> ("cudd_init_cache_size",
                             "Initial number of cache entries in the cudd manager.", "16000000");

    parser.add_option<int> ("cudd_init_available_memory",
                             "Total available memory for the cudd manager.", "0");

    parser.add_option<bool> ("gamer_ordering", "Use Gamer ordering optimization", "true");

    // Add random_seed option.
    utils::add_rng_options(parser);
}
}
