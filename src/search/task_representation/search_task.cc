#include "search_task.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include "../algorithms/int_packer.h"

#include "../utils/memory.h"

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

std::unique_ptr<int_packer::IntPacker> compute_state_packer(const FTSTask &fts_task) {
    std::vector<int> sizes;
    sizes.reserve(fts_task.get_size());
    for (int ts_index = 0; ts_index < fts_task.get_size(); ++ts_index) {
        const TransitionSystem &ts = fts_task.get_ts(ts_index);
        sizes.push_back(ts.get_size());
    }
    return utils::make_unique_ptr<int_packer::IntPacker>(sizes);
}


SearchTask::SearchTask(const FTSTask &fts_task) :
    fts_task(fts_task),
    state_packer(move(compute_state_packer(fts_task))),
//    axiom_evaluator (fts_task),
    min_operator_cost (fts_task.get_min_operator_cost()) {

    /*
      TODO: we currently assume that there are no gaps in the transition
      systems of FTSTask, which for normal M&S is not true.
    */

    create_fts_operators();

    size_t num_variables = fts_task.get_size();
    int num_labels = fts_task.get_num_labels();
    activated_labels_by_var_by_state.resize(num_variables);
    for (size_t var = 0; var < num_variables; ++var) {
        const TransitionSystem & ts = fts_task.get_ts(var);
        activated_labels_by_var_by_state[var].resize(ts.get_size(), boost::dynamic_bitset<>(num_labels, false));
        for (const auto & group_and_transitions : ts) {
            boost::dynamic_bitset<> bs (num_labels, false);
            for (int label : group_and_transitions.label_group) {
                bs.set(label);
            }
            for (const auto & transition :  group_and_transitions.transitions) {
                activated_labels_by_var_by_state[var][transition.src] |= bs;
            }
        }
    }

    initial_state.reserve(num_variables);
    for (size_t index = 0; index < num_variables; ++index) {
        const TransitionSystem & ts = fts_task.get_ts(index);
        if (ts.is_goal_relevant()) {
            goal_relevant_transition_systems.push_back(index);
        }
        initial_state.push_back(ts.get_init_state());
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

/*
  If there are no relevant non-deterministic transition systems for the label,
  then this method adds a single FTSOperator with empty effects.
*/
void SearchTask::multiply_out_non_deterministic_labels(
        LabelID label_id,
        const vector<vector<int>> &targets_by_ts_index,
        int ts_index,
        vector<FactPair> &effects) {
    const vector<int> &non_det_ts = label_to_info[label_id].relevant_non_deterministic_transition_systems;
    if (ts_index == static_cast<int>(non_det_ts.size())) {
        OperatorID op_id(operators.size());
        operators.emplace_back(op_id, label_id, fts_task.get_label_cost(label_id), effects);
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
    label_to_info.resize(num_labels);
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

    /*
      Compute the FTSOperators for all labels by multiplying out the
      combinations of possible target states in all relevant non-deterministic
      transition systems. For the relevant deterministic transition systems,
      add one "dummy" FTSOperator to have an ID for the label.

      Furthermore, for all relevant non-deterministic transition systems of
      a label, compute bitsets indexed by their states that have bits sets to
      true if the corresponding FTSOperator of the label is applicable.
    */
    for (LabelID label_id(0); label_id < num_labels; ++label_id) {
        vector<FactPair> effects;
        multiply_out_non_deterministic_labels(label_id, targets_by_label_by_ts_index[label_id], 0, effects);

        const vector<int> &relevant_non_det_ts = label_to_info[label_id].relevant_non_deterministic_transition_systems;
        const vector<OperatorID> &fts_operators = label_to_info[label_id].fts_operators;
        vector<vector<boost::dynamic_bitset<>>> &applicable_fts_ops_by_ts_index_by_state = label_to_info[label_id].applicable_ops_by_ts_index_by_state;
        applicable_fts_ops_by_ts_index_by_state.resize(relevant_non_det_ts.size());
        for (size_t ts_index = 0; ts_index < relevant_non_det_ts.size(); ++ ts_index) {
            int var = relevant_non_det_ts[ts_index];
            const TransitionSystem &ts = fts_task.get_ts(var);
            applicable_fts_ops_by_ts_index_by_state[ts_index].resize(ts.get_size(), boost::dynamic_bitset<>(fts_operators.size(), false));

            /*
              For each FTS operator and state of the TS, check if the label
              corresponding to the fts operator is activated. If so, set the
              FTS operator to be applicable.
            */
            for (size_t op_index = 0; op_index < fts_operators.size(); ++op_index) {
                LabelID label = operators[fts_operators[op_index].get_index()].get_label();
                for (int value = 0; value < ts.get_size(); ++value) {
                    if (activated_labels_by_var_by_state[var][value].test(label)) {
                        applicable_fts_ops_by_ts_index_by_state[ts_index][value].set(op_index);
                    }
                }
            }
        }
    }
}

 bool SearchTask::is_goal_state(const GlobalState &state) const {
     // TODO: should use our own method in the long term
     return fts_task.is_goal_state(state);
 }

// We only need to predecessor to access certain varaiables' values.
void SearchTask::apply_operator(
    const GlobalState &predecessor, OperatorID op_id, PackedStateBin *buffer) {
    const FTSOperator &fts_op = operators[op_id.get_index()];

    // Effects on deterministic TS
    LabelID label = fts_op.get_label();
    const vector<int> &det_ts = label_to_info[label].relevant_deterministic_transition_systems;
    const vector<unordered_map<int, int>> &src_to_target_by_ts_index = label_to_info[label].src_to_target_by_ts_index;
    for (size_t ts_index = 0; ts_index < det_ts.size(); ++ts_index) {
        int var = det_ts[ts_index];
        const unordered_map<int, int> &src_to_target = src_to_target_by_ts_index[ts_index];
        state_packer->set(buffer, var, src_to_target.at(predecessor[var]));
    }

    // Effects on non-deterministic TS
    for (const FactPair &effect : fts_op.get_effects()) {
        state_packer->set(buffer, effect.var, effect.value);
    }
//    axiom_evaluator.evaluate(buffer, *state_packer);
}

void SearchTask::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    size_t var = 0;
    boost::dynamic_bitset<> activated_labels = activated_labels_by_var_by_state[var][state[var]];
    for (var = 1; var < activated_labels_by_var_by_state.size(); ++var) {
        activated_labels &= activated_labels_by_var_by_state[var][state[var]];
    }

    for (LabelID label(0); label < static_cast<int>(activated_labels.size()); ++label) {
        if (activated_labels[label]) {
            const vector<OperatorID> &label_operators = label_to_info[label].fts_operators;
            if (label_operators.size() == 1) {
                OperatorID op_id = label_operators[0];
                assert(operators[op_id.get_index()].get_effects().empty());
                applicable_ops.push_back(op_id);
            } else {
                const vector<int> &non_det_ts =
                    label_to_info[label].relevant_non_deterministic_transition_systems;
                assert(!non_det_ts.empty());
                const vector<vector<boost::dynamic_bitset<>>>
                    &applicable_fts_ops_by_ts_index_by_state =
                        label_to_info[label].applicable_ops_by_ts_index_by_state;

                size_t ts_index = 0;
                int var = non_det_ts[ts_index];
                boost::dynamic_bitset<> applicable_fts_ops =  applicable_fts_ops_by_ts_index_by_state[ts_index][state[var]];
                for (ts_index = 1;
                     ts_index < applicable_fts_ops_by_ts_index_by_state.size(); ++ts_index) {
                    var = non_det_ts[ts_index];
                    applicable_fts_ops &=
                        applicable_fts_ops_by_ts_index_by_state[ts_index][state[var]];
                }

                for (boost::dynamic_bitset<>::size_type op_index = 0;
                     op_index < applicable_fts_ops.size(); ++op_index) {
                    if (applicable_fts_ops[op_index]) {
                        applicable_ops.push_back(label_operators[op_index]);
                    }
                }

            }
        }
    }
}

bool SearchTask::is_applicable(const GlobalState &state, OperatorID op) const {
    LabelID label = operators[op.get_index()].get_label();
    for (int var = 0; var < fts_task.get_size(); ++var) {
        int value = state[var];
        if (!activated_labels_by_var_by_state[var][value].test(label)) {
            return false;
        }
    }
    return true;
}

int SearchTask::get_operator_cost(OperatorID op) const {
    return fts_task.get_label_cost(operators[op.get_index()].get_label());
}

shared_ptr<SearchTask> get_search_task(const shared_ptr<FTSTask> &fts_task) {
    static shared_ptr<SearchTask> search_task = make_shared<SearchTask>(*fts_task);
    return search_task;
}
}
