#include "fts_task.h"

#include "distances.h"
#include "state.h"
#include "labels.h"
#include "transition_system.h"

#include "../utils/collections.h"
#include "../utils/memory.h"
#include "../utils/system.h"
#include "../algorithms/int_packer.h"

#include "search_task.h"
#include "sas_task.h"
#include "sas_operator.h"

#include "label_equivalence_relation.h"

#include <cassert>

using namespace std;



namespace task_representation {
//Auxiliary classes for construction of the initial FTS
class TransitionSystemData {
public:
    // The following two attributes are only used for statistics
    int num_variables;
    std::vector<int> incorporated_variables;

    std::unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;
    std::vector<std::vector<Transition>> transitions_by_label;
    std::vector<bool> relevant_labels;
    int num_states;
    std::vector<bool> goal_states;
    int init_state;


    void add_transition(int label_no, int src_value, int dest_value) {
        transitions_by_label[label_no].push_back(Transition(src_value, dest_value));
    }
};

int FTSTask::get_label_cost(LabelID label) const {
    return labels->get_label_cost(label);
}

int FTSTask::get_num_labels() const {
    return labels->get_size();
}

int FTSTask::get_min_operator_cost() const {
    return labels->get_min_operator_cost();
}

FTSTask::FTSTask(const SASTask & sas_task) : labels (utils::make_unique_ptr<Labels>(sas_task)) {
    cout << "Building atomic transition systems... " << endl;



    const int num_variables = sas_task.get_num_variables();
    const int num_labels = labels->get_size();

    vector<TransitionSystemData> transition_system_data_by_var (num_variables);


    for (int var_no = 0; var_no < num_variables; ++var_no) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
        ts_data.num_variables = num_variables;
        ts_data.incorporated_variables.push_back(var_no);
        ts_data.label_equivalence_relation = utils::make_unique_ptr<LabelEquivalenceRelation>(*labels);
        /*
          Prepare label_equivalence_relation data structure: add one single-element
          group for every operator.
        */
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            // We use the label number as index for transitions of groups.
            ts_data.label_equivalence_relation->add_label_group({LabelID(label_no)});
        }

        ts_data.transitions_by_label.resize(num_labels);
        ts_data.relevant_labels.resize(num_labels, false);

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
                    transition_system_data_by_var[var_no].add_transition(label_no, value, post_value);
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
                        transition_system_data_by_var[var_no].add_transition(label_no, value, value);
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
                transition_system_data_by_var[var_no].add_transition(label_no, value, value);
                transition_system_data_by_var[var_no].relevant_labels[label_no] = true;
            }
        }
    }

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        int num_states = sas_task.get_variable_domain_size(var_no);

        // Make all irrelevant labels explicit.
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            if (!transition_system_data_by_var[var_no].relevant_labels[label_no]) {
                for (int state = 0; state < num_states; ++state)
                    transition_system_data_by_var[var_no].add_transition(label_no, state, state);
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

    constexpr bool compute_label_equivalence_relation = true;
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        TransitionSystemData &ts_data = transition_system_data_by_var[var_no];
        transition_systems.push_back(utils::make_unique_ptr<TransitionSystem>(
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

}

FTSTask::~FTSTask() {
    labels = nullptr;
    for (auto &transition_system : transition_systems) {
        transition_system = nullptr;
    }
}

bool FTSTask::is_component_valid(int index) const {
    return transition_systems[index]->are_transitions_sorted_unique();
}

void FTSTask::assert_all_components_valid() const {
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        assert (transition_systems[index]);
        assert(is_component_valid(index));
    }
}



std::string FTSTask::get_fact_name(const FactPair & fp) const {
    return "fact" + std::to_string(fp.var) + "-" + std::to_string(fp.value);
}


bool FTSTask::are_facts_mutex(const FactPair & , const FactPair & ) const {
    cerr <<  "FTSTask::are_facts_mutex not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
    return false;
}

std::vector<int> FTSTask::get_initial_state_data() const {
    cerr <<  "FTSTask::get_initial_state_data not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
    return vector<int> ();
}

State FTSTask::get_initial_state() const {
    cerr <<  "FTSTask::get_initial_state not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
    return State(*this, vector<int>());
}

bool FTSTask::is_goal_state (const GlobalState & /*state*/) const {
    cerr <<  "FTSTask::is_goal_state not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
    return false;
}


std::shared_ptr<SearchTask> FTSTask::get_search_task() const {
    if (!search_task) {
        search_task = make_shared<SearchTask> (*this);
    }
    return search_task;
}

std::unique_ptr<int_packer::IntPacker> FTSTask::get_state_packer() const {
    std::vector<int> sizes;
    sizes.reserve(transition_systems.size());
    for (const auto & tr : transition_systems) {
        sizes.push_back(tr->get_size());
    }
    return utils::make_unique_ptr<int_packer::IntPacker>(sizes);
}
}
