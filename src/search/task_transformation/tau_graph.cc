#include "tau_graph.h"

#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "factored_transition_system.h"
#include "../algorithms/sccs.h"
#include "../algorithms/priority_queues.h"

#include "../utils/system.h"

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

        std::vector<std::map<int, TauTransition>> adjacency_map(num_states);

        const Labels & labels_info = fts.get_labels();
        for (const auto & gt : ts) {
            LabelID best_label;
            bool is_own = false;
            for (int label : gt.label_group) {
                if ((!preserve_optimality ||
                     labels_info.get_label_cost(label) == 0) &&
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
                        adjacency_map[trans.src].insert(
                            pair<int, TauTransition>(trans.target,
                                                     TauTransition(trans.target,
                                                                   best_label,
                                                                   labels_info.get_label_cost(best_label))));
                    } else if (labels_info.get_label_cost(best_label) <
                               pos->second.cost) {
                        pos->second = TauTransition(trans.target,
                                                    best_label,
                                                    labels_info.get_label_cost(best_label));
                    }
                }
            }
        }
        /* remove duplicates in adjacency matrix */
        for (int i = 0; i < num_states; i++) {
            for(const auto & entry : adjacency_map[i]) {
                adjacency_matrix[i].push_back(entry.second);
            }
        }
    }

    
    StateEquivalenceRelation TauGraph::compute_own_label_shrinking(){        
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC<TauTransition>::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

        cout << "FINAL SCCS: " << final_sccs.size() << endl;
        cout << final_sccs[0].empty() << endl;
        return final_sccs;
    }

    StateEquivalenceRelation
    TauGraph::compute_own_label_plus_sg_shrinking(const FactoredTransitionSystem &fts, int index){
        /* perform Tarjan's algorithm for finding SCCs */
        StateEquivalenceRelation final_sccs;
        sccs::SCC<TauTransition>::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

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

            return equivalence_relation;
        }
        return final_sccs;
    }
    





    // Auxiliar method that finds the shortest path from source to a target state. It
    // returns the path as a pair of label, target transitions.
    std::vector<std::pair<LabelID, int>>
    TauGraph::find_shortest_path (int source, const std::vector<bool> & target) const {

        cout << "Find shortest path" << endl;
        std::vector<std::pair<LabelID, int>> result_path;

        int num_states = adjacency_matrix.size();
        
        vector<int> distances (num_states, std::numeric_limits<int>::max());
        distances[source] = 0;
        vector<pair<int, LabelID>> best_supporter(num_states, pair<int, LabelID> (-1, LabelID(-1)));

        priority_queues::AdaptiveQueue<int> queue;
        queue.push(0, source);

        while (!queue.empty()) {
            pair<int, int> top_pair = queue.pop();
            int distance = top_pair.first;
            int state = top_pair.second;
            if (target[state]) {
                while(best_supporter[state].first != -1) {
                    result_path.push_back(pair<LabelID, int>
                                          (best_supporter[state].second, state));
                    state = best_supporter[state].first;                    
                }
                assert(state == source);
                std::reverse(result_path.begin(),result_path.end());
                return result_path;
            }
            int state_distance = distances[state];
            assert(state_distance <= distance);
            if (state_distance < distance)
                continue;
            for (const TauTransition &transition : adjacency_matrix[state]) {
                int successor_cost = state_distance + transition.cost;
                if (distances[transition.target] > successor_cost) {
                    distances[transition.target] = successor_cost;
                    queue.push(successor_cost, transition.target);
                    best_supporter[transition.target] = pair<int, LabelID>(state, transition.label);
                }
            }
        }

        cerr << "Error: path in tau graph not found" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
        return result_path;
        
    }
}


