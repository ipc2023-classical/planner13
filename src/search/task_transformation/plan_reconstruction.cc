#include "plan_reconstruction.h"

namespace task_transformation {

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
