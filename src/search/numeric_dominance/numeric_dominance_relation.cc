#include "numeric_dominance_relation.h"

#include "numeric_simulation_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../search_progress.h"
#include "../task_representation/state.h"
#include "../task_representation/search_task.h"

#include <memory>

using namespace std;

template<typename T>
void NumericDominanceRelation<T>::init() {
    simulations.clear();

    simulation_of_variable.resize(task->get_size(), 0);
    for (int ts_id = 0; ts_id < task->get_size(); ++ts_id) {
        simulations.push_back(init_simulation(ts_id));
    }
    parent.resize(task->get_size());
    parent_ids.resize(simulations.size());
    succ.resize(task->get_size());
    succ_ids.resize(simulations.size());
    values_initial_state_against_parent.resize(simulations.size());

    initial_state.resize(task->get_size());
    initial_state_ids.resize(task->get_size());

    set_initial_state(task->get_initial_state());
}

template<typename T>
std::unique_ptr<NumericSimulationRelation<T>> NumericDominanceRelation<T>::init_simulation(int ts_id) {
    auto res = std::make_unique<NumericSimulationRelation<T>>(task->get_ts(ts_id), ts_id, truncate_value, tau_labels);
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
    for (int i = 0; i < task->get_size(); ++i) {
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
    vector<int> copy_t(task->get_size());
    for (int i = 0; i < task->get_size(); ++i) {
        copy_t[i] = t[i];
    }
    return dominates_parent(copy_t, task->get_initial_state(), 0) &&
            !dominates_parent(task->get_initial_state(), copy_t, 0);
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
void
NumericDominanceRelation<T>::compute_ld_simulation(vector<TransitionSystem> &tss, const Labels &labels, bool dump) {
    assert(tss.size() == simulations.size());
    utils::Timer t;
    int num_iterations = 0;
    int num_inner_iterations = 0;

    std::cout << "Compute numLDSim on " << tss.size() << " TSs." << std::endl;
    for (auto &ts : tss) {
        std::cout << ts.get_size() << " states and " << std::endl;
    }

    std::cout << "Compute tau labels" << std::endl;
    tau_labels->initialize(tss, labels);
    label_dominance.init(tss, *this);
    for (int i = 0; i < int(simulations.size()); i++) {
        if (tss[i].get_size() > max_lts_size_to_compute_simulation) {
            std::cout << "Computation of numeric simulation on LTS " << i <<
                      " with " << tss[i].get_size() << " states cancelled because it is too big." << std::endl;
            simulations[i]->cancel_simulation_computation();
        }
    }

    std::vector<int> order_by_size;
    for (int i = 0; i < int(simulations.size()); i++) {
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
            //label_dominance.dump();
            int remaining_to_compute = int(order_by_size.size());
            for (int i : order_by_size) {
                /* std::cout << "Updating " << i << " of size " <<   _ltss[i]->size() << " states and " */
                /*           <<  _ltss[i]->num_transitions() << " transitions" << std::endl; */

                int max_time = std::max(max_simulation_time,
                                        std::min(min_simulation_time,
                                                 1 + max_total_time / remaining_to_compute--));
                num_inner_iterations += simulations[i]->update(label_dominance, max_time);
                //_dominance_relation[i]->dump(_ltss[i]->get_names());
            }
            std::cout << " " << t() << "s" << std::flush;
        } while (label_dominance.update(tss, *this));
        restart = tau_labels->add_noop_dominance_tau_labels(tss, label_dominance);
        if (restart) {
            for (int i : order_by_size) {
                simulations[i]->init_goal_respecting();
            }
        }
    } while (restart);
    std::cout << std::endl << "Numeric LDSim computed " << t() << std::endl;
    std::cout << "Numeric LDSim outer iterations: " << num_iterations << std::endl;
    std::cout << "Numeric LDSim inner iterations: " << num_inner_iterations << std::endl;

    std::cout << "------" << std::endl;
    for (int i = 0; i < int(tss.size()); i++) {
        simulations[i]->statistics();
        std::cout << "------" << std::endl;
    }

    if (dump) {
        std::cout << "------" << std::endl;
        for (int i = 0; i < int(tss.size()); i++) {
//            simulations[i]->dump(tss[i]->get_names());
            std::cout << "------" << std::endl;
            label_dominance.dump(tss[i], i);
        }
    }
    //label_dominance.dump_equivalent();
    //label_dominance.dump_dominance();
    //exit(0);
    //}


    total_max_value = 0;
    for (auto &sim : simulations) {
        total_max_value += sim->compute_max_value();
    }
}

template<typename T>
bool NumericDominanceRelation<T>::action_selection_pruning(const State &state,
                                                           std::vector<OperatorID> &applicable_operators) const {
    const shared_ptr<SearchTask>& search_task = task->get_search_task();

    for (int i = 0; i < task->get_size(); ++i) {
        parent[i] = state[i];
    }
    for (size_t i = 0; i < simulations.size(); ++i) {
        // TODO: Parent_ids are the same as parent as there is no transformation on the task!
        parent_ids[i] = parent[i];
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
            T val = simulations[sim]->q_simulates(succ_id, parent_ids[sim]);
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
void NumericDominanceRelation<T>::prune_dominated_by_parent_or_initial_state(const State &state,
                                           std::vector<OperatorID> &applicable_operators, bool parent_ids_stored,
                                           bool compare_against_parent,
                                           bool compare_against_initial_state) const {

    const shared_ptr<SearchTask>& search_task = task->get_search_task();

    if (!parent_ids_stored) {
        for (int i = 0; i < task->get_size(); ++i) {
            succ[i] = state[i];
        }
        if (compare_against_parent) {
            parent = succ;
            for (size_t i = 0; i < simulations.size(); ++i) {
                // TODO: parent_id is same as parent as there is no transformation on task
                parent_ids[i] = parent[i];
            }
        }
    }

    vector<int> ts_initial_state_does_not_simulate_parent;
    T initial_state_against_parent = 0;
    if (compare_against_initial_state) {
        for (size_t i = 0; i < simulations.size(); ++i) {
            values_initial_state_against_parent[i] =
                    simulations[i]->q_simulates(initial_state_ids[i], parent_ids[i]);
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
        [&](const OperatorID& op_id) {
            FTSOperator fts_op = search_task->get_fts_operator(op_id);

            succ = search_task->generate_successor(state, op_id);

            for (size_t i = 0; i < succ.size(); i++) {
                if (succ[i] != parent[i])
                    relevant_simulations.insert(int(i));
            }

            bool proved_prunable = false;

            //Check dead_ends
            for (int sim :  relevant_simulations) {
                // TODO: succ_ids are same as succ because there is no task transformation
                succ_ids[sim] = succ[simulations[sim]->get_ts_id()];
                if (succ_ids[sim] == -1) {
                    detected_dead_ends++;
                    proved_prunable = true;
                }
            }

            if (!proved_prunable && compare_against_parent) {
                T total_value = 0;
                bool may_simulate = true;
                for (int sim :  relevant_simulations) {
                    T val = simulations[sim]->q_simulates(parent_ids[sim], succ_ids[sim]);

                    if (val == std::numeric_limits<int>::lowest()) {
                        may_simulate = false;
                        break;
                    }
                    total_value += val;
                }

                proved_prunable = may_simulate && (total_value >= 0 || total_value + fts_op.get_cost() > 0);
            }

            if (!proved_prunable && compare_against_initial_state
                    && ts_initial_state_does_not_simulate_parent.size() <= relevant_simulations.size()) {

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
                        T val = simulations[sim]->q_simulates(initial_state_ids[sim], succ_ids[sim]);
                        if (val == std::numeric_limits<int>::lowest()) {
                            may_simulate = false;
                            break;
                        }
                        total_value += val;
                        if (values_initial_state_against_parent[sim] != std::numeric_limits<int>::lowest()) {
                            total_value -= values_initial_state_against_parent[sim];
                        }
                    }
                    proved_prunable = may_simulate && (total_value >= 0 || total_value + fts_op.get_cost() > 0);
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

template <typename T> void NumericDominanceRelation<T>::set_initial_state(const std::vector<int>& state) {
    for(int i = 0; i < task->get_size(); ++i) {
        initial_state[i] = state[i];
    }
    for(size_t i = 0; i < simulations.size(); ++i) {
        // TODO: Initial state ids are the same as initial state because there is no transformation on task
        initial_state_ids[i] = initial_state[i];
    }
}

template
class NumericDominanceRelation<int>;

template
class NumericDominanceRelation<IntEpsilon>;
