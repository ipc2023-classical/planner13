#include "label_dominance_function.h"
#include "local_dominance_function.h"
#include "dominance_function.h"
#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../globals.h"
#include "int_epsilon.h"

using namespace std;
namespace dominance {
    template<typename TCost>
    LabelDominanceFunction<TCost>::LabelDominanceFunction(const task_representation::Labels &_labels) :
            labels(_labels), num_labels(_labels.get_size()) {
    }

    //Initializes label relation (only the first time, to reinitialize call reset instead
    // If use_may_dominate is true, it will use the summary tables. This speeds up a little bit dominance checks but
    // requires memory quadratic in the number of labels so it is not recommended if there are more than 1000 labels.
    template<typename TCost>
    void LabelDominanceFunction<TCost>::init(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
              const std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> & local_dominance_functions, bool use_may_dominate) {
        num_labels = labels.get_size();
        num_tss = int(tss.size());

        std::cout << "Init label dominance: " << num_labels << " labels " << tss.size() << " systems.\n";


        std::vector<TCost>().swap(cost_of_label);
        std::vector<std::vector<task_representation::LabelGroupID> >().swap(ts_label_id_to_label_group_id);
        std::vector<std::vector<std::vector<TCost> > >().swap(lqrel);
        std::vector<std::vector<TCost> >().swap(simulates_irrelevant);
        std::vector<std::vector<TCost> >().swap(simulated_by_irrelevant);

        irrelevant_label_groups_ts.resize(tss.size());
        ts_label_id_to_label_group_id.resize(tss.size());
        simulates_irrelevant.resize(tss.size());
        simulated_by_irrelevant.resize(tss.size());
        lqrel.resize(tss.size());

        cost_of_label.resize(num_labels);
        for (task_representation::LabelID l(0); l < num_labels; ++l) {
            cost_of_label[l] = labels.get_label_cost(l);
        }

        for (int i = 0; i < num_tss; ++i) {

            int num_label_groups = tss[i]->num_label_groups();

            // Compute the map from label_id to label group id for this transition system
            ts_label_id_to_label_group_id[i].resize(labels.get_size());
            for (task_representation::LabelID l_id(0); l_id < labels.get_size(); ++l_id) {
                ts_label_id_to_label_group_id[i][l_id] = tss[i]->get_label_group_id_of_label(l_id);
            }

            // Compute irrelevant label groups for this ts
            for (task_representation::LabelGroupID lg_id(0); lg_id < num_label_groups; ++lg_id) {
                if (!tss[i]->is_relevant_label_group(lg_id)) {
                    irrelevant_label_groups_ts[i].push_back(lg_id);
                }
            }

            /* std::cout << "Relevant label groups: " << num_label_groups << "\n"; */
            simulates_irrelevant[i].resize(num_label_groups, std::numeric_limits<int>::max());
            simulated_by_irrelevant[i].resize(num_label_groups, std::numeric_limits<int>::max());
            lqrel[i].resize(num_label_groups);

            for (int j = 0; j < num_label_groups; j++) {
                lqrel[i][j].resize(num_label_groups, std::numeric_limits<int>::max());
                lqrel[i][j][j] = 0;
            }
        }

        std::cout << "Dominating.\n";
        std::vector<std::vector<int> >().swap(_may_dominate_in);
        std::vector<int>().swap(_may_dominated_by_noop_in);
        std::vector<int>().swap(_may_dominates_noop_in);
        _may_dominated_by_noop_in.resize(num_labels, DOMINATES_IN_ALL);
        _may_dominates_noop_in.resize(num_labels, DOMINATES_IN_ALL);

        if (use_may_dominate) { // If we have more than 5000 labels, there is not enough space.
            _may_dominate_in.resize(num_labels);
            for (auto &l1: _may_dominate_in) {
                l1.resize(num_labels, DOMINATES_IN_ALL);
            }
        }
        std::cout << "Update label dominance: " << num_labels << " labels " << tss.size() << " systems.\n";

        for (int i = 0; i < num_tss; ++i) {
            update(i, *(tss[i]), *(local_dominance_functions[i]));
        }

    }


    template<typename TCost>
    bool
    LabelDominanceFunction<TCost>::update(int ts_id, const task_representation::TransitionSystem &ts, const LocalDominanceFunction<TCost> &sim) {
        bool changes = false;
        for (task_representation::LabelGroupID lg2_id(0); lg2_id < ts.num_label_groups(); ++lg2_id) {
            if (ts.get_label_group(lg2_id).empty())
                continue;

            for (task_representation::LabelGroupID lg1_id(0); lg1_id < ts.num_label_groups(); ++lg1_id) {
                if (lg1_id != lg2_id && !ts.get_label_group(lg1_id).empty() && may_simulate(lg1_id, lg2_id, ts_id)) {
                    TCost min_value = std::numeric_limits<int>::max();
                    //std::cout << "Check " << l1 << " " << l2 << std::endl;
                    //std::cout << "Num transitions: " << ts->get_transitions_label(l1).size()
                    //		    << " " << ts->get_transitions_label(l2).size() << std::endl;
                    //Check if it really simulates
                    //For each transition s--l2-->t, and every label l1 that dominates
                    //l2, exist s--l1-->t', t <= t'?

                    assert(!ts.get_transitions_for_group_id(lg2_id).empty());
                    for (const auto &tr_i: ts.get_transitions_for_group_id(lg2_id)) {
                        TCost max_value = std::numeric_limits<int>::lowest();
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
            TCost old_value = get_simulated_by_irrelevant(lg2_id, ts_id);
            if (old_value != TCost(std::numeric_limits<int>::lowest())) {
                TCost min_value = std::numeric_limits<int>::max();
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
                TCost min_value = std::numeric_limits<int>::max();
                for (int s = 0; s < ts.get_size(); s++) {
                    TCost max_value = std::numeric_limits<int>::lowest();
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


    template<typename TCost>
    void LabelDominanceFunction<TCost>::dump(const task_representation::TransitionSystem &ts, int ts_id) const {
        int count = 0;
        for (task_representation::LabelID l2(0); l2 < int(labels.get_size()); ++l2) {
            if (!ts.is_relevant_label(l2)) continue;

            for (task_representation::LabelID l1(0); l1 < int(labels.get_size()); ++l1) {
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


    template class LabelDominanceFunction<int>;

    template class LabelDominanceFunction<IntEpsilon>;

}