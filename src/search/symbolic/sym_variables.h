#ifndef SYMBOLIC_SYM_VARIABLES_H
#define SYMBOLIC_SYM_VARIABLES_H

#include "sym_bucket.h"

#include "../utils/timer.h"
#include "../globals.h"
#include "../task_representation/fts_task.h"
#include "state_reordering.h"
#include "variable_ordering.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <cassert>

namespace options {
class Options;
class OptionParser;
}

namespace symbolic {
/*
 * BDD-Variables for a symbolic exploration.
 * This information is global for every class using symbolic search.
 * The only decision fixed here is the variable ordering, which is assumed to be always fixed.
 */
struct BDDError {};
extern void exceptionError(std::string message);

class SymVariables {
// Var order used by the algorithm.
    //const VariableOrderType VariableOrdering;
    //Parameters to initialize the CUDD manager
    const long cudd_init_nodes; //Number of initial nodes
    const long cudd_init_cache_size; //Initial cache size
    const long cudd_init_available_memory; //Maximum available memory (bytes)
    const std::shared_ptr<VariableOrdering> variable_ordering;
    const std::shared_ptr<StateReordering> state_reordering;

    std::unique_ptr<Cudd> _manager; //_manager associated with this symbolic search

    // random number generator used for optOrder and VariableOrder
//    std::shared_ptr<utils::RandomNumberGenerator> rng;

    int numBDDVars; //Number of binary variables (just one set, the total number is numBDDVars*2
    std::vector<BDD> variables; // BDD variables

    //The variable order must be complete.
    std::vector <int> var_order; //Variable(FD) order in the BDD
    std::vector <std::vector <int>> bdd_index_src, bdd_index_target; //vars(BDD) for each var(FD)

    // BDDs that use FTSTask
    std::vector<std::vector<BDD>> source_state_BBDs; // BDDs associated with the source states of transitions
    std::vector<std::vector<BDD>> target_state_BDDs; // BDDs associated with the target states of transitions

    std::vector<BDD> biimpBDDs;  //BDDs associated with the biimplication of one variable(FD)
    std::vector<BDD> validValues; // BDD that represents the valid values of all the variables
    BDD validBDD;  // BDD that represents the valid values of all the variables


    //Vector to store the binary description of a state
    //Avoid allocating memory during heuristic evaluation
    std::vector <int> binState;

    void init(const std::vector <int> &v_order, std::map<int, std::vector<int>> var_to_state);

public:
    const std::shared_ptr<task_representation::FTSTask> &task;
    SymVariables(const options::Options &opts, const std::shared_ptr<task_representation::FTSTask> &_task);
    void init();

    BDD getInitialStateBDD() const;
    BDD getGoalBDD() const;
    BDD getGoalBDD(const std::set<int>& relevantVars) const;
    BDD getStateBDD(const std::vector<int> &state) const;

    double numStates(const BDD &bdd) const; //Returns the number of states in a BDD
    double numStates() const;
    double numStates(const Bucket &bucket) const;


    double percentageNumStates(const BDD &bdd) const {
        return numStates(bdd) / numStates();
    }

    bool isIn(const std::vector<int> &state, const BDD &bdd) const;

    inline const std::vector<int> &vars_index_pre(int variable) const {
        return bdd_index_src[variable];
    }

    inline const std::vector<int> &vars_index_eff(int variable) const {
        return bdd_index_target[variable];
    }

    inline const BDD &sourceStateBDD(int lts, int value_state) {
        return source_state_BBDs[lts][value_state];
    }

    inline const BDD &targetStateBDD(int lts, int value_state) {
        return target_state_BDDs[lts][value_state];
    }

    // Old --v
//    inline const BDD &preBDD(int variable, int value) const {
//        return preconditionBDDs [variable] [value];
//    }
//
//    inline const BDD &effBDD(int variable, int value) const {
//        return effectBDDs [variable] [value];
//    }

    inline BDD getCubePre(int var) const {
        return getCube(var, bdd_index_src);
    }
    inline BDD getCubePre(const std::set <int> &vars) const {
        return getCube(vars, bdd_index_src);
    }

    inline BDD getCubeEff(int var) const {
        return getCube(var, bdd_index_target);
    }
    inline BDD getCubeEff(const std::set <int> &vars) const {
        return getCube(vars, bdd_index_target);
    }

    inline const BDD &biimp(int variable) const {
        return biimpBDDs[variable];
    }

    inline long totalNodes() const {
        return _manager->ReadNodeCount();
    }

    inline std::vector <BDD> getBDDVarsPre() const {
        return getBDDVars(var_order, bdd_index_src);
    }
    inline std::vector <BDD> getBDDVarsEff() const {
        return getBDDVars(var_order, bdd_index_target);
    }
    inline std::vector <BDD> getBDDVarsPre(const std::vector <int> &vars) const {
        return getBDDVars(vars, bdd_index_src);
    }
    inline std::vector <BDD> getBDDVarsEff(const std::vector <int> &vars) const {
        return getBDDVars(vars, bdd_index_target);
    }

    inline unsigned long totalMemory() const {
        return _manager->ReadMemoryInUse();
    }

    inline double totalMemoryGB() const {
        return _manager->ReadMemoryInUse()/(1024*1024*1024);
    }

    inline BDD zeroBDD() const {
        return _manager->bddZero();
    }

    inline BDD oneBDD() const {
	    assert(_manager);
        return _manager->bddOne();
    }

    inline BDD validStates() const {
        return validBDD;
    }

    inline Cudd *mgr() const {
        return _manager.get();
    }

    inline BDD bddVar(int index) const {
        return variables[index];
    }

    inline BDD bddNewVar(int index) {
        return _manager->bddVar(index);
    }

    inline int usedNodes() const {
        return _manager->ReadSize();
    }

    inline void setTimeLimit(int maxTime) {
        _manager->SetTimeLimit(maxTime);
        _manager->ResetStartTime();
    }

    inline void unsetTimeLimit() {
        _manager->UnsetTimeLimit();
    }

    void print();
    
    template <class T> 
    int *getBinaryDescription(const T &state) {
        int pos = 0;
        //  cout << "State " << endl;
        for (int v : var_order) {
            //cout << v << "=" << state[v] << " " << g_variable_domain[v] << " assignments and  " << binary_len[v] << " variables   " ;
            //preconditionBDDs[v] [state[v]].PrintMinterm();

            for (size_t j = 0; j < bdd_index_src[v].size(); j++) {
                binState[pos++] = ((state[v] >> j) % 2);
                binState[pos++] = 0; //Skip interleaving variable
            }
        }
        /* cout << "Binary description: ";
           for(int i = 0; i < pos; i++){
           cout << binState[i];
           }
           cout << endl;*/

        return &(binState[0]);
    }


    inline ADD getADD(int value) {
        return _manager->constant(value);
    }

    inline ADD getADD(std::map<int, BDD> heur) {
        ADD h = getADD(-1);
        for (const auto &entry : heur) {
            int distance = 1 + entry.first;
            h += entry.second.Add() * getADD(distance);
        }
        return h;
    }

    static void add_options_to_parser(options::OptionParser &parser);

    void print_options() const;


private:
    //Auxiliar function helping to create precondition and effect BDDs
    //Generates value for bddVars.
    BDD generateBDDVar(const std::vector<int> &_bddVars, int value) const;
    BDD getCube(int var, const std::vector<std::vector<int>> &v_index) const;
    BDD getCube(const std::set <int> &vars, const std::vector<std::vector<int>> &v_index) const;
    BDD createBiimplicationBDD(const std::vector<int> &vars, const std::vector<int> &vars2) const;
    std::vector <BDD> getBDDVars(const std::vector <int> &vars, const std::vector<std::vector<int>> &v_index) const;


    inline BDD createPreconditionBDD(int variable, int value) const {
        return generateBDDVar(bdd_index_src[variable], value);
    }

    inline BDD createEffectBDD(int variable, int value) const {
        return generateBDDVar(bdd_index_target[variable], value);
    }

    inline int getNumBDDVars() const {
        return numBDDVars;
    }
    };
}

#endif
