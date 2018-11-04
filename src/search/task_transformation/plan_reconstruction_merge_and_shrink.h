#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_MERGE_AND_SHRINK_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_MERGE_AND_SHRINK_H

#include "plan_reconstruction.h"
#include "../search_space.h"
#include "../state_registry.h"

namespace task_representation {
class FTSTask;
}

namespace task_transformation {
class LabelMap;
class MergeAndShrinkRepresentation;

class PlanReconstructionMergeAndShrink : public PlanReconstruction {
    // We do plan reconstruction by performing a search on the predecessor task. We allow two type of transitions:
    // 1) Transitions with a label l that maps to the next label in the plan and whose target maps to the next abstract state in the plan
    // 2) Transitions with a tau label whose target maps to the same abstract state in the plan. 
    std::shared_ptr<task_representation::FTSTask> predecessor_task;

    mutable StateRegistry state_registry;
    mutable SearchSpace search_space;

    // We need the merge and shrink representation, which for evety state in the
    // predecessor task, it obtains the state in the task where the plan was found.    
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> merge_and_shrink_representations;

    // We need a LabelMap from labels in the predecessor_task to labels in the task where
    // the plan was found.
    std::unique_ptr<LabelMap> label_map;


    // This is the set of tau_labels, that we can use. 
    //std::vector<int> tau_labels;
    
    bool match_states(const GlobalState & original_state,
                      const GlobalState & abstract_state) const;

    bool match_labels(int original_label,
                      int abstract_label) const;

    void reconstruct_step(int label, const GlobalState & target,
                          std::vector<int> & new_label_path,
                          std::vector<GlobalState> & new_traversed_states) const ;
    
public:
    PlanReconstructionMergeAndShrink(
        const std::shared_ptr<task_representation::FTSTask> &predecessor_task,
        std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&merge_and_shrink_representations,
        std::unique_ptr<LabelMap> label_map);
    virtual ~PlanReconstructionMergeAndShrink() = default;
    virtual void reconstruct_plan(Plan &plan) const override;
};
}
#endif
