#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_H

#include <vector>
#include <memory>

namespace task_representation {
    class LabelID;
    class State;
    class FTSTask;
}

namespace task_transformation {
class PlanReconstruction {
    public:
    // Given a sequence of actions that is a plan for the successor task, retrieve a plan
    // for the predecessor task. Directly modifies the contents of plan and traversed_states
    virtual void reconstruct_plan(std::vector<task_representation::LabelID> & plan,
				  std::vector<task_representation::State> & traversed_states) const = 0;   
};

class PlanReconstructionSequence : public PlanReconstruction {
    std::vector<std::unique_ptr<PlanReconstruction> > plan_reconstruction;

public:
    virtual void reconstruct_plan(std::vector<task_representation::LabelID> & plan,
				  std::vector<task_representation::State> & traversed_states) const {
	for (const auto & pr : plan_reconstruction) {
	    pr->reconstruct_plan(plan, traversed_states);
	}
    }
};



class PlanReconstructionStep : public PlanReconstruction {
    // We do plan reconstruction by performing a search on the predecessor task. We allow two type of transitions:
    // 1) Transitions with a label l that maps to the next label in the plan and whose target maps to the next abstract state in the plan
    // 2) Transitions with a tau label whose target maps to the same abstract state in the plan. 
    //std::shared_ptr<task_representation::FTSTask> predecessor_task;

    // We need the merge and shrink representation, which for evety state in the
    // predecessor task, it obtains the state in the task where the plan was found.    
    //std::vector<MergeAndShrinkRepresentation> merge_and_shrink_representation;

    // We need a LabelMap from labels in the predecessor_task to labels in the task where
    // the plan was found.
    //LabelMap label_map;    

    // This is the set of tau_labels, that we can use. 
    //std::vector<int> tau_labels;


public:

    /* PlanReconstructionStep (std::shared_ptr<task_representation::FTSTask> predecessor_task, */
    /* 			    std::vector<MergeAndShrinkRepresentation> merge_and_shrink_representation, */
    /* 			    LabelMap label_map, */
    /* 			    std::vector<int> tau_labels); */
    
    virtual void reconstruct_plan (std::vector<task_representation::LabelID> & plan,
				   std::vector<task_representation::State> & traversed_states) const;

};

}
#endif
