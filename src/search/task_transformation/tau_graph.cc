#include "tau_graph.h"

#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "factored_transition_system.h"
#include "../algorithms/sccs.h"

#include <algorithm>
#include <utility>
#include <map>

using namespace task_representation;
using namespace std;

namespace task_transformation {
    TauGraph::TauGraph (const FactoredTransitionSystem &fts,
                        int index,
                        bool preserve_optimality) :
        adjacency_matrix(fts.get_ts(index).get_size()),
        is_goal (fts.get_ts(index).get_is_goal())  {
        const TransitionSystem & ts = fts.get_ts(index);
        int num_states = ts.get_size();

        std::vector<std::map<int, LabelID>> adjacency_map;
        
        const Labels & labels_info = fts.get_labels();
        for (const auto & gt : ts) {
            LabelID best_label;
            bool is_own = false;
            for (int label : gt.label_group) {
                if ((!preserve_optimality ||
                     fts.get_labels().get_label_cost(label) == 0) &&
                    fts.is_tau_label(index, LabelID(label))) {
                    if (!is_own ||
                        labels_info.get_label_cost(label) <
                        labels_info.get_label_cost(best_label)) {
                        is_own = true;
                        best_label = LabelID(label);
                    }
                }
            }
                    
            if(is_own) {
                for (const auto & trans : gt.transitions) {
                    auto pos = adjacency_map[trans.src].find(trans.target);
                    if (pos == adjacency_map[trans.src].end()) {
                        adjacency_map[trans.src][trans.target] = best_label;
                    } else if (labels_info.get_label_cost(best_label) <
                        labels_info.get_label_cost(pos->second)) {
                        pos->second = best_label;
                    }
                }
            }
        }
    
        /* remove duplicates in adjacency matrix */
        for (int i = 0; i < num_states; i++) {
            copy(adjacency_map[i].begin(), adjacency_map[i].end(),
                 back_inserter(adjacency_matrix[i]));
        }
    }

    
    StateEquivalenceRelation TauGraph::compute_own_label_shrinking(){        
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC<pair<int, LabelID> >::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

        return final_sccs;
    }

    StateEquivalenceRelation
    TauGraph::compute_own_label_plus_sg_shrinking(const FactoredTransitionSystem &fts, int index){
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC<pair<int, LabelID> >::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

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





    // Auxiliar method that finds the shortest path from source to a target state. It
    // returns the path as a pair of label, target transitions.
    std::vector<std::pair<LabelID, int>>
    TauGraph::find_shortest_path (int // source
                                  , const std::vector<bool> & // target
        ) const {

        return     std::vector<std::pair<LabelID, int>>();

    // static void dijkstra_search(
    //     const vector<vector<pair<int, int>>> &graph,
    //     priority_queues::AdaptiveQueue<int> &queue,
    //     vector<int> &distances, ) {
        
    //     while (!queue.empty()) {
    //         pair<int, int> top_pair = queue.pop();
    //         int distance = top_pair.first;
    //         int state = top_pair.second;
    //         int state_distance = distances[state];
    //         assert(state_distance <= distance);
    //         if (state_distance < distance)
    //             continue;
    //         for (size_t i = 0; i < graph[state].size(); ++i) {
    //             const pair<int, int> &transition = graph[state][i];
    //             int successor = transition.first;
    //             int cost = transition.second;
    //             int successor_cost = state_distance + cost;
    //             if (distances[successor] > successor_cost) {
    //                 distances[successor] = successor_cost;
    //                 queue.push(successor_cost, successor);
    //             }
    //         }
    //     }
    // }
    }

}
