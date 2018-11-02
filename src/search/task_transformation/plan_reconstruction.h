#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H

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
    virtual void reconstruct_plan(std::vector<task_representation::LabelID> & plan,
        std::vector<task_representation::State> & traversed_states) const = 0;
};

class PlanReconstructionSequence : public PlanReconstruction {
    std::vector<std::unique_ptr<PlanReconstruction>> plan_reconstructions;
public:
    explicit PlanReconstructionSequence(
        std::vector<std::unique_ptr<PlanReconstruction>> &&plan_reconstructions);
    virtual ~PlanReconstructionSequence() override;
    virtual void reconstruct_plan(std::vector<task_representation::LabelID> & plan,
        std::vector<task_representation::State> & traversed_states) const override {
        for (const auto & pr : plan_reconstructions) {
            pr->reconstruct_plan(plan, traversed_states);
        }
    }
};
}
#endif
