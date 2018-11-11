#include "plan_reconstruction_tau_path.h"

#include "tau_graph.h"

#include "../task_representation/transition_system.h"

using namespace task_representation;
using namespace std;

namespace task_transformation { 
    PlanReconstructionTauPath::PlanReconstructionTauPath() : initial_state(vector<int>()) {

}

// Does s has a transition with label to some t whose abstraction is abstract_target?
bool TauShrinking::is_target(int s, int label, int abstract_target) const {
    for (const Transition & tr : transition_system->get_transitions_with_label(label) ) {
        if (tr.src == s && abstraction[tr.target] == abstract_target) {
            return true;
        }
    }
    return false;
}


void TauShrinking::
reconstruct_step(int label, const PlanState & abstract_target_state,
                 std::vector<int> & new_label_path,
                 std::vector<PlanState> & new_traversed_states) const {

    int source = new_traversed_states.back()[ts_index_predecessor];
    int abstract_target = abstract_target_state[ts_index_successor];
    if (is_target(source, label, abstract_target)) {
        return; // Nothing to do
    }

    int num_states = transition_system->get_size();

    // 1) Find the right target

    std::vector<bool> targets(num_states, false);
    for (int s = 0; s < num_states; ++s) {
        targets[s] = is_target(s, label, abstract_target);
    }
        
    // 2) Use TauGraph to find a path from src to target
    std::vector<std::pair<task_representation::LabelID, int>> tau_path =
        tau_graph->find_shortest_path(source, targets);
    
    // 3) Insert tau path into the plan
    for (const auto & step : tau_path) {
        new_label_path.push_back(step.first);
        auto new_state = new_traversed_states.back();
        new_state.set(ts_index_predecessor, step.second);
        new_traversed_states.push_back(new_state);
    }
}

void PlanReconstructionTauPath::reconstruct_plan(Plan &plan) const {
    
    const std::vector<int> & label_path = plan.get_labels ();
    const std::vector<PlanState> & traversed_states = plan.get_traversed_states ();
    assert(label_path.size() + 1 == traversed_states.size());

    std::vector<int> new_label_path;
    std::vector<PlanState> new_traversed_states;
     
    new_traversed_states.push_back(initial_state);

    for(size_t step = 0; step < label_path.size(); ++step) {
        //cout << "Step: " << step << endl;
        int label = label_path[step];
        assert(step + 1 < traversed_states.size());
        const PlanState & target = traversed_states[step+1];
        for (const TauShrinking & tau_shrinking : tau_transformations) {
            tau_shrinking.reconstruct_step(label, target, new_label_path, new_traversed_states);
        }
    }
    
    plan.set_plan(move(new_traversed_states), move(new_label_path));    
}

}
