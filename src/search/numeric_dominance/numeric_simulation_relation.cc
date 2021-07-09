#include "numeric_simulation_relation.h"
#include "numeric_label_relation.h"
#include "../algorithms/priority_queues.h"
#include "dijkstra_search_epsilon.h"
#include "breadth_first_search.h"
#include "../task_transformation/distances.h"
#include "../utils/timer.h"

#include <map>

using namespace std;

bool applyPostSrc(const TransitionSystem& ts, int src, std::function<bool(const Transition & t, LabelGroupID lg_id)> && f) {
    // TODO: This inefficiently goes through all transitions each time, could be ordered by src state
    for (LabelGroupID lg_id(0); lg_id < ts.num_label_groups(); ++lg_id) {
        for (const Transition t : ts.get_transitions_for_group_id(lg_id)) {
            if (t.src != src) continue;

            if (f(t, lg_id)) return true;
        }
    }
    return false;
}

template<typename T>
NumericSimulationRelation<T>::NumericSimulationRelation(const TransitionSystem& _ts, int _ts_id,
                                                        int truncate_value,
                                                        std::shared_ptr<TauLabelManager<T>> tau_labels_mgr)
        : ts(_ts), ts_id(_ts_id), truncate_value(truncate_value), tau_labels(tau_labels_mgr), max_relation_value(0),
          cancelled(false) {
}

template<typename T>
void NumericSimulationRelation<T>::init_goal_respecting() {
    // TODO: Computing distances (possibly) again, they might already have been computed in task transformation
    Distances distances = task_transformation::Distances(ts);
    distances.compute_distances(false, true, Verbosity::VERBOSE);

    int num_states = ts.get_size();

    relation.resize(num_states);
    // is_relation_stable.resize(num_states);
    for (int s = 0; s < num_states; s++) {
        relation[s].resize(num_states);
        // is_relation_stable[s].resize(num_states, false);
        for (int t = 0; t < num_states; t++) {
            // qrel (t, s) = h*(t) - h*(s)
            // if (!abs->is_goal_state(t) && abs->is_goal_state(s)) {
            // 	relation[s][t] = goal_distances_with_tau[t] ;
            // } else {
            // 	relation[s][t] = goal_distances[t] - goal_distances[s];
            // }

            //Here we have not computed tau distances yet (because
            //with dominated by noop version may change)
            relation[s][t] = distances.get_goal_distance(t) - distances.get_goal_distance(s);
        }
    }
    tau_distances_id = 0;
}


template<>
void NumericSimulationRelation<IntEpsilon>::init_goal_respecting() {
    // TODO: Computing distances (possibly) again, they might already have been computed in task transformation
    Distances distances = task_transformation::Distances(ts);
    distances.compute_distances(false, true, Verbosity::NORMAL);

    int num_states = ts.get_size();
    cout << "Recompute distances with epsilon" << endl;
    relation.resize(num_states);

    for (int s = 0; s < num_states; s++) {
        relation[s].resize(num_states);
        // is_relation_stable[s].resize(num_states, false);

        for (int t = 0; t < num_states; t++) {
            // qrel (t, s) = h*(t) - h*(s)
            // if (!abs->is_goal_state(t) && abs->is_goal_state(s)) {
            // 	relation[s][t] = goal_distances_with_tau[t;]
            // } else {
            IntEpsilonSum rel = IntEpsilonSum(distances.get_goal_distance(t)) - IntEpsilonSum(distances.get_goal_distance(s));
            relation[s][t] = rel.get_epsilon_negative();
            // }
        }
    }
    tau_distances_id = 0;
}

template<typename T>
T NumericSimulationRelation<T>::compare_transitions(int tr_s_target, LabelID tr_s_label,
                                                    int tr_t_target, LabelID tr_t_label,
                                                    T tau_distance,
                                                    const NumericLabelRelation<T> &label_dominance) const {
    if (label_dominance.may_dominate(tr_t_label, tr_s_label, ts_id) &&
        may_simulate(tr_t_target, tr_s_target)) {

        return tau_distance +
               label_dominance.q_dominates(tr_t_label, tr_s_label, ts_id)
               + label_dominance.get_label_cost(tr_s_label)
               - label_dominance.get_label_cost(tr_t_label)
               + q_simulates(tr_t_target, tr_s_target);
    } else {
        return std::numeric_limits<int>::lowest();
    }
}

template<typename T>
T NumericSimulationRelation<T>::compare_noop(int tr_s_target, LabelID tr_s_label,
                                             int t, T tau_distance,
                                             const NumericLabelRelation<T> &label_dominance) const {

    // Checking noop
    if (may_simulate(t, tr_s_target) &&
        label_dominance.may_dominated_by_noop(tr_s_label, ts_id)) {
        return tau_distance +
               q_simulates(t, tr_s_target) +
               label_dominance.get_label_cost(tr_s_label) +
               label_dominance.q_dominated_by_noop(tr_s_label, ts_id);
    } else {
        return std::numeric_limits<int>::lowest();
    }
}


template<typename T>
void NumericSimulationRelation<T>::cancel_simulation_computation() {

    const auto &tau_distances = tau_labels->get_tau_distances(ts_id);
    int new_tau_distances_id = tau_distances.get_id();
    if (new_tau_distances_id != tau_distances_id || !cancelled) {
        const auto &_tau_distances = tau_labels->get_tau_distances(ts_id);
        cancelled = true;
        for (int s = 0; s < ts.get_size(); s++) {
            for (int t = 0; t < ts.get_size(); t++) { //for each pair of states t, s
                update_value(t, s, _tau_distances.minus_shortest_path(t, s));
            }
        }
    }
}


template<typename T>
vector<int> NumericSimulationRelation<T>::get_dangerous_labels() const {
    vector<int> dangerous_labels;

    int num_states = ts.get_size();
    vector<bool> is_state_to_check(num_states);
    vector<bool> is_ok(num_states);
    for (LabelGroupID lg_id(0); lg_id < ts.num_label_groups(); ++lg_id) {
        std::fill(is_ok.begin(), is_ok.end(), false);
        std::fill(is_state_to_check.begin(), is_state_to_check.end(), false);
        const std::vector<Transition> &trs = ts.get_transitions_for_group_id(lg_id);
        std::vector<int> states_to_check;

        for (const auto &tr : trs) {
            int s = tr.src;
            int t = tr.target;
            if (is_ok[s]) {
                continue;
            } else if (may_simulate(t, s) && q_simulates(t, s) >= 0) {
                is_ok[s] = true;
            } else if (!is_state_to_check[s]) {
                states_to_check.push_back(s);
                is_state_to_check[s] = true;
            }
        }

        for (int s : states_to_check) {
            if (!is_ok[s]) {
                // Add dangerous labels
                const LabelGroup &lg = ts.get_label_group(lg_id);
                dangerous_labels.insert(dangerous_labels.end(), lg.begin(), lg.end());
                break;
            }
        }
    }
    //for (int i  : dangerous_labels) cout << g_operators[i].get_name() << endl;
    return dangerous_labels;
}


template<typename T>
int NumericSimulationRelation<T>::update_pair_stable(const NumericLabelRelation<T> &label_dominance,
                                                     const TauDistances<T> &tau_distances,
                                                     int s, int t) {
    assert (s != t // && !is_relation_stable[s][t]
            && may_simulate(t, s));

    T lower_bound = tau_distances.minus_shortest_path(t, s);
    T previous_value = q_simulates(t, s);
    // cout << "prev: " << previous_value << endl;

    assert(lower_bound <= previous_value);
    if (lower_bound == previous_value) {
        return false;
    }

    T min_value = previous_value;
    // if (ts->is_goal(t) || !ts->is_goal(s)) {

    //Check if really t simulates s
    //for each transition s--l->s':
    // a) with noop t >= s' and l dominated by noop?
    // b) exist t--l'-->t', t' >= s' and l dominated by l'?
    applyPostSrc(ts, s, [&](const Transition &trs, LabelGroupID lg_id) {
        for (int tr_s_label : ts.get_label_group(lg_id)) {
            T max_value = std::numeric_limits<int>::lowest();
            for (int t2 : tau_distances.states_reachable_from(t)) {
                T tau_distance = tau_distances.minus_shortest_path(t, t2);

                max_value = max(max_value,
                                compare_noop(trs.target, LabelID(tr_s_label), t2, tau_distance, label_dominance));

                if (max_value >= min_value) {
                    continue; // Go to next transition
                }

                // is_relation_stable[s][t] = ts->applyPostSrc(t2,[&](const LTSTransition & trt) {
                // 	    if(is_relation_stable[trs.target][trt.target] || (trs.target == s && trt.target == t)) {
                // 		for(int tr_t_label : ts->get_labels(trt.label_group)) {
                // 		    max_value_stable = max(max_value_stable, compare_transitions(ts_id, trs.target, tr_s_label, trt.target, tr_t_label, tau_distance, label_dominance));
                // 		    if (max_value_stable == max_value) {
                // 			//break if we have found a transition that simulates with the best result possible
                // 			return true;
                // 		    }
                // 		}
                // 	    }
                // 	    return false;
                // 	});

                applyPostSrc(ts, t2, [&](const Transition &trt, LabelGroupID lg_id) {
                    for (int tr_t_label : ts.get_label_group(lg_id)) {
                        max_value = max(max_value,
                                        compare_transitions(trs.target, LabelID(tr_s_label), trt.target, LabelID(tr_t_label),
                                                            tau_distance, label_dominance));
                        if (max_value >= min_value) {
                            //break if we have found a transition that simulates with the best result possible
                            return true;
                        }
                    }
                    return false;
                });

                if (max_value >= min_value) {
                    break;
                }
            }

            /*if(min_value > std::numeric_limits<int>::lowest() && max_value == std::numeric_limits<int>::lowest()) {
              std::cout << ts->name(t) << " does not simulate "
              << ts->name(s) << " because of "
              << ts->name(trs.src) << " => "
              << ts->name(trs.target) << " ("
              << tr_s_label << ")"; // << std::endl;
              std::cout << "  Simulates? "
              << q_simulates(t, trs.target);
              std::cout << "  domnoop? "
              << label_dominance.may_dominated_by_noop(tr_s_label, ts_id) << "   " << endl;
              // label_dominance.dump(trs.label);
              // for (auto trt : ts->get_transitions(t)) {
              // std::cout << "Tried with: "
              // 	  << ts->name(trt.src) << " => "
              // 	  << ts->name(trt.target) << " ("
              // 	  << trt.label << ")" << " label dom: "
              // 	  << label_dominance.q_dominates(trt.label,
              // 					 trs.label, ts_id)
              // 	  << " target sim "
              // 	  << q_simulates(trt.target, trs.target)
              // 	  << std::endl;
              // }
              }*/
            min_value = std::min(min_value, max_value);
            if (min_value <= lower_bound) {
                return true;
            }
        }
        return false;
    });
    assert(min_value < std::numeric_limits<int>::max());

    min_value = std::max(min_value, lower_bound);

    // } else {
    // 	min_value = lower_bound;
    // }

    assert(min_value <= previous_value);


    if (min_value < previous_value) {
        //cout << "Updating " << ts->get_names()[s] << " <= " << ts->get_names()[t]
        // << " with " << min_value << " before " << previous_value << endl;


        update_value(t, s, min_value);
        return true;
    }
    return false;
}

template<typename T>
int NumericSimulationRelation<T>::update_pair(const NumericLabelRelation<T> &label_dominance,
                                              const TauDistances<T> &tau_distances,
                                              int s, int t) {
    assert (s != t && may_simulate(t, s));

    T lower_bound = tau_distances.minus_shortest_path(t, s);

    T previous_value = q_simulates(t, s);

    // cout << "prev: " << previous_value << endl;

    assert(lower_bound <= previous_value);
    if (lower_bound == previous_value) {
        return false;
    }

    T min_value = previous_value;
    // if (lts->is_goal(t) || !lts->is_goal(s)) {

    //Check if really t simulates s
    //for each transition s--l->s':
    // a) with noop t >= s' and l dominated by noop?
    // b) exist t--l'-->t', t' >= s' and l dominated by l'?
    applyPostSrc(ts, s, [&](const Transition &trs, LabelGroupID lg_id) {
        for (int tr_s_label : ts.get_label_group(lg_id)) {
            T max_value = std::numeric_limits<int>::lowest();
            for (int t2 : tau_distances.states_reachable_from(t)) {
                T tau_distance = tau_distances.minus_shortest_path(t, t2);

                max_value = max(max_value,
                                compare_noop(trs.target, LabelID(tr_s_label), t2, tau_distance, label_dominance));

                if (max_value >= min_value) {
                    continue; // Go to next transition
                }

                applyPostSrc(ts, t2, [&](const Transition &trt, LabelGroupID lg_id) {
                    for (int tr_t_label : ts.get_label_group(lg_id)) {
                        max_value = max(max_value,
                                        compare_transitions(trs.target, LabelID(tr_s_label), trt.target, LabelID(tr_t_label),
                                                            tau_distance, label_dominance));
                        if (max_value >= min_value) {
                            //break if we have found a transition that simulates with the best result possible
                            return true;
                        }
                    }
                    return false;
                });

                if (max_value >= min_value) {
                    break;
                }
            }

            /*if(min_value > std::numeric_limits<int>::lowest() && max_value == std::numeric_limits<int>::lowest()) {
              std::cout << lts->name(t) << " does not simulate "
              << lts->name(s) << " because of "
              << lts->name(trs.src) << " => "
              << lts->name(trs.target) << " ("
              << tr_s_label << ")"; // << std::endl;
              std::cout << "  Simulates? "
              << q_simulates(t, trs.target);
              std::cout << "  domnoop? "
              << label_dominance.may_dominated_by_noop(tr_s_label, lts_id) << "   " << endl;
              // label_dominance.dump(trs.label);
              // for (auto trt : lts->get_transitions(t)) {
              // std::cout << "Tried with: "
              // 	  << lts->name(trt.src) << " => "
              // 	  << lts->name(trt.target) << " ("
              // 	  << trt.label << ")" << " label dom: "
              // 	  << label_dominance.q_dominates(trt.label,
              // 					 trs.label, lts_id)
              // 	  << " target sim "
              // 	  << q_simulates(trt.target, trs.target)
              // 	  << std::endl;
              // }
              }*/
            min_value = std::min(min_value, max_value);

            if (min_value < -truncate_value) {
                min_value = lower_bound;
                return true;
            } else if (min_value <= lower_bound) {
                return true;
            }
        }
        return false;
    });
    assert(min_value < std::numeric_limits<int>::max());

    min_value = std::max(min_value, lower_bound);

    // } else {
    // 	min_value = lower_bound;
    // }

    assert(min_value <= previous_value);


    if (min_value < previous_value) {
        //cout << "Updating " << lts->get_names()[s] << " <= " << lts->get_names()[t]
        // << " with " << min_value << " before " << previous_value << endl;

        update_value(t, s, min_value);
        return true;
    }
    return false;
}


template<typename T>
int NumericSimulationRelation<T>::update(const NumericLabelRelation<T> &label_dominance,
                                         int max_time) {
    if (cancelled) {
        cancel_simulation_computation(); //check that tau-labels have not changed
        return 0;
    }

    assert(tau_labels);


    const auto &tau_distances = tau_labels->get_tau_distances(ts_id);

    int new_tau_distances_id = tau_distances.get_id();
    if (new_tau_distances_id != tau_distances_id) { //recompute_goal_respecting
        tau_distances_id = new_tau_distances_id;
        for (int s = 0; s < ts.get_size(); s++) {
            for (int t = 0; t < ts.get_size(); t++) { //for each pair of states t, s
                if (!ts.is_goal_state(t) && ts.is_goal_state(s)) {
                    if (tau_distances.get_goal_distance(t) == std::numeric_limits<int>::max()) {
                        update_value(t, s, std::numeric_limits<int>::lowest());
                    } else {
                        update_value(t, s, min(relation[s][t], -tau_distances.get_goal_distance(t)));
                    }
                }

            }
        }
    }

    utils::Timer timer;

    int num_iterations = 0;
    bool changes = true;
    while (changes) {
        num_iterations++;
        changes = false;
        for (int s = 0; s < ts.get_size(); s++) {
            for (int t = 0; t < ts.get_size(); t++) { //for each pair of states t, s
                if (timer() > max_time) {
                    cout << "Computation of numeric simulation on LTS " << ts_id
                         << " with " << ts.get_size()
                         << " states cancelled after " << timer() << " seconds." << endl;

                    cancel_simulation_computation();
                    return num_iterations;
                }

                // cout << "s: " << s << " t: " << t << endl;
                if (s != t && may_simulate(t, s)) { // && !is_relation_stable[s][t]
                    changes |= update_pair(label_dominance, tau_distances, s, t);
                }
            }
        }
    }

    return num_iterations;


    // for (int s = 0; s < ts->size(); s++) {
    // 	cout << g_fact_names[ts_id][s] << endl;
    // }
    //dump(g_fact_names[ts_id]);
}

template<typename T>
void NumericSimulationRelation<T>::dump(const vector<string> &names) const {
    cout << "SIMREL:" << endl;
    for (int j = 0; j < int(relation.size()); ++j) {
        for (int i = 0; i < int(relation.size()); ++i) {
            if (may_simulate(j, i) && i != j) {
                cout << names[i] << " <= " << names[j] << " (" << q_simulates(j, i) << ")" << endl;
            }
        }
    }
}

template<typename T>
void NumericSimulationRelation<T>::dump() const {
    cout << "SIMREL:" << endl;
    for (int j = 0; size_t(j) < relation.size(); ++j) {
        for (int i = 0; size_t(i) < relation.size(); ++i) {
            cout << q_simulates(j, i) << " ";
        }
        cout << endl;
    }
}


template<typename T>
bool NumericSimulationRelation<T>::has_dominance() const {
    for (int j = 0; size_t(j) < relation.size(); ++j) {
        for (int i = 0; size_t(i) < relation.size(); ++i) {
            if (i == j) continue;
            if (relation[i][j] > std::numeric_limits<int>::lowest()) {
                return true;
            }
        }
    }

    return false;
}


template<typename T>
bool NumericSimulationRelation<T>::has_positive_dominance() const {
    for (int j = 0; size_t(j) < relation.size(); ++j) {
        for (int i = 0; size_t(i) < relation.size(); ++i) {
            if (i == j) continue;
            if (relation[i][j] >= 0) {
                return true;
            }
        }
    }

    return false;
}

template<typename T>
void NumericSimulationRelation<T>::statistics() const {
    map<T, int> values;
    for (int j = 0; size_t(j) < relation.size(); ++j) {
        for (int i = 0; size_t(i) < relation.size(); ++i) {
            if (i == j) continue;
            // if (relation[i][j] > std::numeric_limits<int>::lowest()) {
            if (values.count(relation[i][j])) {
                values[relation[i][j]]++;
            } else {
                values[relation[i][j]] = 1;
            }
            // }
        }
    }

    for (auto &it : values) {
        if (it.first == std::numeric_limits<int>::lowest()) {
            cout << "-infinity";
        } else {
            cout << it.first;
        }

        cout << ": " << it.second << endl;
    }
}

template
class NumericSimulationRelation<int>;

template
class NumericSimulationRelation<IntEpsilon>;
