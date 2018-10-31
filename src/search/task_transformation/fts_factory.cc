#include "fts_factory.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "merge_and_shrink_representation.h"
#include "types.h"

#include "../task_representation/labels.h"
#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/transition_system.h"

#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace task_representation;

namespace task_transformation {
class FTSFactory {
    const SASTask &sas_task;

    struct TransitionSystemData {
        // The following two attributes are only used for statistics
        int num_variables;
        vector<int> incorporated_variables;

        unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;
        vector<vector<Transition>> transitions_by_label;
        vector<bool> relevant_labels;
        int num_states;
        vector<bool> goal_states;
        int init_state;
        TransitionSystemData(TransitionSystemData &&other)
            : num_variables(other.num_variables),
              incorporated_variables(move(other.incorporated_variables)),
              label_equivalence_relation(move(other.label_equivalence_relation)),
              transitions_by_label(move(other.transitions_by_label)),
              relevant_labels(move(other.relevant_labels)),
              num_states(other.num_states),
              goal_states(move(other.goal_states)),
              init_state(other.init_state) {
        }
        TransitionSystemData() = default;
        TransitionSystemData(TransitionSystemData &other) = delete;
        TransitionSystemData &operator=(TransitionSystemData &other) = delete;
    };
    vector<TransitionSystemData> transition_system_data_by_var;

    unique_ptr<Labels> create_labels();
    void build_label_equivalence_relation(LabelEquivalenceRelation &label_equivalence_relation);
    void build_state_data(int var_no);
    void initialize_transition_system_data(const Labels &labels);
    void add_transition(int var_no, int label_no,
                        int src_value, int dest_value);
    void build_transitions();
    vector<unique_ptr<TransitionSystem>> create_transition_systems();
    vector<unique_ptr<MergeAndShrinkRepresentation>> create_mas_representations();
    vector<unique_ptr<Distances>> create_distances(
        const vector<unique_ptr<TransitionSystem>> &transition_systems);
public:
    explicit FTSFactory(const SASTask &sas_task);

    /*
      Note: create() may only be called once. We don't worry about
      misuse because the class is only used internally in this file.
    */
    FactoredTransitionSystem create(
        const bool compute_init_distances,
        const bool compute_goal_distances,
        Verbosity verbosity);
};

FTSFactory::FTSFactory(const SASTask &sas_task) : sas_task(sas_task) {
}

unique_ptr<Labels> FTSFactory::create_labels() {
    vector<unique_ptr<Label>> result;
    vector<vector<int>> sas_op_indices_by_label;
    int num_ops = sas_task.get_num_operators();
    int max_num_labels = (num_ops ? num_ops * 2 - 1 : 0);
    result.reserve(max_num_labels);
    sas_op_indices_by_label.reserve(max_num_labels);
    for (int index = 0; index < num_ops; ++index) {
        result.push_back(utils::make_unique_ptr<Label>(sas_task.get_operator_cost(index, false)));
        sas_op_indices_by_label.push_back({index});
    }
    return utils::make_unique_ptr<Labels>(move(result), max_num_labels, move(sas_op_indices_by_label));
}

void FTSFactory::build_label_equivalence_relation(
    LabelEquivalenceRelation &label_equivalence_relation) {
    /*
      Prepare label_equivalence_relation data structure: add one single-element
      group for every operator.
    */
    int num_labels = sas_task.get_num_operators();
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        // We use the label number as index for transitions of groups.
        label_equivalence_relation.add_label_group({LabelID(label_no)});
    }
}

void FTSFactory::build_state_data(int var_no) {
    TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
    ts_data.init_state = sas_task.get_initial_state_data()[var_no];

    int range = sas_task.get_variable_domain_size(var_no);
    ts_data.num_states = range;

    int goal_val = sas_task.get_goal_value(var_no);
    if (goal_val == -1){
        ts_data.goal_states.resize(range, true);
    } else {
        ts_data.goal_states.resize(range, false);
        ts_data.goal_states[goal_val] = true;
    }
}

void FTSFactory::initialize_transition_system_data(const Labels &labels) {
    int num_variables = sas_task.get_num_variables();
    int num_labels = sas_task.get_num_operators();
    transition_system_data_by_var.resize(num_variables);
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
        ts_data.num_variables = num_variables;
        ts_data.incorporated_variables.push_back(var_no);
        ts_data.label_equivalence_relation = utils::make_unique_ptr<LabelEquivalenceRelation>(labels);
        build_label_equivalence_relation(*ts_data.label_equivalence_relation);
        ts_data.transitions_by_label.resize(labels.get_max_size());
        ts_data.relevant_labels.resize(num_labels, false);
        build_state_data(var_no);
    }
}

void FTSFactory::add_transition(int var_no, int label_no,
                                int src_value, int dest_value) {
    transition_system_data_by_var[var_no].transitions_by_label[label_no].push_back(
        Transition(src_value, dest_value));
}

void FTSFactory::build_transitions() {
    int num_variables = sas_task.get_num_variables();
    int num_labels = sas_task.get_num_operators();
    /*
      - Add all transitions.
      - Computes relevant operator information as a side effect.
    */
    for (int label_no = 0; label_no < sas_task.get_num_operators(); ++label_no) {
        const auto & op = sas_task.get_operator(label_no);
        /*
          - Mark op as relevant in the transition systems corresponding
          to variables on which it has a precondition or effect.
          - Add transitions induced by op in these transition systems.
        */
        unordered_map<int, int> pre_val;
        for (const auto & precondition : op.get_preconditions()) {
            pre_val[precondition.var] = precondition.val;
        }
        vector <bool> has_effect_on_var(sas_task.get_num_variables(), false);

        for (const SASEffect & effect : op.get_effects()) {
            int var_no = effect.var;
            has_effect_on_var[var_no] = true;
            int post_value = effect.val;

            // Determine possible values that var can have when this
            // operator is applicable.
            int pre_value = -1;
            auto pre_val_it = pre_val.find(var_no);
            if (pre_val_it != pre_val.end())
                pre_value = pre_val_it->second;
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = sas_task.get_variable_domain_size(var_no);
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }

            /*
              cond_effect_pre_value == x means that the effect has an
              effect condition "var == x".
              cond_effect_pre_value == -1 means no effect condition on var.
              has_other_effect_cond is true iff there exists an effect
              condition on a variable other than var.
            */
            int cond_effect_pre_value = -1;
            bool has_other_effect_cond = false;
            for (const auto & condition : effect.conditions) {
                if (condition.var == var_no) {
                    cond_effect_pre_value = condition.val;
                } else {
                    has_other_effect_cond = true;
                }
            }

            // Handle transitions that occur when the effect triggers.
            for (int value = pre_value_min; value < pre_value_max; ++value) {
                /*
                  Only add a transition if it is possible that the effect
                  triggers. We can rule out that the effect triggers if it has
                  a condition on var and this condition is not satisfied.
                */
                if (cond_effect_pre_value == -1 || cond_effect_pre_value == value)
                    add_transition(var_no, label_no, value, post_value);
            }

            // Handle transitions that occur when the effect does not trigger.
            if (!effect.conditions.empty()) {
                for (int value = pre_value_min; value < pre_value_max; ++value) {
                    /*
                      Add self-loop if the effect might not trigger.
                      If the effect has a condition on another variable, then
                      it can fail to trigger no matter which value var has.
                      If it only has a condition on var, then the effect
                      fails to trigger if this condition is false.
                    */
                    if (has_other_effect_cond || value != cond_effect_pre_value)
                        add_transition(var_no, label_no, value, value);
                }
            }
            transition_system_data_by_var[var_no].relevant_labels[label_no] = true;
        }

        /*
          We must handle preconditions *after* effects because handling
          the effects sets has_effect_on_var.
        */
        for (const auto & precondition : op.get_preconditions()) {
            int var_no = precondition.var;
            if (!has_effect_on_var[var_no]) {
                int value = precondition.val;
                add_transition(var_no, label_no, value, value);
                transition_system_data_by_var[var_no].relevant_labels[label_no] = true;
            }
        }
    }

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        int num_states = sas_task.get_variable_domain_size(var_no);

        // Make all irrelevant labels explicit.
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            if (!transition_system_data_by_var[var_no].relevant_labels[label_no]) {
                for (int state = 0; state < num_states; ++state) {
                    add_transition(var_no, label_no, state, state);
                }
            }
        }
    }

    if (sas_task.has_conditional_effects()) {
        /*
          TODO: Our method for generating transitions is only guarantueed to generate
          sorted and unique transitions if the task has no conditional effects. We could
          replace the instance variable by a call to has_conditional_effects(task_proxy).
          Generally, the questions is whether we rely on sorted transitions anyway.
        */
        for (int var_no = 0; var_no < num_variables; ++var_no) {
            vector<vector<Transition>> &transitions_by_label =
                transition_system_data_by_var[var_no].transitions_by_label;
            for (vector<Transition> &transitions : transitions_by_label) {
                sort(transitions.begin(), transitions.end());
                transitions.erase(unique(transitions.begin(),
                                         transitions.end()),
                                  transitions.end());
            }
        }
    }
}

vector<unique_ptr<TransitionSystem>> FTSFactory::create_transition_systems() {
    // Create the actual TransitionSystem objects.
    int num_variables = sas_task.get_num_variables();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<TransitionSystem>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    const bool compute_label_equivalence_relation = true;
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
        result.push_back(utils::make_unique_ptr<TransitionSystem>(
                             ts_data.num_variables,
                             move(ts_data.incorporated_variables),
                             move(ts_data.label_equivalence_relation),
                             move(ts_data.transitions_by_label),
                             ts_data.num_states,
                             move(ts_data.goal_states),
                             ts_data.init_state,
                             compute_label_equivalence_relation
                             ));
    }
    return result;
}

vector<unique_ptr<MergeAndShrinkRepresentation>> FTSFactory::create_mas_representations() {
    // Create the actual MergeAndShrinkRepresentation objects.
    int num_variables = sas_task.get_num_variables();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<MergeAndShrinkRepresentation>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        int range = sas_task.get_variable_domain_size(var_no);
        result.push_back(
            utils::make_unique_ptr<MergeAndShrinkRepresentationLeaf>(var_no, range));
    }
    return result;
}

vector<unique_ptr<Distances>> FTSFactory::create_distances(
    const vector<unique_ptr<TransitionSystem>> &transition_systems) {
    // Create the actual Distances objects.
    int num_variables = sas_task.get_num_variables();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<Distances>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        result.push_back(
            utils::make_unique_ptr<Distances>(*transition_systems[var_no]));
    }
    return result;
}

FactoredTransitionSystem FTSFactory::create(
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    if (verbosity >= Verbosity::NORMAL) {
        cout << "Building atomic transition systems... " << endl;
    }

    unique_ptr<Labels> labels = create_labels();

    initialize_transition_system_data(*labels);
    build_transitions();
    vector<unique_ptr<TransitionSystem>> transition_systems =
        create_transition_systems();
    vector<unique_ptr<MergeAndShrinkRepresentation>> mas_representations =
        create_mas_representations();
    vector<unique_ptr<Distances>> distances =
        create_distances(transition_systems);

    return FactoredTransitionSystem(
        move(labels),
        move(transition_systems),
        move(mas_representations),
        move(distances),
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}

FactoredTransitionSystem create_factored_transition_system(
    const SASTask &sas_task,     
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    return FTSFactory(sas_task).create(
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}
}
