#include "label_dominance_function.h"
#include "local_dominance_function.h"
#include "dominance_function.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../globals.h"

using namespace std;
namespace dominance {
    template<typename T>
    LabelDominanceFunction<T>::LabelDominanceFunction(const Labels &_labels) :
            labels(_labels), num_labels(_labels.get_size()) {
    }

    template<typename T>
    bool
    LabelDominanceFunction<T>::update(int ts_id, const TransitionSystem &ts, const LocalDominanceFunction<T> &sim) {
        bool changes = false;
        for (LabelGroupID lg2_id(0); lg2_id < ts.num_label_groups(); ++lg2_id) {
            if (ts.get_label_group(lg2_id).empty())
                continue;

            for (LabelGroupID lg1_id(0); lg1_id < ts.num_label_groups(); ++lg1_id) {
                if (lg1_id != lg2_id && !ts.get_label_group(lg1_id).empty() && may_simulate(lg1_id, lg2_id, ts_id)) {
                    T min_value = std::numeric_limits<int>::max();
                    //std::cout << "Check " << l1 << " " << l2 << std::endl;
                    //std::cout << "Num transitions: " << ts->get_transitions_label(l1).size()
                    //		    << " " << ts->get_transitions_label(l2).size() << std::endl;
                    //Check if it really simulates
                    //For each transition s--l2-->t, and every label l1 that dominates
                    //l2, exist s--l1-->t', t <= t'?

                    assert(!ts.get_transitions_for_group_id(lg2_id).empty());
                    for (const auto &tr_i: ts.get_transitions_for_group_id(lg2_id)) {
                        T max_value = std::numeric_limits<int>::lowest();
                        //TODO: for(auto tr2 : ts->get_transitions_for_label_src(l1, tr_i.src)){
                        for (const auto &tr_j: ts.get_transitions_for_group_id(lg1_id)) {
                            if (tr_j.src == tr_i.src && sim.may_simulate(tr_j.target, tr_i.target)) {
                                max_value = std::max(max_value, sim.q_simulates(tr_j.target, tr_i.target));
                                if (max_value >= min_value) {
                                    break; //Stop checking this tr_i
                                }
                            }
                        }
                        min_value = std::min(min_value, max_value);
                        if (min_value == std::numeric_limits<int>::lowest()) {
                            break; //Stop checking trs of l1, l2
                        }
                    }

                    changes |= set_lqrel(lg1_id, lg2_id, ts_id, ts, min_value);
                    assert(min_value != std::numeric_limits<int>::max());
                }
            }

            //Is l2 simulated by irrelevant_labels in ts?
            T old_value = get_simulated_by_irrelevant(lg2_id, ts_id);
            if (old_value != T(std::numeric_limits<int>::lowest())) {
                T min_value = std::numeric_limits<int>::max();
                for (const auto &tr: ts.get_transitions_for_group_id(lg2_id)) {
                    min_value = std::min(min_value, sim.q_simulates(tr.src, tr.target));
                    if (min_value == std::numeric_limits<int>::lowest()) {
                        break;
                    }
                }

                assert(min_value != std::numeric_limits<int>::max());

                if (min_value < old_value) {
                    changes |= set_simulated_by_irrelevant(lg2_id, ts_id, ts, min_value);
                    // for (int l : ts->get_irrelevant_labels()){
                    //     changes |= set_lqrel(l, l2, ts_id, min_value);
                    // }
                    old_value = min_value;
                }
            }

            //Does l2 simulate irrelevant_labels in ts?
            old_value = get_simulates_irrelevant(lg2_id, ts_id);
            if (old_value != std::numeric_limits<int>::lowest()) {
                T min_value = std::numeric_limits<int>::max();
                for (int s = 0; s < ts.get_size(); s++) {
                    T max_value = std::numeric_limits<int>::lowest();
                    for (const auto &tr: ts.get_transitions_for_group_id(lg2_id)) {
                        if (tr.src == s) {
                            max_value = std::max(max_value, sim.q_simulates(tr.target, tr.src));
                            if (max_value > min_value) {
                                break;
                            }
                        }
                    }
                    min_value = std::min(min_value, max_value);
                }
                assert(min_value != std::numeric_limits<int>::max());
                if (min_value < old_value) {
                    old_value = min_value;
                    changes |= set_simulates_irrelevant(lg2_id, ts_id, ts, min_value);
                    // for (int l : ts->get_irrelevant_labels()){
                    //     changes |= set_lqrel(l2, l, ts_id, min_value);
                    // }
                }
            }
        }

        // for (int l : ts->get_irrelevant_labels()) {
        // 	set_simulates_irrelevant(l, ts_id, 0);
        // 	set_simulated_by_irrelevant(l, ts_id, 0);
        // }

        return changes;
    }


    template<typename T>
    void LabelDominanceFunction<T>::dump(const TransitionSystem &ts, int ts_id) const {
        int count = 0;
        for (LabelID l2(0); l2 < int(labels.get_size()); ++l2) {
            if (!ts.is_relevant_label(l2)) continue;

            for (LabelID l1(0); l1 < int(labels.get_size()); ++l1) {
                if (!ts.is_relevant_label(l1)) continue;

                if (l2 == l1) {
                    continue;
                }
                if (may_dominate(l2, l1, ts_id)) {
                    cout << l1 << " <= " << l2 << " with " << q_dominates(l2, l1, ts_id) << endl;
                    count++;
                }
            }
            if (may_dominated_by_noop(l2, ts_id)) {
                cout << l2 << " dominates noop: " << q_dominated_by_noop(l2, ts_id) << endl;
                count++;
            }
            if (may_dominate_noop_in(l2, ts_id)) {
                cout << l2 << " dominated by noop: " << q_dominates_noop(l2, ts_id) << endl;
                count++;
            }
        }
        cout << "Numeric label relation " << ts_id << " total count: " << count << endl;

    }


    template
    class LabelDominanceFunction<int>;

    template
    class LabelDominanceFunction<IntEpsilon>;

}