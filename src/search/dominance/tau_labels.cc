#include "tau_labels.h"

#include <iostream>
#include "../option_parser.h"

#include "../algorithms/dynamic_bitset.h"

#include "int_epsilon.h"

#include "../algorithms/breadth_first_search.h"
#include "../algorithms/dijkstra_search_epsilon.h"

namespace dominance {
    const int TAU_IN_ALL = -1;
    const int TAU_IN_NONE = -2;

    using namespace std;

    template<typename TCost>
    TauLabels<TCost>::TauLabels(const std::vector<std::unique_ptr<TransitionSystem>> &tss, const Labels &labels) :
            num_tau_labels_for_some {0}, num_tau_labels_for_all {0} {
        int num_tss = tss.size();
        tau_labels.resize(num_tss);
        //Initialized empty, as this is only if there is an additional cost to make an operator tau.
        tau_label_cost.resize(num_tss);

        int num_labels = labels.get_size();
        original_cost.reserve(num_labels);
        label_relevant_for.reserve(num_labels);

        // Enumerate all labels and compute determine for which transitions systems they are relevant
        for (LabelID l(0); l < num_labels; ++l) {
            int transition_system_relevant = TAU_IN_ALL;
            for (size_t ts_id = 0; ts_id < tss.size(); ++ts_id) {
                // If the label cannot be tau in other ts, it may be in this one. If it may be in more than one,
                // it is tau in none.
                if (!label_may_be_tau_in_other_ts(*(tss[ts_id]), l)) {
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

            //Adding label_relevant_for[l]
            assert (int(label_relevant_for.size()) == l);
            label_relevant_for.push_back(transition_system_relevant);

            //Adding original_cost[l]
            assert (int(original_cost.size()) == l);
            original_cost.push_back(epsilon_if_zero<TCost>(labels.get_label_cost(l)));
            assert (original_cost[l] != TCost(0));
        }

        std::cout << "Computed tau labels as self-loops everywhere: " << num_tau_labels_for_all << " : "
                  << num_tau_labels_for_some << " / " << num_labels << "\n";
    }


    template<typename TCost>
    set<int>
    TauLabels<TCost>::add_recursive_tau_labels(const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                           const std::vector<std::unique_ptr<TauDistances<TCost>>> &tau_distances) {
        int num_labels = original_cost.size();
        set<int> check_distances_of;

        assert(tss.size() == tau_distances.size());

        for (LabelID l(0); l < num_labels; ++l) {
            TCost total_tau_cost = 0;
            int transition_system_relevant = TAU_IN_ALL;
            for (int ts_id = 0; ts_id < int(tss.size()); ++ts_id) {
                assert(tau_distances[ts_id]);
                if (label_may_be_tau_in_other_ts(*(tss[ts_id]), l)) {
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
                    if (tss[ts_id]->is_relevant_label(l) && label_relevant_for[l] != ts_id) {
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


    template<typename TCost>
    bool TauDistances<TCost>::precompute(const TauLabels<TCost> &tau_labels, const TransitionSystem &ts,
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
            for (int label_no: tau_labels.get_tau_labels(ts_id)) {
                for (const auto &trans: ts.get_transitions_with_label(label_no)) {
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
            vector<vector<pair<int, TCost> > > tau_graph(num_states);
            for (int label_no: tau_labels.get_tau_labels(ts_id)) {
                for (const auto &trans: ts.get_transitions_with_label(label_no)) {
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

        if (copy_distances != distances_with_tau) {
            ++id;
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

    template <typename TCost>
    TauLabelInfo<TCost>::TauLabelInfo(TauLabels<TCost> _tau_labels,
                                      const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                      bool distances_required) :
                                            tau_labels(_tau_labels),
                                            are_distances_required(distances_required) {

        tau_distances.resize(tss.size());
        //Precompute distances
        for (int ts_id = 0; ts_id < int(tss.size()); ++ts_id) {
            tau_distances[ts_id] = make_unique<TauDistances<TCost>>();
            tau_distances[ts_id]->precompute(tau_labels, *(tss[ts_id]), ts_id, !are_distances_required);
        }
    }

    template <typename TCost>
    std::shared_ptr<TauLabelInfo<TCost>> TauLabelManager::compute_tau_labels(const std::vector<std::unique_ptr<TransitionSystem>> &tss, const Labels &labels, bool are_distances_required) const {

        TauLabels<TCost> tau_labels (tss, labels);
        auto result = make_shared<TauLabelInfo<TCost>>(tau_labels, tss, are_distances_required || recursive);

        if (recursive) {
            result->add_recursive_tau_labels(tss);
        }

        return result;
    }



    template<typename TCost>
    bool TauLabelInfo<TCost>::add_recursive_tau_labels(const std::vector<std::unique_ptr<TransitionSystem>> &tss) {
        bool changes = true;
        bool some_changes = false;
        while (changes) {
            changes = false;
            for ( int ts : tau_labels.add_recursive_tau_labels(tss, tau_distances)) {
                changes |= tau_distances[ts]->precompute(tau_labels, *(tss[ts]), ts, false);
            }
            some_changes |= changes;
        }
        return some_changes;
    }

    template <typename TCost>
    bool TauLabelManager::recompute_tau_labels(TauLabelInfo<TCost> & tau_label_info, const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                               const LabelDominanceFunction<TCost> &label_dominance) const{
        if (!noop_dominance) {
            return false;
        }

        bool some_changes = tau_label_info.add_noop_dominance_tau_labels(tss, label_dominance);

        if (recursive && some_changes) {
            tau_label_info.add_recursive_tau_labels(tss);
        }

        return some_changes;
    }

template<typename TCost>
bool TauLabelInfo<TCost>::add_noop_dominance_tau_labels(const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                                        const LabelDominanceFunction<TCost> &label_dominance) {
    bool some_changes = false;
    for (int ts: tau_labels.add_noop_dominance_tau_labels(label_dominance)) {
        some_changes |= tau_distances[ts]->precompute(tau_labels, *(tss[ts]), ts, !are_distances_required);
    }
    return some_changes;
}

    template<typename TCost>
    set<int> TauLabels<TCost>::add_noop_dominance_tau_labels(const LabelDominanceFunction<TCost> &label_dominance) {

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
    template<typename TCost>
    bool TauLabels<TCost>::label_may_be_tau_in_other_ts(const TransitionSystem &ts, LabelID l_id) {
        dynamic_bitset::DynamicBitset<> state_has_self_loop(ts.get_size());
        for (const Transition &trs: ts.get_transitions_with_label(l_id)) {
            if (trs.src == trs.target) {
                state_has_self_loop.set(trs.src);
            }
        }

        // Could be tau in some other ts if all states have self-loops with this label
        return state_has_self_loop.count() == ts.get_size();
    }


    TauLabelManager::TauLabelManager(const Options &opts) :
        self_loops(opts.get<bool>("tau_labels_self_loops")),
        recursive(opts.get<bool>("tau_labels_recursive")),
        noop_dominance(opts.get<bool>("tau_labels_noop")) {
    }


    void TauLabelManager::add_options_to_parser(options::OptionParser &parser) {

        parser.add_option<bool>("tau_labels_recursive",
                                "Use stronger notion of tau labels based on self loops everywhere",
                                "true");

        parser.add_option<bool>("tau_labels_self_loops",
                                "Use stronger notion of tau labels based on self loops everywhere",
                                "true");

        parser.add_option<bool>("tau_labels_noop",
                                "Use stronger notion of tau labels based on noop",
                                "false"); // TODO: Test this option
    }


    void TauLabelManager::print_config() const {
        assert(self_loops || !recursive);
        cout << "Tau labels self_loops: " << self_loops << endl;
        cout << "Tau labels recursive: " << recursive << endl;
        cout << "Tau labels noop: " << noop_dominance << endl;
    }


    template class TauLabelInfo<int>;
    template class TauLabelInfo<IntEpsilon>;

    template std::shared_ptr<TauLabelInfo<int>> TauLabelManager::compute_tau_labels<int> (const std::vector<std::unique_ptr<TransitionSystem>> &tss, const Labels &labels, bool only_reachability) const;

    template std::shared_ptr<TauLabelInfo<IntEpsilon>> TauLabelManager::compute_tau_labels<IntEpsilon> (const std::vector<std::unique_ptr<TransitionSystem>> &tss, const Labels &labels, bool only_reachability) const;

    template bool TauLabelManager::recompute_tau_labels<int>(TauLabelInfo<int> & tau_labels, const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                              const LabelDominanceFunction<int> &label_dominance) const;

    template bool TauLabelManager::recompute_tau_labels<IntEpsilon>(TauLabelInfo<IntEpsilon> & tau_labels, const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                                              const LabelDominanceFunction<IntEpsilon> &label_dominance) const;


}