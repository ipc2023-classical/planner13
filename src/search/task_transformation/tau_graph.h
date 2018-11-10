#ifndef TASK_TRANSFORMATION_TAU_GRAPH_H
#define TASK_TRANSFORMATION_TAU_GRAPH_H

#include "types.h"

#include <vector> 
namespace task_transformation {
/* class LabelMap; */
/* class MergeAndShrinkRepresentation; */

    class FactoredTransitionSystem;
    
    class TauGraph {
        std::vector<std::vector<int> > adjacency_matrix; 
        std::vector<bool> is_goal;
    public:

        TauGraph(const FactoredTransitionSystem &fts, int index, bool preserve_optimality);

        StateEquivalenceRelation compute_own_label_shrinking();
        StateEquivalenceRelation compute_own_label_plus_sg_shrinking(const FactoredTransitionSystem &fts, int index);

        //Inserts tau-labels into the plan to reach target
        /* void reconstruct_path(int target,  std::Plan & plan); */
    };

}
#endif
