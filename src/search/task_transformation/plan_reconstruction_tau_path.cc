
#include "plan_reconstruction_tau_path.h"

#include "tau_graph.h"

#include "../task_representation/transition_system.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/fact.h"

using namespace task_representation;
using namespace std;

namespace task_transformation { 
    
// Does s has a transition with label to some t whose abstraction is abstract_target?
bool TauShrinking::is_target(int s, int label, int abstract_target) const {
    for (const Transition & tr : transition_system->get_transitions_with_label(label) ) {
        if (tr.src == s &&  (abstract_target == -1 || abstraction[tr.target] == abstract_target)) {
            return true;
        }
    }
    return false;
}

void TauShrinking::apply_label_mapping(const LabelMapping & label_mapping) {
    transition_system->apply_label_mapping(label_mapping);
    tau_graph->apply_label_mapping(label_mapping);
}
void TauShrinking::
reconstruct_step(int source, const std::vector<bool> & targets,
                 std::vector<int> & new_label_path,
                 std::vector<PlanState> & new_traversed_states) const {
    // Use TauGraph to find a path from src to target
    std::vector<std::pair<task_representation::LabelID, int>> tau_path =
        tau_graph->find_shortest_path(source, targets);
    
    // Insert tau path into the plan
    for (const auto & step : tau_path) {
        new_label_path.push_back(step.first);
        auto new_state = new_traversed_states.back();
        new_state.set(ts_index_predecessor, step.second);
        new_traversed_states.push_back(new_state);
        // if (transition_system->get_incorporated_variables().size() == 1) {
        //     cout << g_sas_task()->get_fact_name(FactPair(transition_system->get_incorporated_variables()[0], new_state)) << endl;
        // }
    }
}


void TauShrinking::
reconstruct_step(int label, const PlanState & abstract_target_state,
                 std::vector<int> & new_label_path,
                 std::vector<PlanState> & new_traversed_states) const {

    int source = new_traversed_states.back()[ts_index_predecessor];
    int abstract_target = ts_index_successor >= 0 ?  abstract_target_state[ts_index_successor] : -1;

    // if (transition_system->get_incorporated_variables().size() == 1) {
    //     cout << g_sas_task()->get_fact_name(FactPair(transition_system->get_incorporated_variables()[0], source)) << endl;
    // }

   
    if (is_target(source, label, abstract_target)) {
        return; // Nothing to do
    }

    int num_states = transition_system->get_size();

    // Find the right target

    std::vector<bool> targets(num_states, false);
    for (int s = 0; s < num_states; ++s) {
        targets[s] = is_target(s, label, abstract_target);
    }

    reconstruct_step (source, targets, new_label_path, new_traversed_states);
    
}

void TauShrinking::reconstruct_goal_step(std::vector<int> & new_label_path,
                                         std::vector<PlanState> & new_traversed_states) const {
    // cout << "Reconstructing goal: " << transition_system->get_incorporated_variables()[0] << endl;
    assert(!new_traversed_states.empty());
    assert (ts_index_predecessor >= 0);
    assert(ts_index_predecessor < (int)(new_traversed_states.back().size()));
    int source = new_traversed_states.back()[ts_index_predecessor];
    reconstruct_step (source, transition_system->get_is_goal(), new_label_path, new_traversed_states);
}

void PlanReconstructionTauPath::reconstruct_plan(Plan &plan) const {    
    const std::vector<int> & label_path = plan.get_labels ();
    const std::vector<PlanState> & traversed_states = plan.get_traversed_states ();
    cout << "TauPath reconstruction of plan with " << label_path.size() << " steps" << endl;
    assert(label_path.size() + 1 == traversed_states.size());

    std::vector<int> new_label_path;
    std::vector<PlanState> new_traversed_states;
     
    new_traversed_states.push_back(initial_state);

    for(size_t step = 0; step < label_path.size(); ++step) {
        //cout << "Step: " << step << endl;
        int label = label_path[step];
        assert(step + 1 < traversed_states.size());
        const PlanState & target = traversed_states[step+1];
        for (const auto & tau_shrinking : tau_transformations) {
            tau_shrinking->reconstruct_step(label, target, new_label_path, new_traversed_states);
        }
    }

    for (const auto & tau_shrinking : tau_transformations) {
        tau_shrinking->reconstruct_goal_step(new_label_path, new_traversed_states);
    }

    // cout << "Tau path " << new_traversed_states[0];
    // for(size_t step = 0; step < new_label_path.size(); ++step) {
    //     cout << " --" << new_label_path[step] << "--> " << new_traversed_states[step+1];
    // }
    // cout << endl;

    plan.set_plan(move(new_traversed_states), move(new_label_path));    
}

}
