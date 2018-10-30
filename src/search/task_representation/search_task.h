#ifndef TASK_REPRESENTATION_SEARCH_TASK_H
#define TASK_REPRESENTATION_SEARCH_TASK_H

#include "fact.h"
#include "labels.h"

#include "../global_state.h"
#include "../operator_id.h"

#include <memory>
#include <set>
#include <unordered_map>

#include <boost/dynamic_bitset.hpp>

namespace int_packer {
    class IntPacker;
}

namespace task_representation {
class FTSOperator;
class FTSTask;
class State;
class Transition;

struct OperatorTreeNode {
    int variable;
    //If variable == -1, index_operators is
    std::vector<int> index_operators;

    OperatorTreeNode() : variable(-1){
    }
};

class OperatorTree {

    std::vector<OperatorTreeNode> nodes;
    std::vector<int> root_per_label;

//    OperatorTree (const FTSTask & fts_task, std::vector<AbstractOperator> & operators);

    /* int construct_tree(int label_no, const FTSTask & fts_task,  */
    /*                        std::vector<AbstractOperator> & operators, */
    /*                        map<OperatorTreeNode, int> & cache, */
    /*                        std::vector<AbstractOperator> & new_operators, int var = 0) { */
    /*         int num_variables = fts_task.get_size(); */

    /*         //Skip irrelevant variables */
    /*         while(var < num_variables && !fts.only_has_self_loops(var, label_no) ) { */
    /*             var++;  */
    /*         } */

    /*         OperatorTreeNode new_tree_node; */
    /*         if (var == num_variables) { */
    /*             // Base case: Add operators to vector of operators */
    /*             for(auto & newop : new_operators ) { */
    /*                 auto newopit = std::find(operators.begin(), operators.end(), newop); */
    /*                 int op_id; */
    /*                 if (newopit != operators.end()) { */
    /*                     op_id = newopit - operators.begin(); */
    /*                 } else { */
    /*                     op_id = operators.size(); */
    /*                     operators.push_back(newop); */
    /*                 } */
    /*                 new_tree_node.index_operators.push_back(op_id); */
    /*             }  */
    /*         } else { // Recursive case */
    /*             const auto & ts = fts.get_ts(var);             */
    /*             vector<int> targets = ts.get_targets(label_no); */
    /*             assert(targets.size() > 0); */
    /*             if (targets.size() == 1 && !ts.has_precondition(label_no)) { */
    /*                 for (auto & op : new_operators) { */
    /*                     op.add_effect(var, targets[0]); */
    /*                 } */
    /*                 return construct_tree(label_no, fts_task, operators, cache, new_operators, var + 1); */
    /*             } */
    /*             new_tree_node.variable = variable; */
    /*             std::vector<AbstractOperator> new_new_operators; */
    /*             for (int target : targets) {                 */
    /*                 for(auto newop : new_operators) { //Iterate making copy */
    /*                     newop.add_effect(var, target); */
    /*                     new_new_operators.push_back(newop); */
    /*                 } */
    /*             } */


    /*             const auto & sources =  fts.get_transitions_by_label_and_target(label_no, target); */
    /*             for (int source : sources) { */

    /*             } */

    /*         } */


    /*         nodes.push_back (new_tree_node); */
    /*         return nodes.size() -1; */
    /* } */

    void get_operator_ids(const GlobalState & state, int label, std::vector<OperatorID> & operators) const;
};

// Common functionality between SASTask and FTSTask for enabling search. We keep some
// information duplicated here (e.g. operator costs) to ensure that the access to the
// functions that are critical performance for the search is as fast as possible. Hence,
// we only allow for virtual functions that may be called at most once per search.
class SearchTask {
private:
    const FTSTask &fts_task;
    std::unique_ptr<int_packer::IntPacker> state_packer;
//    AxiomEvaluator axiom_evaluator;
    /* std::unique_ptr<SuccessorGenerator> successor_generator; */

    std::vector<int> initial_state;

    std::vector<FTSOperator> operators;
    std::vector<std::vector<boost::dynamic_bitset<>>> activated_labels_by_var_by_state;
    //std::vector<OperatorTree> operator_tree;

    struct LabelInformation {
        // The set of transition systems (ids) in which the label is relevant
        // and deterministic.
        std::vector<int> relevant_deterministic_transition_systems;
        // Mapping of source to target states of each relevant deterministic
        // TS. That is, this vector is indexed by indices of above vector and
        // *not* the id of the TS itself.
        std::vector<std::unordered_map<int, int>> src_to_target_by_ts_index;

        // The set of transition systems (ids) in which the label is relevant
        // and has at least one non-deterministic transition.
        std::vector<int> relevant_non_deterministic_transition_systems;
        // Set of FTS operators for the non-deterministic transition systems.
        std::vector<OperatorID> fts_operators;
        // For each of the above TS, indexed by the index of the vector,
        // store for each state a bitset (num bits equal to num operators in
        // fts_operators) with bits set to true if the corresponding operator
        // is applicable in the state.
        std::vector<std::vector<boost::dynamic_bitset<>>> applicable_ops_by_ts_index_by_state;
    };
    std::vector<LabelInformation> label_to_info;

    std::vector<int> goal_relevant_transition_systems;

    const int min_operator_cost;

    bool is_label_group_relevant(
        int num_states, const std::vector<Transition> &transitions);
    bool are_transitions_deterministic(const std::vector<Transition> &transitions);
    void multiply_out_non_deterministic_labels(
        LabelID label_id,
        const std::vector<std::vector<int>> &targets_by_ts_index,
        int pos,
        std::vector<FactPair> &effects);
    void create_fts_operators();
public:
    SearchTask(const FTSTask & fts_task);

    bool is_goal_state(const GlobalState &state) const;

    void apply_operator(
        const GlobalState &predecessor, OperatorID op, PackedStateBin *buffer);

    void generate_applicable_ops(
        const GlobalState &state,
        std::vector<OperatorID> &applicable_ops) const;

    bool is_applicable(const GlobalState & state, OperatorID op) const;

    int num_variables() const {
        return initial_state.size();
    }

    const std::vector<int> &get_initial_state_data() const {
        return initial_state;
    }
    const int_packer::IntPacker *get_state_packer() const {
        return state_packer.get();
    }

//    AxiomEvaluator & get_axiom_evaluator() {
//        return axiom_evaluator;
//    }

    int get_operator_cost(OperatorID op) const;

    int get_min_operator_cost() const {
        return min_operator_cost;
    }

    void dump_op(OperatorID op) const;
};

extern std::shared_ptr<SearchTask> get_search_task(
    const std::shared_ptr<FTSTask> &fts_task);
}

#endif
