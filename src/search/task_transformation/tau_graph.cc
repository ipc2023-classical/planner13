#include "tau_graph.h"

#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "factored_transition_system.h"
#include "../algorithms/sccs.h"

#include <algorithm>

using namespace task_representation;
using namespace std;

namespace task_transformation {
    TauGraph::TauGraph (const FactoredTransitionSystem &fts, int index, bool preserve_optimality) {
        const TransitionSystem & ts = fts.get_ts(index);
        int num_states = ts.get_size();
        is_goal = ts.get_is_goal();

        cout << "Applying OwnLabel shrinking to ts: " << index << endl;
        
        vector<vector<int> > adjacency_matrix(num_states);
        for (const auto & gt : ts) {
            bool is_own = std::any_of(gt.label_group.begin(), gt.label_group.end(),
                                      [&](int label) {
                                          return fts.is_tau_label(index, LabelID(label)) &&
                                          (!preserve_optimality ||
                                           fts.get_labels().get_label_cost(label) == 0);
                                      });
            if(is_own) {
                for (const auto & trans : gt.transitions) {
                    adjacency_matrix[trans.src].push_back(trans.target);
                    // cout << trans.src << " -> " << trans.target << endl;
                }
            }
        }
    
        /* remove duplicates in adjacency matrix */
        for (int i = 0; i < num_states; i++) {
            ::sort(adjacency_matrix[i].begin(), adjacency_matrix[i].end());
            vector<int>::iterator it = unique(adjacency_matrix[i].begin(), adjacency_matrix[i].end());
            adjacency_matrix[i].erase(it, adjacency_matrix[i].end());
        }
    }

    
    StateEquivalenceRelation TauGraph::compute_own_label_shrinking(){        
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

        return final_sccs;
    }

    StateEquivalenceRelation
    TauGraph::compute_own_label_plus_sg_shrinking(const FactoredTransitionSystem &fts, int index){
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

        int new_size = final_sccs.size();
        if (fts.is_only_goal_relevant(index)) {
            /* now bring those groups together that follow the second rule */
            cout << "also using second rule of own-label shrinking" << endl;
            int goal_scc = -1;
            for (size_t i = 0; i < final_sccs.size(); i++) {
                if (is_goal[final_sccs[i].front()]) {
                    if (goal_scc == -1) {
                        goal_scc = i;
                    } else {
                        final_sccs[goal_scc].splice_after(final_sccs[goal_scc].begin(), final_sccs[i]);
                        new_size--;
                    }
                }
            }
        }
    
        // only need to apply abstraction if this actually changes anything
        StateEquivalenceRelation equivalence_relation;
        equivalence_relation.resize(new_size);
        int counter = 0;
        for (size_t group = 0; group < final_sccs.size(); ++group) {
            if (final_sccs[group].empty())
                continue;
            equivalence_relation[counter].swap(final_sccs[group]);
            counter++;
        }
    
        return final_sccs;
    }
}
