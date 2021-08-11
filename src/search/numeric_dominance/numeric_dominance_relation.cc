#include "numeric_dominance_relation.h"

#include "numeric_simulation_relation.h"
#include "numeric_label_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../search_progress.h"
#include "../task_representation/state.h"
#include "../task_representation/search_task.h"

#include <memory>

using namespace std;

template<typename T>
void NumericDominanceRelationBuilder<T>::init() {
    ndr->simulations.clear();

    ndr->simulation_of_variable.resize(tss.size(), 0);
    for (size_t ts_id = 0; ts_id < tss.size(); ++ts_id) {
        ndr->simulations.push_back(init_simulation(ts_id));
    }

    ndr->initial_state.resize(tss.size());
    set_initial_state();
}

template<typename T>
std::unique_ptr<NumericSimulationRelation<T>> NumericDominanceRelationBuilder<T>::init_simulation(int ts_id) {
    auto res = std::make_unique<NumericSimulationRelation<T>>(tss[ts_id], ts_id, truncate_value, ndr->tau_labels);
    res->init_goal_respecting();
    return res;
}

// int NumericDominanceRelation<T>::get_cost(const State &state) const{
//     int cost = 0;
//     for(auto & sim : simulations) {
// 	int new_cost = sim->get_cost(state);
// 	if (new_cost == -1) return -1;
// 	cost = max (cost, new_cost);
//     }
//     return cost;
// }



// bool NumericDominanceRelation<T>::parent_dominates_successor(const State & parent,
// 							  const Operator *op) const {

//     for(const auto & sim : simulations) {
// 	int val = sim->q_simulates(t, s);
// 	// cout << val << endl;
// 	if(val == std::numeric_limits<int>::lowest()) {
// 	    return false;
// 	}
// 	if(val < 0) {
// 	    sum_negatives += val;
// 	} else {
// 	    max_positive = std::max(max_positive, val);
// 	}

//     }
// }

template<typename T>
bool NumericDominanceRelation<T>::dominates(const State &t, const State &s, int g_diff) const {
    T total_value = 0;
    for (size_t i = 0; i < size_t(num_transition_systems); ++i) {
        assert(simulations[i]);

        T val = simulations[i]->q_simulates(t[i], s[i]);
        if (val == std::numeric_limits<int>::lowest()) {
            return false;
        }
        total_value += val;
    }

    return total_value - g_diff >= 0;

}

template<typename T>
bool NumericDominanceRelation<T>::strictly_dominates(const State &t, const State &s) const {
    return dominates(t, s, 0) && !dominates(s, t, 0);
}


template<typename T>
bool NumericDominanceRelation<T>::strictly_dominates_initial_state(const State &t) const {
    vector<int> copy_t(num_transition_systems);
    for (size_t i = 0; i < size_t(num_transition_systems); ++i) {
        copy_t[i] = t[i];
    }
    return dominates_parent(copy_t, initial_state, 0) &&
           !dominates_parent(initial_state, copy_t, 0);
}

template<typename T>
bool NumericDominanceRelation<T>::dominates_parent(const vector<int> &state,
                                                   const vector<int> &parent_state,
                                                   int action_cost) const {
    T total_value = 0;
    for (size_t i = 0; i < simulations.size(); ++i) {
        T val = simulations[i]->q_simulates(state[i], parent_state[i]);
        if (val == std::numeric_limits<int>::lowest()) {
            return false;
        }
        total_value += val;
    }

    return total_value - action_cost >= 0;
}

template<typename T>
std::shared_ptr<NumericDominanceRelation<T>>
NumericDominanceRelationBuilder<T>::compute_ld_simulation(bool dump) {
    assert(ndr); // If this is NULL, this dominance relation builder has probably already been used
    assert(tss.size() == ndr->simulations.size());
    utils::Timer t;
    int num_iterations = 0;
    int num_inner_iterations = 0;

    if (dump) std::cout << "Compute numLDSim on " << tss.size() << " TSs." << std::endl;

    if (dump) std::cout << "Compute tau labels" << std::endl;
    ndr->tau_labels->initialize(tss, labels);
    ndr->label_relation.init(tss, *ndr);
    for (int i = 0; i < int(ndr->simulations.size()); i++) {
        if (tss[i].get_size() > max_lts_size_to_compute_simulation) {
            if (dump)
                std::cout << "Computation of numeric simulation on LTS " << i <<
                          " with " << tss[i].get_size() << " states cancelled because it is too big." << std::endl;
            ndr->simulations[i]->cancel_simulation_computation();
        }
    }

    std::vector<int> order_by_size;
    for (int i = 0; i < int(ndr->simulations.size()); i++) {
        order_by_size.push_back(i);
    }

    std::sort(order_by_size.begin(), order_by_size.end(), [&](int a, int b) {
        return tss[a].get_size() < tss[b].get_size();
    });
    std::cout << "  Init numLDSim in " << t() << "s: " << std::flush;
    bool restart = false;
    do {
        do {
            num_iterations++;
            //label_relation.dump();
            int remaining_to_compute = int(order_by_size.size());
            for (int i : order_by_size) {
                /* std::cout << "Updating " << i << " of size " <<   _ltss[i]->size() << " states and " */
                /*           <<  _ltss[i]->num_transitions() << " transitions" << std::endl; */

                int max_time = std::max(max_simulation_time,
                                        std::min(min_simulation_time,
                                                 1 + max_total_time / remaining_to_compute--));
                num_inner_iterations += ndr->simulations[i]->update(ndr->label_relation, max_time);
                //_dominance_relation[i]->dump(_ltss[i]->get_names());
            }
            std::cout << " " << t() << "s" << std::flush;
        } while (ndr->label_relation.update(tss, *ndr));
        restart = ndr->tau_labels->add_noop_dominance_tau_labels(tss, ndr->label_relation);
        if (restart) {
            for (int i : order_by_size) {
                ndr->simulations[i]->init_goal_respecting();
            }
        }
    } while (restart);
    std::cout << std::endl << "Numeric LDSim computed " << t() << std::endl;
    std::cout << "Numeric LDSim outer iterations: " << num_iterations << std::endl;
    std::cout << "Numeric LDSim inner iterations: " << num_inner_iterations << std::endl;

    if (dump) {

    }

    if (dump) {
        std::cout << "" << "------" << std::endl;
        for (int i = 0; i < int(tss.size()); i++) {

            ndr->simulations[i]->statistics();
            std::cout << "------" << std::endl;
        }

        std::cout << "------" << std::endl;
        for (int i = 0; i < int(tss.size()); i++) {
//            simulations[i]->dump(tss[i]->get_names());
            std::cout << "------" << std::endl;
            ndr->label_relation.dump(tss[i], i);
        }
    }
    //label_relation.dump_equivalent();
    //label_relation.dump_dominance();
    //exit(0);
    //}


    ndr->total_max_value = 0;
    for (auto &sim : ndr->simulations) {
        ndr->total_max_value += sim->compute_max_value();
    }

    return move(ndr);
}

template<typename T>
bool NumericDominanceRelation<T>::action_selection_pruning(const shared_ptr<FTSTask> fts_task, const State &state,
                                                           std::vector<OperatorID> &applicable_operators) const {
    const shared_ptr<SearchTask> &search_task = fts_task->get_search_task();

    for (int i = 0; i < num_transition_systems; ++i) {
        parent[i] = state[i];
    }

    succ = parent;
    for (auto op_id : applicable_operators) {
        FTSOperator fts_op = search_task->get_fts_operator(op_id);

        succ = search_task->generate_successor(state, op_id);

        for (size_t i = 0; i < succ.size(); i++) {
            if (succ[i] != parent[i])
                relevant_simulations.insert(int(i));
        }

        T total_value = 0;
        bool may_simulate = true;
        for (int sim :  relevant_simulations) {
            // TODO: Succ_id is the same as succ as there is no transformation on the task
            int succ_id = succ[simulations[sim]->get_ts_id()];
            if (succ_id == -1) {
                may_simulate = false;
                break;
            }
            T val = simulations[sim]->q_simulates(succ_id, parent[sim]);
            if (val == std::numeric_limits<int>::lowest()) {
                may_simulate = false;
                break;
            }
            total_value += val;
        }
        relevant_simulations.clear();

        //TODO: Use adjusted cost instead.
        if (may_simulate && total_value - fts_op.get_cost() >= 0) {
//            search_progress.inc_action_selection(applicable_operators.size() - 1);
            applicable_operators.clear();
            applicable_operators.push_back(op_id);
            return true;
        }

//        for (const auto &eff : fts_op.get_effects()) {
//            succ[eff.var] = parent[eff.var];
//        }

    }

    return false;
}

template<typename T>
void NumericDominanceRelation<T>::prune_dominated_by_parent_or_initial_state(const shared_ptr<FTSTask> fts_task,
                                                                             const State &state,
                                                                             std::vector<OperatorID> &applicable_operators,
                                                                             bool parent_ids_stored,
                                                                             bool compare_against_parent,
                                                                             bool compare_against_initial_state) const {

    const shared_ptr<SearchTask> &search_task = fts_task->get_search_task();

    if (!parent_ids_stored) {
        for (int i = 0; i < num_transition_systems; ++i) {
            succ[i] = state[i];
        }
        if (compare_against_parent) {
            parent = succ;
        }
    }

    vector<int> ts_initial_state_does_not_simulate_parent;
    T initial_state_against_parent = 0;
    if (compare_against_initial_state) {
        for (size_t i = 0; i < simulations.size(); ++i) {
            values_initial_state_against_parent[i] =
                    simulations[i]->q_simulates(initial_state[i], parent[i]);
            if (values_initial_state_against_parent[i] == std::numeric_limits<int>::lowest()) {
                ts_initial_state_does_not_simulate_parent.push_back(int(i));
            } else {
                initial_state_against_parent += values_initial_state_against_parent[i];
            }
        }
    }

    int detected_dead_ends = 0;
//    int ops_before = int(applicable_operators.size());
    applicable_operators.erase(std::remove_if(applicable_operators.begin(),
                                              applicable_operators.end(),
                                              [&](const OperatorID &op_id) {
                                                  FTSOperator fts_op = search_task->get_fts_operator(op_id);

                                                  succ = search_task->generate_successor(state, op_id);

                                                  for (size_t i = 0; i < succ.size(); i++) {
                                                      if (succ[i] != parent[i])
                                                          relevant_simulations.insert(int(i));
                                                  }

                                                  bool proved_prunable = false;

                                                  //Check dead_ends
                                                  for (int sim :  relevant_simulations) {
                                                      if (succ[sim] == -1) {
                                                          detected_dead_ends++;
                                                          proved_prunable = true;
                                                      }
                                                  }

                                                  if (!proved_prunable && compare_against_parent) {
                                                      T total_value = 0;
                                                      bool may_simulate = true;
                                                      for (int sim :  relevant_simulations) {
                                                          T val = simulations[sim]->q_simulates(parent[sim], succ[sim]);

                                                          if (val == std::numeric_limits<int>::lowest()) {
                                                              may_simulate = false;
                                                              break;
                                                          }
                                                          total_value += val;
                                                      }

                                                      proved_prunable = may_simulate && (total_value >= 0 ||
                                                                                         total_value +
                                                                                         fts_op.get_cost() > 0);
                                                  }

                                                  if (!proved_prunable && compare_against_initial_state
                                                      && ts_initial_state_does_not_simulate_parent.size() <=
                                                         relevant_simulations.size()) {

                                                      bool all_not_simulated_change = true;
                                                      for (int sim_must_change :  ts_initial_state_does_not_simulate_parent) {
                                                          bool found = false;
                                                          for (int sim : relevant_simulations) {
                                                              if (sim_must_change == sim) {
                                                                  found = true;
                                                                  break;
                                                              }
                                                          }
                                                          if (!found) {
                                                              all_not_simulated_change = false;
                                                              break; //proved no
                                                          }
                                                      }

                                                      if (all_not_simulated_change) {
                                                          T total_value = initial_state_against_parent;
                                                          bool may_simulate = true;
                                                          for (int sim :  relevant_simulations) {
                                                              T val = simulations[sim]->q_simulates(initial_state[sim],
                                                                                                    succ[sim]);
                                                              if (val == std::numeric_limits<int>::lowest()) {
                                                                  may_simulate = false;
                                                                  break;
                                                              }
                                                              total_value += val;
                                                              if (values_initial_state_against_parent[sim] !=
                                                                  std::numeric_limits<int>::lowest()) {
                                                                  total_value -= values_initial_state_against_parent[sim];
                                                              }
                                                          }
                                                          proved_prunable = may_simulate && (total_value >= 0 ||
                                                                                             total_value +
                                                                                             fts_op.get_cost() > 0);
                                                      }
                                                  }

                                                  relevant_simulations.clear();

                                                  //TODO: Use adjusted cost instead.
                                                  return proved_prunable;
                                              }), applicable_operators.end());



//    if (ops_before > applicable_operators.size()) {
//        search_progress.inc_dead_ends(detected_dead_ends);
//        search_progress.inc_pruned((ops_before - applicable_operators.size()) - detected_dead_ends);
//        //cout << "Pruned "  << ops_before  -applicable_operators.size() << " out of " << ops_before << endl;
//    }
}

template<typename T>
void NumericDominanceRelationBuilder<T>::set_initial_state() {
    for (size_t i = 0; i < tss.size(); ++i) {
        ndr->initial_state[i] = tss[i].get_init_state();
    }
}

/* Returns true if we succeeded in propagating the effects of pruning a transition in ts i. */
template<typename T>
bool NumericDominanceRelation<T>::propagate_transition_pruning(int ts_id,
                                                               const TransitionSystem &ts,
                                                               int src, LabelID l_id, int target) const {
    const NumericSimulationRelation<T> &nsr = *simulations[ts_id];

    vector<bool> Tlbool(ts.get_size(), false), Tlpbool(ts.get_size(), false);
    vector<int> Tl, Tlp;

    bool still_simulates_irrelevant = !(label_relation.get_label_simulates_irrelevant(l_id, ts_id) >= 0);

    //For each transition from src, check if anything has changed
    applyPostSrc(ts, src, [&](const Transition &tr, LabelGroupID lg_id) {
        for (int tr_label : ts.get_label_group(lg_id)) {
            if (l_id == tr_label) { //Same label
                if (tr.target == target) {
                    continue;
                }
                if (!still_simulates_irrelevant && nsr.simulates(tr.target, tr.src)) {
                    //There is another transition with the same label which simulates noop
                    still_simulates_irrelevant = true;
                }
                if (!Tlbool[tr.target]) {
                    Tl.push_back(tr.target);
                    Tlbool[tr.target] = true;
                }
            } else if (label_relation.may_simulate(ts.get_label_group_id_of_label(l_id),
                                                   ts.get_label_group_id_of_label(LabelID(tr_label)), ts_id)
                                                   && nsr.simulates(target, tr.target)) {
                if (!Tlpbool[tr.target]) {
                    Tlp.push_back(tr.target);
                    Tlpbool[tr.target] = true;
                }
            }
        }
        return false;
    });
    if (!still_simulates_irrelevant) {
        return false;
    }
    for (int t : Tlp) {
        if (!Tlbool[t] &&
            find_if(begin(Tl), end(Tl), [&](int t2) {
                return nsr.simulates(t2, t);
            }) == end(Tl)) {
            return false;
        }
    }
    return true;
}

template
class NumericDominanceRelationBuilder<int>;

template
class NumericDominanceRelationBuilder<IntEpsilon>;

template
class NumericDominanceRelation<int>;

template
class NumericDominanceRelation<IntEpsilon>;
