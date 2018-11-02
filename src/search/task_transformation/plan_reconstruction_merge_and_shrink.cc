#include "plan_reconstruction_merge_and_shrink.h"

#include "label_map.h"
#include "merge_and_shrink_representation.h"

using namespace std;

namespace task_transformation {
PlanReconstructionMergeAndShrink::PlanReconstructionMergeAndShrink(
    const shared_ptr<task_representation::FTSTask> &predecessor_task,
    vector<unique_ptr<MergeAndShrinkRepresentation>> &&merge_and_shrink_representations,
    unique_ptr<LabelMap> label_map)
    : predecessor_task(predecessor_task),
      merge_and_shrink_representations(move(merge_and_shrink_representations)),
      label_map(move(label_map)) {
}

PlanReconstructionMergeAndShrink::~PlanReconstructionMergeAndShrink() {
    for (auto &mas_representation : merge_and_shrink_representations) {
        mas_representation = nullptr;
    }
    label_map = nullptr;
}

void PlanReconstructionMergeAndShrink::reconstruct_plan(
    vector<task_representation::LabelID> &,
    vector<task_representation::State> &) const {
    // TODO
}

// void LabelReductionPlanReconstruction::reconstruct_plan(std::vector<OperatorID> & plan) const {
//     vector<OperatorID> new_plan;
//     for(const auto & op_id : plan) {
//     const Operator & op = successor_ops.get_operator(op_id);
//     for (int old_label : label_map.get_reverse(op.get_label())) {
//         if (old_task->has_transition(current_state, old_label, op.get_effects())) {
//         new_plan.push_back(predecessor_ops.get_id(old_label, op.get_effects()));
//         }
//     }
//     }

//     new_plan.swap(plan);
// }

}
