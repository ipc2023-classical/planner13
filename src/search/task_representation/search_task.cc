#include "search_task.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include "../algorithms/int_packer.h"

#include<map>

using namespace std;

namespace task_representation {
// OperatorTree::OperatorTree (const FTSTask & fts_task,
//                             std::vector<AbstractOperator> & operators) {
//     map<OperatorTreeNode, int> cache;
//     int num_labels = fts_task.get_num_labels();
//     for (int label_no = 0; label_no < num_labels; ++label_no) {
//         std::vector<AbstractOperator> new_operators;
//         root_per_label[label_no] = construct_tree (label_no, fts_task, operators, operators_by_label, cache, new_operators, 0);
//     }
// }

// void OperatorTree::get_operator_ids(const GlobalState & state, int label, std::vector<OperatorID> & operators) const {
//     int node_index = root_per_label[label];
//     while(nodes[node_index].variable >= 0) {
//         const int value = state[nodes[node_index].variable];
//         node_index = nodes[node_index].index_operators[value];
//         assert(node_index != 0);
//     }
//     return nodes[node_index].index_operators;
// }


SearchTask::SearchTask(const FTSTask & fts_task) :
    state_packer(std::move(fts_task.get_state_packer())),
    axiom_evaluator (fts_task),
    min_operator_cost (fts_task.get_min_operator_cost()) {

    std::vector<AbstractOperator> operators;
    std::vector<std::vector<OperatorID> > operators_by_label;

    size_t num_variables = fts_task.get_size();
    int num_labels = fts_task.get_num_labels();
    activated_labels_by_state.resize(num_variables);
    for(size_t var = 0; var < num_variables; ++var) {
        const TransitionSystem & ts = fts_task.get_ts(var);
        activated_labels_by_state[var].resize(ts.get_size(), boost::dynamic_bitset<>(num_labels, false));
        for (const auto & group_and_transitions : ts) {
            boost::dynamic_bitset<> bs (num_labels, false);
            for (int label : group_and_transitions.label_group) {
                bs.set(label);
            }
            for (const auto & transition :  group_and_transitions.transitions) {
                activated_labels_by_state[var][transition.src] |= bs;
            }
        }
    }

    for (size_t index = 0; index < num_variables; ++index) {
        const TransitionSystem & ts = fts_task.get_ts(index);

        if(ts.is_goal_relevant()) {
            goal_relevant_transition_systems.push_back(index);
        }
    }

    }

// bool SearchTask::is_goal_state(const GlobalState &state) const {
//     for (int i : goal_relevant_transition_systems) {
//         if (!transition_systems[i]->is_goal_state(state[i])) {
//             return false;
//         }
//     }
//     return true;
// }

//Generate all the combinations of targets for every variable
void SearchTask::create_operators(int label_no, const FTSTask & task,
                                  std::vector<AbstractOperator> & operators,
                                  vector<Effect> & assignments, int var) {

    (void) label_no;
    (void) task;
    (void) operators;
    (void) assignments;
    (void) var;
     // int num_variables = task.get_size();

    // //Skip irrelevant variables
    // while(var < num_variables && !task.only_has_self_loops(var, label_no) ) {
    //         var++;
    // }

    // if (var == num_variables) {
    //         operators_by_label[label_no].push_back(OperatorID(operators.size()));
    //         operators.push_back(AbstractOperator(label_no, task.get_label_cost(label_no), assignments));
    // } else {
    //         const auto & ts = task.get_ts(var);
    //         const auto & targets = ts.get_targets(label_no);
    //         assert(targets.size() > 0);
    //         for (int target : targets) {
    //             assignments.push_back(Effect(var, target));
    //             create_operators(label_no, task, operators, assignments, var + 1);
    //             assignments.pop_back();
    //         }
    // }
}


void SearchTask::apply_operator(OperatorID op_id, PackedStateBin *buffer) {
    for (const auto & effect : operators[op_id.get_index()].effects) {
        state_packer->set(buffer, effect.variable, effect.value);
    }
    axiom_evaluator.evaluate(buffer, *state_packer);
}


// void SearchTask::generate_applicable_ops(const State &state,
//                                          std::vector<OperatorID> &applicable_ops) const {
//}

void SearchTask::generate_applicable_ops(
    const GlobalState &state, std::vector<OperatorID> &applicable_ops) const {

    auto activated_labels = activated_labels_by_state[0][state[(size_t)0]];

    for(size_t var = 1; var < activated_labels_by_state.size(); ++var) {
        activated_labels &= activated_labels_by_state[var][state[var]];
    }

    applicable_ops.push_back(operators_by_label[0][0]);
    // for(size_t label = 0; label < activated_labels.size(); ++label) {
    //         if(activated_labels[label]) {
    //             for (int var : relevant_vars_for_label[label]) {
    //                 applicable_ops.push_back(operators_by_label[xxx]);
    //             }
    //         }
    // }

}



bool SearchTask::is_applicable(const GlobalState & state, OperatorID op) const {
    for(const auto & pre : operators[op.get_index()].preconditions){
        if(!pre.values.count(state[pre.variable])) {
            return false;
        }
    }
    return true;
}
}
