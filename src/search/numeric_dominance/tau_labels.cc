#include "tau_labels.h"

#include <iostream>
#include "../option_parser.h"

#include "../task_representation/transition_system.h"
#include "../task_representation/labels.h"
#include "../algorithms/dynamic_bitset.h"

#include "int_epsilon.h"

#include "breadth_first_search.h"
#include "dijkstra_search_epsilon.h"


const int TAU_IN_ALL = -1;
const int TAU_IN_NONE = -2;

using namespace std;

template<typename T>
TauLabels<T>::TauLabels(const std::vector<TransitionSystem> &tss, const Labels &labels) {
    tau_labels.resize(tss.size());
    tau_label_cost.resize(tss.size());

    int num_labels = labels.get_size();
    original_cost.resize(num_labels);
    label_relevant_for.resize(num_labels);

    num_tau_labels_for_some = 0;
    num_tau_labels_for_all = 0;

    // Enumerate all labels and compute determine for which transitions systems they are relevant
    for (LabelID l(0); l < num_labels; ++l) {
        original_cost[l] = epsilon_if_zero<T>(labels.get_label_cost(l));
        assert (original_cost[l] != T(0));

        int transition_system_relevant = TAU_IN_ALL;
        for (size_t ts_id = 0; ts_id < tss.size(); ++ts_id) {
            // If the label cannot be tau in other ts, it may be in this one. If it may be in more than one, it is tau
            // in none.
            if (!label_may_be_tau_in_other_ts(tss[ts_id], l)) {
                if (transition_system_relevant == TAU_IN_ALL) {
                    transition_system_relevant = int(ts_id);
                } else {
                    transition_system_relevant = TAU_IN_NONE;
                    break;
                }
            }
        }
        //TODO: check why there are labels irrelevant everywhere
        // assert(transition_system_relevant != -1);
        if (transition_system_relevant >= 0) {
            num_tau_labels_for_some++;
            // Add this label as a tau label for the transition system in which it is relevant
            tau_labels[transition_system_relevant].push_back(l);
        }
        label_relevant_for[l] = transition_system_relevant;
    }

    std::cout << "Computed tau labels as self-loops everywhere: " << num_tau_labels_for_all << " : "
              << num_tau_labels_for_some << " / " << num_labels << "\n";
}


template<typename T>
set<int>
TauLabels<T>::add_recursive_tau_labels(const std::vector<TransitionSystem> &tss,
                                       const std::vector<std::unique_ptr<TauDistances<T>>> &tau_distances) {
    int num_labels = original_cost.size();
    set<int> check_distances_of;

    assert(tss.size() == tau_distances.size());

    for (LabelID l(0); l < num_labels; ++l) {
        T total_tau_cost = 0;
        int transition_system_relevant = TAU_IN_ALL;
        for (int ts_id = 0; ts_id < int(tss.size()); ++ts_id) {
            assert(tau_distances[ts_id]);
            if (label_may_be_tau_in_other_ts(tss[ts_id], l)) {
                if (!tau_distances[ts_id]->is_fully_invertible()) {
                    if (transition_system_relevant == TAU_IN_ALL) {
                        transition_system_relevant = ts_id;
                    } else {
                        transition_system_relevant = TAU_IN_NONE;
                        break;
                    }
                } else {
                    total_tau_cost += tau_distances[ts_id]->get_cost_fully_invertible();
                }
            }
        }

        if (transition_system_relevant == TAU_IN_ALL && label_relevant_for[l] != TAU_IN_ALL) {
            //This label is a tau label in all transition systems
            for (int ts_id = 0; ts_id < int(tss.size()); ++ts_id) {
                if (tss[ts_id].is_relevant_label(l) && label_relevant_for[l] != ts_id) {
                    tau_labels[ts_id].push_back(l);
                    check_distances_of.insert(ts_id);

                    set_tau_cost(ts_id, l, total_tau_cost -
                                           tau_distances[ts_id]->get_cost_fully_invertible());
                }
            }

            if (label_relevant_for[l] == TAU_IN_NONE) {
                num_tau_labels_for_some++;
            }
            num_tau_labels_for_all++;
        } else if (transition_system_relevant >= 0 && label_relevant_for[l] == TAU_IN_NONE) {
            tau_labels[transition_system_relevant].push_back(l);
            set_tau_cost(transition_system_relevant, l, total_tau_cost);
            check_distances_of.insert(transition_system_relevant);
            num_tau_labels_for_some++;
        }

        label_relevant_for[l] = transition_system_relevant;
    }

    std::cout << "Computed tau labels recursive: " << num_tau_labels_for_all << " : " << num_tau_labels_for_some
              << " / " << num_labels << "\n";

    // int num_labels = original_cost.size();
    // vector<vector<int>> ts_relevant_per_label(num_labels); 
    // for(int l = 0; l < num_labels; l++) {
    // 	for (int lts_id = 0; lts_id < tss.size(); ++lts_id){
    // 	    if(tss[lts_id]->is_relevant_label(l)) {
    // 		ts_relevant_per_label[num_labels].push_back(lts_id);
    // 	    }
    // 	}
    // }

    // for (int lts_id = 0; lts_id < tss.size(); ++lts_id) {
    // 	for (size_t i = 0; i < tss[lts_id]->get_num_label_groups(); ++i) {
    // 	    LabelGroup lg (i);
    // 	    int cost_tau = 0;

    // 	    for (size_t s = 0; s < tss[ts]->size(); ++s) {
    // 		int cost_s = std::numeric_limits<int>::max();
    // 		for (const auto & tr : get_transitions_label_group(lg)) {

    // 		    cost_s = min(cost_s,
    // 				 tau_distances.shortest_path(s, tr.first) + tau_distances.shortest_path(tr.second, s));
    // 		    if (cost_s <= cost_tau) {
    // 			break;
    // 		    }

    // 		}


    // 		cost_tau = max(cost_tau, cost_s);
    // 		if(cost_tau ==  std::numeric_limits<int>::max()) {
    // 		    break;
    // 		}
    // 	    }

    // 	    if(cost_tau < std::numeric_limits<int>::max()) {
    // 		for (int l : tss[lts_id]->get_labels_group(lg)){

    // 		}
    // 	    }
    // 	}	 
    // }

    // for(int l = 0; l < num_labels; l++) {
    // 	int extra_cost = 0;
    // 	erase_if(ts_relevant_per_label[l],
    // 		 [&] (int ts) {
    // 		     int cost = 0;
    // 		     vector<int> cost_per_state (tss.num_states, std::numeric_limits<int>::max());
    // 		     for (size_t s = 0; s < tss[ts]->size(); ++s) {

    // 		     }

    // 			 cost = max(cost, 
    // 		     }
    // 		     extra_cost += cost
    // 		 });
    // }

    return check_distances_of;
}


template<typename T>
bool TauDistances<T>::precompute(const TauLabels<T> &tau_labels, const TransitionSystem &ts,
                                 int ts_id, bool only_reachability) {

    if (!distances_with_tau.empty() && num_tau_labels == tau_labels.size(ts_id)) {
        return false;
    }

    //There may be no new tau labels
    // assert(num_tau_labels >= tau_labels.size(ts_id));

    num_tau_labels = tau_labels.size(ts_id);
    int num_states = ts.get_size();
    distances_with_tau.resize(num_states);
    reachable_with_tau.resize(num_states);
    // max_cost_reach_with_tau.resize(num_states, 0);
    // max_cost_reach_from_with_tau.resize(num_states, 0);

    const auto copy_distances = distances_with_tau;

    if (only_reachability) {
        //Create copy of the graph only with tau transitions
        vector<vector<int> > tau_graph(num_states);
        for (int label_no : tau_labels.get_tau_labels(ts_id)) {
            for (const auto &trans : ts.get_transitions_with_label(label_no)) {
                if (trans.src != trans.target) {
                    tau_graph[trans.src].push_back(trans.target);
                }
            }
        }

        for (int s = 0; s < num_states; ++s) {
            auto &distances = distances_with_tau[s];
            distances.resize(num_states);
            std::fill(distances.begin(), distances.end(), std::numeric_limits<int>::max());
            distances[s] = 0;
            reachable_with_tau[s].clear();

            //Perform Dijkstra search from s
            breadth_first_search_reachability_distances_one(tau_graph, s, distances,
                                                            reachable_with_tau[s]);

            //cout << "BFS finished " << reachable_with_tau[s].size() << endl;
        }
    } else {
        //Create copy of the graph only with tau transitions
        vector<vector<pair<int, T> > > tau_graph(num_states);
        for (int label_no : tau_labels.get_tau_labels(ts_id)) {
            for (const auto &trans : ts.get_transitions_with_label(label_no)) {
                if (trans.src != trans.target) {
                    tau_graph[trans.src].push_back(make_pair(trans.target,
                                                             tau_labels.get_cost(ts_id, label_no)));
                }
            }
        }

        for (int s = 0; s < num_states; ++s) {
            //Perform Dijkstra search from s
            auto &distances = distances_with_tau[s];
            distances.resize(num_states);
            reachable_with_tau[s].clear();
            std::fill(distances.begin(), distances.end(), std::numeric_limits<int>::max());
            distances[s] = 0;
            dijkstra_search_epsilon(tau_graph, s, distances, reachable_with_tau[s]);

        }
    }

    goal_distances_with_tau.resize(num_states);
    for (int s = 0; s < num_states; ++s) {
        goal_distances_with_tau[s] = std::numeric_limits<int>::max();
        for (int t = 0; t < num_states; t++) {
            if (ts.is_goal_state(t)) {
                goal_distances_with_tau[s] = min(goal_distances_with_tau[s], distances_with_tau[s][t]);
            }
        }
    }


    cost_fully_invertible = 0;

    for (int s = 0; s < num_states; ++s) {
        if (int(reachable_with_tau[s].size()) < num_states) {
            cost_fully_invertible = std::numeric_limits<int>::max();
        } else {
            for (int sp = 0; sp < num_states; ++sp) {
                cost_fully_invertible = max(cost_fully_invertible,
                                            distances_with_tau[s][sp] + distances_with_tau[s][sp]);
                // 	max_cost_reach_with_tau [sp] = max(max_cost_reach_with_tau [sp], distances[sp]);
                // 	max_cost_reach_from_with_tau [s] = max(max_cost_reach_from_with_tau [s], distances[sp]);
            }
        }
    }

    if (cost_fully_invertible < std::numeric_limits<int>::max()) {
        cout << "Fully invertible: " << ts_id << " with cost " << cost_fully_invertible << endl;
    }
// #ifndef NDEBUG
//     if (!only_reachability){
// 	const std::vector <int> & goal_distances = abs->get_goal_distances();
// 	for(int t : reachable_with_tau [s]) {
// 	    if(T(goal_distances[s]) > distances[t] + T(goal_distances[t])) {
// 		cout << endl;
// 		cout << T(goal_distances[s])  << endl; 
// 		cout << distances[t] << endl;
// 		cout << T(goal_distances[t]) << endl;
// 	    }
// 	    assert(T(goal_distances[s]) <= distances[t] + T(goal_distances[t]));
// 	    assert(s == t || distances[t] > 0);
// 	}
//     }
// #endif
    if (copy_distances != distances_with_tau) {
        id++;
        return true;
    }
    return false;
}

template<>
int TauDistances<int>::get_cost_fully_invertible() const {
    return cost_fully_invertible;
}

template<>
IntEpsilon TauDistances<IntEpsilon>::get_cost_fully_invertible() const {
    return cost_fully_invertible.get_value() + 1;
}

template<typename T>
void TauLabelManager<T>::initialize(const std::vector<TransitionSystem> &tss, const Labels &labels) {
    tau_labels = make_unique<TauLabels<T>>(tss, labels);

    tau_distances.resize(tss.size());

    //First precompute distances
    for (int ts_id = 0; ts_id < int(tss.size()); ++ts_id) {
        tau_distances[ts_id] = make_unique<TauDistances<T>>();
        tau_distances[ts_id]->precompute(*tau_labels, tss[ts_id], ts_id, only_reachability);
    }

    if (recursive) {
        bool changes = true;
        while (changes) {
            changes = false;
            for (int ts : tau_labels->add_recursive_tau_labels(tss, tau_distances)) {
                changes |= tau_distances[ts]->precompute(*tau_labels, tss[ts], ts, false);
            }
        }
    }
}


template<typename T>
bool TauLabelManager<T>::add_noop_dominance_tau_labels(const std::vector<TransitionSystem> &tss,
                                                       const NumericLabelRelation<T> &label_dominance) {
    if (!noop_dominance) {
        return false;
    }

    bool some_changes = false;
    for (int ts : tau_labels->add_noop_dominance_tau_labels(label_dominance)) {
        some_changes |= tau_distances[ts]->precompute(*tau_labels, tss[ts], ts, only_reachability && !recursive);
    }

    if (recursive) {
        bool changes = true;
        while (changes) {
            changes = false;
            for (int ts : tau_labels->add_recursive_tau_labels(tss, tau_distances)) {
                changes |= tau_distances[ts]->precompute(*tau_labels, tss[ts], ts, false);
            }
            some_changes |= changes;
        }
    }

    return some_changes;
}

template<typename T>
set<int> TauLabels<T>::add_noop_dominance_tau_labels(const NumericLabelRelation<T> &label_dominance) {

    set<int> ts_with_new_tau_labels;
    int num_ltss = tau_labels.size();

    int num_labels = original_cost.size();

    std::cout << "Compute tau labels with noop dominance" << "\n";
    for (LabelID l(0); l < num_labels; ++l) {
        if (label_relevant_for[l] == TAU_IN_ALL) continue;
        if (label_dominance.dominates_noop_in_all(l)) {
            if (label_relevant_for[l] == TAU_IN_NONE) {
                for (int ts_id = 0; ts_id < num_ltss; ++ts_id) {
                    tau_labels[ts_id].push_back(l);
                    ts_with_new_tau_labels.insert(ts_id);
                    if (label_dominance.q_dominates_noop(l, ts_id) < 0) {
                        set_tau_cost(ts_id, l, -label_dominance.q_dominates_noop(l, ts_id));
                    }

                }
                num_tau_labels_for_some++;
            } else if (label_relevant_for[l] >= 0) {
                int ts_id = label_relevant_for[l];
                tau_labels[ts_id].push_back(l);
                ts_with_new_tau_labels.insert(ts_id);
                if (label_dominance.q_dominates_noop(l, ts_id) < 0) {
                    set_tau_cost(ts_id, l, -label_dominance.q_dominates_noop(l, ts_id));
                }
            }
            num_tau_labels_for_all++;
            cout << l << " is tau for all " << endl;

            label_relevant_for[l] = TAU_IN_ALL;

        } else if (label_dominance.dominates_noop_in_some(l)) {
            int lts_id = label_dominance.get_may_dominates_noop_in(l);
            if (label_relevant_for[l] == TAU_IN_NONE) {
                num_tau_labels_for_some++;
                tau_labels[lts_id].push_back(l);
                ts_with_new_tau_labels.insert(lts_id);
                if (label_dominance.q_dominates_noop(l, lts_id) < 0) {
                    set_tau_cost(lts_id, l, -label_dominance.q_dominates_noop(l, lts_id));
                }
            } else {
                assert (label_relevant_for[l] == lts_id);
            }
            label_relevant_for[l] = lts_id;
        }
    }

    std::cout << "Computed tau labels noop: " << num_tau_labels_for_all << " : " << num_tau_labels_for_some << " / "
              << num_labels << "\n";

    return ts_with_new_tau_labels;
}

// This function computes whether this label can be tau in some other transition system than this. This is
// true if it has a self-loop with this label in all states
template<typename T>
bool TauLabels<T>::label_may_be_tau_in_other_ts(const TransitionSystem &ts, LabelID l_id) {
    dynamic_bitset::DynamicBitset<> state_has_self_loop(ts.get_size());
    for (const Transition &trs : ts.get_transitions_with_label(l_id)) {
        if (trs.src == trs.target) {
            state_has_self_loop.set(trs.src);
        }
    }

    // Could be tau in some other ts if all states have self-loops with this label
    return state_has_self_loop.count() == ts.get_size();
}


template<typename T>
TauLabelManager<T>::TauLabelManager(const Options &opts,
                                    bool only_reachability_) : \
    only_reachability(only_reachability_),
    self_loops(opts.get<bool>("tau_labels_self_loops")),
    recursive(opts.get<bool>("tau_labels_recursive")),
    noop_dominance(opts.get<bool>("tau_labels_noop")) {
    // compute_tau_labels_with_noop_dominance(opts.get<bool>("compute_tau_labels_with_noop_dominance")),
    // tau_label_dominance(opts.get<bool>("tau_label_dominance")),
}

template<typename T>
void TauLabelManager<T>::add_options_to_parser(options::OptionParser &parser) {

    parser.add_option<bool>("tau_labels_recursive",
                            "Use stronger notion of tau labels based on self loops everywhere",
                            "true");

    parser.add_option<bool>("tau_labels_self_loops",
                            "Use stronger notion of tau labels based on self loops everywhere",
                            "true");

    parser.add_option<bool>("tau_labels_noop",
                            "Use stronger notion of tau labels based on noop",
                            "true");
}


template<typename T>
void TauLabelManager<T>::print_config() const {
    assert(self_loops || !recursive);
    cout << "Tau labels self_loops: " << self_loops << endl;
    cout << "Tau labels recursive: " << recursive << endl;
    cout << "Tau labels noop: " << noop_dominance << endl;
}



// template class TauDistances<int>; 
// template class TauDistances<IntEpsilon>; 

template
class TauLabelManager<int>;

template
class TauLabelManager<IntEpsilon>;
