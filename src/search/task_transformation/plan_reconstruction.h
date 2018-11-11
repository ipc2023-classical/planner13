#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H

#include "../plan.h"
#include <vector>
#include <memory>

namespace task_representation {
    class LabelID;
    class State;
}

namespace task_transformation {
class PlanReconstruction {
public:
    virtual ~PlanReconstruction() = default;
    // Given a sequence of actions that is a plan for the successor task, retrieve a plan
    // for the predecessor task. Directly modifies the contents of plan and traversed_states
    virtual void reconstruct_plan(Plan & plan) const = 0;
};

class PlanReconstructionSequence : public PlanReconstruction {
    std::vector<std::shared_ptr<PlanReconstruction>> plan_reconstructions;
public:
    explicit PlanReconstructionSequence(
        std::vector<std::shared_ptr<PlanReconstruction>> plan_reconstructions);
    virtual ~PlanReconstructionSequence() override = default;

    virtual void reconstruct_plan(Plan & plan) const override {
        for (const auto & pr : plan_reconstructions) {
            pr->reconstruct_plan(plan);
        }
    }
};
}
#endif
