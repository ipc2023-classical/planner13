#include "plan_reconstruction_merge_and_shrink.h"

#include "label_map.h"
#include "merge_and_shrink_representation.h"
#include "../task_representation/fts_task.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/search_task.h"
#include "../operator_id.h"
#include "../utils/system.h"

#include <iostream>
using namespace std;

namespace task_transformation {
PlanReconstructionMergeAndShrink::PlanReconstructionMergeAndShrink(
    const shared_ptr<task_representation::FTSTask> &predecessor_task,
    vector<unique_ptr<MergeAndShrinkRepresentation>> &&merge_and_shrink_representations,
    unique_ptr<LabelMap> label_map)
    : predecessor_task(predecessor_task),
      state_registry(predecessor_task->get_search_task()),
      search_space(state_registry,OperatorCost::NORMAL), 
      merge_and_shrink_representations(move(merge_and_shrink_representations)),
      label_map(move(label_map)) {
}


    bool PlanReconstructionMergeAndShrink::match_states(const GlobalState & original_state,
                                                 const GlobalState & abstract_state) const {
        for(size_t i = 0; i < merge_and_shrink_representations.size(); ++i) {
            int value = merge_and_shrink_representations[i]->get_value(original_state);
            if (value != abstract_state[i])  {
                return false;
            }
        }
        return true;
    }

    
    bool PlanReconstructionMergeAndShrink::match_labels(int original_label,
                                                        int abstract_label) const {
        // cout << "cmp " << label_map->get_reduced_label(original_label)
        //      << " "  << abstract_label << endl;
        return label_map->get_reduced_label(original_label) == abstract_label;
    }

    void PlanReconstructionMergeAndShrink::reconstruct_step(int label, const GlobalState & target,
                                                            std::vector<int> & new_label_path,
                                                            std::vector<GlobalState> & new_traversed_states) const {
        const GlobalState & initial_state = new_traversed_states.back();
        auto search_task = predecessor_task->get_search_task();

        std::vector<OperatorID> applicable_ops; 
        search_task->generate_applicable_ops(initial_state, applicable_ops);
        bool found_match = false;
        for (OperatorID op : applicable_ops)  {
            LabelID original_label = search_task->get_label (op);
            
            if (match_labels(original_label, label)) {
                auto result_state = state_registry.get_successor_state(initial_state, op);
                if (match_states(result_state, target)) {
                    found_match = true;
                    //cout << g_sas_task()->get_operator_name(original_label);
                    new_label_path.push_back(original_label);
                    new_traversed_states.push_back(result_state);
                    break;
                }
            }
        }
        if (!found_match) {
            cout << "Error: no match found in plan reconstruction" << endl;
            cout << "Trying to find a match with abstract label: " << label << endl;
            for(int i = 0; i < g_sas_task()->get_num_operators(); ++i) {
                if (label_map->get_reduced_label(i) == label) {
                    cout <<  g_sas_task()->get_operator_name(i) << endl;
                }
            }
            cout << "Available original labels:" << endl;
            for (OperatorID op : applicable_ops)  {
                LabelID original_label = search_task->get_label (op);
                cout << "  " << original_label << " reduced to " << label_map->get_reduced_label(original_label) << endl;
            }
            utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
        }

    }

void PlanReconstructionMergeAndShrink::reconstruct_plan(Plan & plan) const {
    const std::vector<int> & label_path = plan.get_labels ();
    const std::vector<GlobalState> & traversed_states = plan.get_traversed_states ();
    assert(label_path.size() + 1 == traversed_states.size());
    std::vector<int> new_label_path;
    std::vector<GlobalState> new_traversed_states;

    GlobalState predecessor = traversed_states[0];
    new_traversed_states.push_back(state_registry.get_initial_state());
    for(size_t step = 0; step < label_path.size(); ++step) {
        //cout << "Step: " << step << endl;
        int label = label_path[step];
        assert(step + 1 < traversed_states.size());
        const GlobalState & target = traversed_states[step+1];
        reconstruct_step(label, target, new_label_path, new_traversed_states);
    }
    
    plan.set_plan(new_traversed_states, new_label_path);
}


}
