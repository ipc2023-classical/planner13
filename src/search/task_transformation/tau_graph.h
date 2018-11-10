#ifndef TASK_TRANSFORMATION_TAU_GRAPH_H
#define TASK_TRANSFORMATION_TAU_GRAPH_H

#include "types.h"

#include "../task_representation/labels.h"

#include <vector>


namespace task_transformation {
/* class LabelMap; */
/* class MergeAndShrinkRepresentation; */

    class FactoredTransitionSystem;
    
    class TauGraph {
        std::vector<std::vector<std::pair<int, task_representation::LabelID> > > adjacency_matrix; 
        std::vector<bool> is_goal;
    public:

        TauGraph(const FactoredTransitionSystem &fts, int index, bool preserve_optimality);

        StateEquivalenceRelation compute_own_label_shrinking();
        StateEquivalenceRelation compute_own_label_plus_sg_shrinking(const FactoredTransitionSystem &fts, int index);

        // Auxiliar method that finds the shortest path from source to a target state. It
        // returns the path as a pair of label, target transitions.
        std::vector<std::pair<task_representation::LabelID, int>>
            find_shortest_path (int source, const std::vector<bool> & target) const; 
    };

}
#endif
