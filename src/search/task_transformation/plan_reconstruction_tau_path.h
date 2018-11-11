#ifndef TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_TAU_PATH_H
#define TASK_TRANSFORMATION_PLAN_RECONSTRUCTION_TAU_PATH_H

#include "plan_reconstruction.h"
#include "../search_space.h"
#include "../state_registry.h"

namespace task_representation {
class FTSTask;
class TransitionSystem;
}

namespace task_transformation {
class LabelMap;
class MergeAndShrinkRepresentation;
class TauGraph;

// This Task Reconstruction obtains a plan from the task right after tau-label shrinking
// has been applied to a transition system. This requires us, every time this type of
// shrinking is applied we should restart the merge_and_shrink process from there.

// The plan that we receive contains states where one of two things happens:
// a) ts_index == -1 -> the states do not have any value of our variable. This makes
//    things easier because it means that we only need to introduce a tau-path from
//    the current state to a state where the next label in the plan is executable.

// b) ts_index >= 0 -> the states have a value for ts_index val, therefore we must
// ensure that the tau-path ends in a state s with a transition s -- l --> t where l
// is the next label in the plan and abstraction(t) == val where abstraction is the
// equivalence mapping returned by this abstraction.

class TauShrinking {
    // Index of the transition system in the predecessor and successor task
    int ts_index_predecessor, ts_index_successor; 

    // Tau label graph of the predecessor task
    std::unique_ptr<TauGraph> tau_graph;
    
    // transition system of the predecessor class: needed to know from which states a
    // label is applicable.
    std::unique_ptr<task_representation::TransitionSystem> transition_system; 

    // Mapping from states to abstract states
    std::vector<int> abstraction;
    
    bool is_target(int s, int label, int abstract_target) const;

public:
    TauShrinking (int ts_index_predecessor_, int ts_index_successor_,
                  std::unique_ptr<TauGraph> tau_graph_,
                  std::vector<int> && abstraction_mapping_,
                  std::unique_ptr<task_representation::TransitionSystem> && transition_system_) :
    ts_index_predecessor (ts_index_predecessor_),
        ts_index_successor (ts_index_successor_),
        tau_graph(move(tau_graph_)),
        transition_system(move(transition_system_)),
        abstraction(move(abstraction_mapping_)) {
        }

        
    void reconstruct_step(int label, const PlanState & target,
                          std::vector<int> & new_label_path,
                          std::vector<PlanState> & new_traversed_states) const ;
};

class PlanReconstructionTauPath : public PlanReconstruction {
    PlanState initial_state;
    std::vector<std::unique_ptr<TauShrinking> > tau_transformations;
public:
    PlanReconstructionTauPath(PlanState initial_state_,
                              std::vector<std::unique_ptr<TauShrinking> > && transformations) :
    initial_state(initial_state_), tau_transformations(move(transformations)) {
    }
    
    virtual ~PlanReconstructionTauPath() = default;
    virtual void reconstruct_plan(Plan &plan) const override;
};
}
#endif
