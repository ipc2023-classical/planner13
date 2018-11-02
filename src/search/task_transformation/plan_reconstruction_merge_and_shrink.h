#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_MERGE_AND_SHRINK_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_MERGE_AND_SHRINK_H

#include "plan_reconstruction.h"

namespace task_transformation {
class LabelMap;
class MergeAndShrinkRepresentation;

class PlanReconstructionMergeAndShrink : public PlanReconstruction {
    // We do plan reconstruction by performing a search on the predecessor task. We allow two type of transitions:
    // 1) Transitions with a label l that maps to the next label in the plan and whose target maps to the next abstract state in the plan
    // 2) Transitions with a tau label whose target maps to the same abstract state in the plan. 
    //std::shared_ptr<task_representation::FTSTask> predecessor_task;

    // We need the merge and shrink representation, which for evety state in the
    // predecessor task, it obtains the state in the task where the plan was found.    
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> merge_and_shrink_representations;

    // We need a LabelMap from labels in the predecessor_task to labels in the task where
    // the plan was found.
    std::unique_ptr<LabelMap> label_map;

    // This is the set of tau_labels, that we can use. 
    //std::vector<int> tau_labels;
public:
    PlanReconstructionMergeAndShrink(
//        std::shared_ptr<task_representation::FTSTask> predecessor_task,
        std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&merge_and_shrink_representations,
        std::unique_ptr<LabelMap> label_map/*,
        std::vector<int> tau_labels*/);
    virtual ~PlanReconstructionMergeAndShrink() override;
    virtual void reconstruct_plan(
        std::vector<task_representation::LabelID> &plan,
        std::vector<task_representation::State> &traversed_states) const override;
};
}
#endif
