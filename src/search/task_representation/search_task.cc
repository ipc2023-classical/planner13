#include "search_task.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include "../algorithms/int_packer.h"

#include <map>
#include <unordered_map>

using namespace std;

namespace task_representation {
// OperatorTree::OperatorTree (const FTSTask & fts_task,
//                             vector<AbstractOperator> & operators) {
//     map<OperatorTreeNode, int> cache;
//     int num_labels = fts_task.get_num_labels();
//     for (int label_no = 0; label_no < num_labels; ++label_no) {
//         vector<AbstractOperator> new_operators;
//         root_per_label[label_no] = construct_tree (label_no, fts_task, operators, operators_by_label, cache, new_operators, 0);
//     }
// }

// void OperatorTree::get_operator_ids(const GlobalState & state, int label, vector<OperatorID> & operators) const {
//     int node_index = root_per_label[label];
//     while(nodes[node_index].variable >= 0) {
//         const int value = state[nodes[node_index].variable];
//         node_index = nodes[node_index].index_operators[value];
//         assert(node_index != 0);
//     }
//     return nodes[node_index].index_operators;
// }


SearchTask::SearchTask(const FTSTask &fts_task) :
    fts_task(fts_task),
    state_packer(move(fts_task.get_state_packer())),
//    axiom_evaluator (fts_task),
    min_operator_cost (fts_task.get_min_operator_cost()) {

    /*
      TODO: we currently assume that there are no gaps in the transition
      systems of FTSTask, which for normal M&S is not true.
    */

    create_fts_operators();

    size_t num_variables = fts_task.get_size();
    int num_labels = fts_task.get_num_labels();
    activated_labels_by_state.resize(num_variables);
    for (size_t var = 0; var < num_variables; ++var) {
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
        if (ts.is_goal_relevant()) {
            goal_relevant_transition_systems.push_back(index);
        }
    }
}

bool SearchTask::is_label_group_relevant(
    int num_states, const vector<Transition> &transitions) {
    if (static_cast<int>(transitions.size()) == num_states) {
        /*
          A label group is irrelevant it has exactly a self-loop transition
          for every state.
        */
        for (const Transition &transition : transitions) {
            if (transition.target != transition.src) {
                return true;
            }
        }
    } else {
        return true;
    }
    return false;
}

bool SearchTask::are_transitions_deterministic(const vector<Transition> &transitions) {
    set<int> sources;
    for (const Transition &t : transitions) {
        if (sources.count(t.src)) {
            return false;
        } else {
            sources.insert(t.src);
        }
    }
    return true;
}

void SearchTask::multiply_out_non_deterministic_labels(
        LabelID label_id,
        const vector<vector<int>> &targets_by_ts_index,
        int ts_index,
        vector<FactPair> &effects) {
    const vector<int> &non_det_ts = label_to_info[label_id].relevant_non_deterministic_transition_systems;
    if (ts_index == static_cast<int>(non_det_ts.size())) {
        OperatorID op_id(operators.size());
        operators.emplace_back(op_id, label_id, fts_task.get_label_cost(label_id), effects);
        operators_by_label[label_id].push_back(op_id);
        label_to_info[label_id].fts_operators.push_back(op_id);
        return;
    }
    const vector<int> &targets = targets_by_ts_index[ts_index];
    int var = label_to_info[label_id].relevant_non_deterministic_transition_systems[ts_index];
    for (int target : targets) {
        effects.emplace_back(var, target);
        multiply_out_non_deterministic_labels(label_id, targets_by_ts_index, ts_index + 1, effects);
        effects.pop_back();
    }
}

void SearchTask::create_fts_operators() {
    size_t num_variables = fts_task.get_size();
    int num_labels = fts_task.get_num_labels();
    // Set of targets (no duplicates) of transitions indexed by labels and by
    // the indices of the non-deterministic transition systems of that label.
    vector<vector<vector<int>>> targets_by_label_by_ts_index(num_labels);
    for (size_t var = 0; var < num_variables; ++var) {
        const TransitionSystem & ts = fts_task.get_ts(var);
        for (const GroupAndTransitions &gat : ts) {
            const LabelGroup &label_group = gat.label_group;
            const vector<Transition> &transitions = gat.transitions;
            if (is_label_group_relevant(ts.get_size(), transitions)) {
                bool deterministic = are_transitions_deterministic(transitions);
                if (deterministic) {
                    unordered_map<int, int> src_to_target;
                    for (const Transition &t : transitions) {
                        assert(!src_to_target.count(t.src));
                        src_to_target[t.src] = t.target;
                    }
                    for (LabelID label_id : label_group) {
                        label_to_info[label_id].relevant_deterministic_transition_systems.push_back(var);
                        label_to_info[label_id].src_to_target_by_ts_index.push_back(src_to_target);
                    }
                } else {
                    set<int> targets;
                    for (const Transition &t : transitions) {
                        targets.insert(t.target);
                    }
                    for (LabelID label_id : label_group) {
                        label_to_info[label_id].relevant_non_deterministic_transition_systems.push_back(var);
                        targets_by_label_by_ts_index[label_id].emplace_back(targets.begin(), targets.end());
                    }
                }
            }
        }
    }

    operators_by_label.resize(num_labels);
    for (LabelID label_id(0); label_id < num_labels; ++label_id) {
        vector<FactPair> effects;
        multiply_out_non_deterministic_labels(label_id, targets_by_label_by_ts_index[label_id], 0, effects);
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

void SearchTask::apply_operator(OperatorID op_id, PackedStateBin *buffer) {
    for (const FactPair &effect : operators[op_id.get_index()].get_effects()) {
        state_packer->set(buffer, effect.var, effect.value);
    }
//    axiom_evaluator.evaluate(buffer, *state_packer);
}

void SearchTask::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {

    // TODO: implement this

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

bool SearchTask::is_applicable(const GlobalState &state, OperatorID op) const {
    LabelID label = operators[op.get_index()].get_label();
    for (int var = 0; var < fts_task.get_size(); ++var) {
        int value = state[var];
        if (!activated_labels_by_state[var][value].test(label)) {
            return false;
        }
    }
    return true;
}

int SearchTask::get_operator_cost(OperatorID op) const {
    return fts_task.get_label_cost(operators[op.get_index()].get_label());
}
}
