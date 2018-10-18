#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H

#include <vector>
#include <memory>

namespace task_representation {
    class OperatorID;
    class State;
    
}

namespace task_transformation {
class PlanReconstruction {
    public:
    // Given a sequence of actions that is a plan for the successor task, retrieve a plan
    // for the predecessor task. Directly modifies the contents of plan and
    // traversed_states
    virtual void reconstruct_plan(std::vector<task_representation::OperatorID> & plan,
				  std::vector<task_representation::State> & traversed_states) const = 0;   
};

class PlanReconstructionSequence {
    std::vector<std::unique_ptr<PlanReconstruction> > plan_reconstruction;

public:
    virtual void reconstruct_plan(std::vector<task_representation::OperatorID> & plan,
				  std::vector<task_representation::State> & traversed_states) const {
	for (const auto & pr : plan_reconstruction) {
	    pr->reconstruct_plan(plan, traversed_states);
	}
    }
};

class LabelReductionPlanReconstruction : public PlanReconstruction {
    /* std::shared_ptr<Task> predecessor_task; */
    /* std::shared_ptr<Operators> predecessor_ops, successor_ops; */
    /* LabelMap label_map; */
    
    public:
    virtual void reconstruct_plan(std::vector<task_representation::OperatorID> & plan, 
				  std::vector<task_representation::State> & traversed_states) const;
};


class ShrinkingPlanReconstruction : public PlanReconstruction {
    /* std::shared_ptr<Task> predecessor_task; */
    /* std::shared_ptr<Operators> predecessor_ops, successor_ops; */
    /* LabelMap label_map; */

public:
    virtual void reconstruct_plan (std::vector<task_representation::OperatorID> & plan,
				   std::vector<task_representation::State> & traversed_states) const;

};

}
#endif
