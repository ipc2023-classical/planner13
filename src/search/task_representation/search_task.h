#ifndef TASK_REPRESENTATION_SEARCH_TASK_H
#define TASK_REPRESENTATION_SEARCH_TASK_H

#include <memory>
#include <set>

#include <boost/dynamic_bitset.hpp>

#include "../axioms.h"
#include "../operator_id.h"

namespace int_packer {
    class IntPacker;
}

namespace task_representation {
struct Precondition {
    int variable;
    std::set<int> values;
};

struct Effect {
    int variable;
    int value;
    Effect (int var, int val) : variable(var), value(val){
    }
};

// Each operator is associated with a label plus a set of target effects in each TS
// whenever there are non self-loop transitions

struct AbstractOperator {
    int label;
    int cost;

    std::vector<Precondition> preconditions;
    std::vector<Effect> effects;
    AbstractOperator(int label_,
                     int cost_,
                     std::vector<Effect> effects_) : label(label_), cost(cost_), effects(effects_) {
                     }
};

class State;

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

    OperatorTree (const FTSTask & fts_task, std::vector<AbstractOperator> & operators);

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
    std::unique_ptr<int_packer::IntPacker> state_packer;
    AxiomEvaluator axiom_evaluator;
    /* std::unique_ptr<SuccessorGenerator> successor_generator; */

    std::vector<int> initial_state;

    std::vector<AbstractOperator> operators;
    std::vector<std::vector<OperatorID> > operators_by_label;
    std::vector<std::vector<boost::dynamic_bitset<> > > activated_labels_by_state;
    //std::vector<OperatorTree> operator_tree;

    std::vector<int> goal_relevant_transition_systems;

    const int min_operator_cost;

    //Generate all the combinations of targets for every variable
    void create_operators(int label_no, const FTSTask & fts_task,
                          std::vector<AbstractOperator> & operators,
                          std::vector<Effect> & assignments, int var = 0) ;
public:
    SearchTask(const FTSTask & fts_task);

    bool is_goal_state(const GlobalState &state) const;

    void apply_operator(OperatorID op, PackedStateBin *buffer) ;

    void generate_applicable_ops(const GlobalState &state,
                                 std::vector<OperatorID> &applicable_ops) const;

    bool is_applicable(const GlobalState & state, OperatorID op) const;

    int num_variables() const {
        return initial_state.size();
    }

    const std::vector<int> & get_initial_state_data() const {
        return initial_state;
    }
    const int_packer::IntPacker * get_state_packer() const {
        return state_packer.get();
    }

    AxiomEvaluator & get_axiom_evaluator() {
        return axiom_evaluator;
    }

    int get_operator_cost(OperatorID op) const {
        return operators[op.get_index()].cost;
    }


    int get_min_operator_cost() const {
        return min_operator_cost;
    }

};
}

#endif
