#ifndef TASK_REPRESENTATION_SAS_TASK_H
#define TASK_REPRESENTATION_SAS_TASK_H

#include "sas_operator.h"

#include "../operator_id.h"

#include "../utils/hash.h"
#include "../utils/collections.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>

namespace options {
class Options;
}

class GlobalState;

namespace task_representation {
struct FactPair;
class SASTask {
    bool g_use_metric;

    int g_min_action_cost;
    int g_max_action_cost;

    std::vector<SASOperator> g_operators;
    std::vector<SASOperator> g_axioms;

    // This vector holds the initial values *before* the axioms have been evaluated.
    // Use a state registry to obtain the real initial state.
    std::vector<int> g_initial_state_data;
    std::vector<std::pair<int, int>> g_goal;
    std::vector<int> initial_state_values; //Values in the initial state (after evaluating axioms)

    std::vector<std::string> g_variable_name;
    std::vector<int> g_variable_domain;
    std::vector<std::vector<std::string>> g_fact_names;
    std::vector<int> g_axiom_layers;
    std::vector<int> g_default_axiom_values;


    void read_and_verify_version(std::istream &in);
    void read_metric(std::istream &in);
    void read_variables(std::istream &in);
    void read_mutexes(std::istream &in);
    void read_goal(std::istream &in);
    void read_operators(std::istream &in);
    void read_axioms(std::istream &in);


// TODO: This needs a proper type and should be moved to a separate
//       mutexes.cc file or similar, accessed via something called
//       g_mutexes. (Right now, the interface is via global function
//       are_mutex, which is at least better than exposing the data
//       structure globally.)

    std::vector<std::vector<std::set<FactPair>>> g_inconsistent_facts;


    const SASOperator & get_operator_or_axiom(int index, bool is_axiom) const;

public:
    SASTask();
    void read_from_file(std::istream &in);
    ~SASTask() = default;
    int get_num_variables() const ;
    std::string get_variable_name(int var) const ;
    int get_variable_domain_size(int var) const ;
    int get_variable_axiom_layer(int var) const ;
    int get_variable_default_axiom_value(int var) const ;
    std::string get_fact_name(const FactPair &fact) const ;
    bool are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const ;

    int get_operator_cost(int index, bool is_axiom = false) const ;
    std::string get_operator_name(int index, bool is_axiom = false) const ;

    const std::vector<SASOperator> & get_operators() const {
        return g_operators;
    }

    const SASOperator & get_operator(int index) const {
        return g_operators[index];
    }


    int get_num_operators() const ;
    int get_num_operator_preconditions(int index, bool is_axiom) const ;
    FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const ;
    int get_num_operator_effects(int op_index, bool is_axiom) const ;
    int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const ;
    FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const ;
    FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const ;
    OperatorID get_global_operator_id(OperatorID id) const ;

    int get_num_axioms() const ;

    int get_num_goals() const ;
    FactPair get_goal_fact(int index) const ;

    bool test_goal(const GlobalState &state) const;

    std::vector<int> get_initial_state_data() const {
        return g_initial_state_data;
    }


    int get_goal_value(int var) const {
        for (const auto & goal : g_goal) {
            if (goal.first == var) {
                return goal.second;
            }
        }
        return -1;
    }

    void dump_goal() const;
    void dump_everything() const;

    void check_fact(int var, int val) const;

    bool has_conditional_effects() const;
    int get_first_conditional_effects_op_id() const;
    bool has_axioms() const;
    bool is_unit_cost()  const;

    // TODO: these methods should live somewhere else
    void verify_no_axioms_no_conditional_effects() const;
    void verify_no_conditional_effects() const ;
    void verify_no_axioms() const;

    void save_plan(const std::vector<int> & plan, const std::string & filename) const;
    
};
}

#endif
