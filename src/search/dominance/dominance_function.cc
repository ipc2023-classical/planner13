#include "dominance_function.h"

#include "local_dominance_function.h"
#include "label_dominance_function.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../search_progress.h"
#include "../task_representation/state.h"
#include "../task_representation/search_task.h"

#include <memory>

using namespace std;
namespace dominance {

// int DominanceFunction<T>::get_cost(const State &state) const{
//     int cost = 0;
//     for(auto & sim : local_functions) {
// 	int new_cost = sim->get_cost(state);
// 	if (new_cost == -1) return -1;
// 	cost = max (cost, new_cost);
//     }
//     return cost;
// }



// bool DominanceFunction<T>::parent_dominates_successor(const State & parent,
// 							  const Operator *op) const {

//     for(const auto & sim : local_functions) {
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
    bool DominanceFunction<T>::dominates(const State &t, const State &s, int g_diff) const {
        T total_value = 0;
        for (size_t i = 0; i < local_functions.size(); ++i) {
            assert(local_functions[i]);

            T val = local_functions[i]->q_simulates(t[i], s[i]);
            if (val == std::numeric_limits<int>::lowest()) {
                return false;
            }
            total_value += val;
        }

        return total_value - g_diff >= 0;

    }

    template<typename T>
    bool DominanceFunction<T>::strictly_dominates(const State &t, const State &s) const {
        return dominates(t, s, 0) && !dominates(s, t, 0);
    }

    template<typename T>
    bool DominanceFunction<T>::dominates_parent(const vector<int> &state,
                                                const vector<int> &parent_state,
                                                int action_cost) const {
        T total_value = 0;
        for (size_t i = 0; i < local_functions.size(); ++i) {
            T val = local_functions[i]->q_simulates(state[i], parent_state[i]);
            if (val == std::numeric_limits<int>::lowest()) {
                return false;
            }
            total_value += val;
        }

        return total_value - action_cost >= 0;
    }


/* Returns true if we succeeded in propagating the effects of pruning a transition in ts i. */
    template<typename T>
    bool DominanceFunction<T>::propagate_transition_pruning(int ts_id,
                                                            const TransitionSystem &ts,
                                                            int src, LabelID l_id, int target) const {
        const LocalDominanceFunction<T> &nsr = *local_functions[ts_id];

        vector<bool> Tlbool(ts.get_size(), false);
        vector<bool> Tlpbool(ts.get_size(), false);
        vector<int> Tl, Tlp;

        bool still_simulates_irrelevant = !(label_relation.get_label_simulates_irrelevant(l_id, ts_id) >= 0);

        //For each transition from src, check if anything has changed
        applyPostSrc(ts, src, [&](const Transition &tr, LabelGroupID lg_id) {
            for (int tr_label: ts.get_label_group(lg_id)) {
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
        for (int t: Tlp) {
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
    class DominanceFunction<int>;

    template
    class DominanceFunction<IntEpsilon>;
}
