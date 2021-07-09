#ifndef NUMERIC_DOMINANCE_NUMERIC_LABEL_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_LABEL_RELATION_H

#include "../task_representation/label_equivalence_relation.h"

#include <iostream>
#include <vector>
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "int_epsilon.h"


const int DOMINATES_IN_ALL = -2;
const int DOMINATES_IN_NONE = -1;

template<typename T>
class NumericDominanceRelation;

template<typename T>
class NumericSimulationRelation;

using namespace task_representation;

/*
 * Label relation represents the preorder relations on labels that
 * occur in a set of LTS
 */
template<typename T>
class NumericLabelRelation {
    const Labels &labels;
    int num_labels;
    int num_tss;

    const int num_labels_to_use_dominates_in;

    // Summary matrix for each l1, l2 indicating whether l1 dominates
    // l2 in all (-2), in none (-1) or only in i (i)
    std::vector<std::vector<int> > dominates_in;
    std::vector<int> dominates_noop_in, dominated_by_noop_in;

    //For each lts, matrix indicating whether l1 simulates l2, noop
    //simulates l or l simulates noop
    std::vector<T> cost_of_label;

    // Maps ts id and label id to the label group id
    std::vector<std::vector<LabelGroupID> > ts_label_id_to_label_group_id;
    std::vector<std::vector<LabelGroupID>> irrelevant_label_groups_ts;
    std::vector<std::vector<std::vector<T> > > lqrel;
    std::vector<std::vector<T> > simulated_by_irrelevant;
    std::vector<std::vector<T> > simulates_irrelevant;

    /* std::shared_ptr<TauLabelManager<T>> tau_labels; */

    bool update(int i, const TransitionSystem &ts, const NumericSimulationRelation<T> &sim);

    inline T get_lqrel(LabelGroupID lg1_id, LabelGroupID lg2_id, int lts) const {
        if (lg1_id >= 0) {
            if (lg2_id >= 0) {
                return lqrel[lts][lg1_id][lg2_id];
            } else {
                return simulates_irrelevant[lts][lg1_id];
            }
        } else {
            if (lg2_id != -1) {
                return simulated_by_irrelevant[lts][lg2_id];
            } else {
                return 0; //Both are irrelevant
            }
        }
    }

    inline T get_lqrel(LabelID l1_id, LabelID l2_id, int ts) const {
        return get_lqrel(ts_label_id_to_label_group_id[ts][l1_id], ts_label_id_to_label_group_id[ts][l2_id], ts);
    }

    inline bool set_lqrel(LabelGroupID lg1_id, LabelGroupID lg2_id, int ts_id, const TransitionSystem &ts, T value) {
        assert(value != std::numeric_limits<int>::lowest() + 1);
        /* assert(dominates_in.empty() ||
           dominates_in[l1][l2] == DOMINATES_IN_ALL || dominates_in[l1][l2] != ts_id); */
        /* int pos1 = position_of_label[ts_id][l1]; */
        /* int pos2 = position_of_label[ts_id][l2]; */

        assert(lg1_id >= 0 && lg2_id >= 0);
        assert(ts_id >= 0 && ts_id < int(lqrel.size()));
        assert(lg1_id < int(lqrel[ts_id].size()));
        assert(lg2_id < int(lqrel[ts_id][lg1_id].size()));

        assert(value <= lqrel[ts_id][lg1_id][lg2_id]);
        if (value < lqrel[ts_id][lg1_id][lg2_id]) {
            lqrel[ts_id][lg1_id][lg2_id] = value;

            if (value == std::numeric_limits<int>::lowest() && !dominates_in.empty()) {
                for (int l1 : ts.get_label_group(lg1_id)) {
                    for (int l2 : ts.get_label_group(lg2_id)) {
                        if (dominates_in[l1][l2] == DOMINATES_IN_ALL) {
                            dominates_in[l1][l2] = ts_id;
                        } else if (dominates_in[l1][l2] != ts_id) {
                            dominates_in[l1][l2] = DOMINATES_IN_NONE;
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    inline T get_simulated_by_irrelevant(LabelID l, int ts) const {
        // TODO: Check that label group is not dead
        return simulated_by_irrelevant[ts][ts_label_id_to_label_group_id[ts][l]];
    }

    inline T get_simulated_by_irrelevant(LabelGroupID lgroup, int lts) const {
        // TODO: Check that label group is not dead
        return simulated_by_irrelevant[lts][lgroup];
    }

    inline bool set_simulated_by_irrelevant(LabelGroupID lg_id, int ts_id, const TransitionSystem &ts, T value) {
        //Returns if there were changes in dominated_by_noop_in
        //int pos = position_of_label[ts_id][l];
        assert(lg_id >= 0);

        assert(value <= simulated_by_irrelevant[ts_id][lg_id]);
        assert(value != std::numeric_limits<int>::lowest() + 1);
        if (value < simulated_by_irrelevant[ts_id][lg_id]) {
            simulated_by_irrelevant[ts_id][lg_id] = value;

            if (value == std::numeric_limits<int>::lowest()) {

                for (int l_id : ts.get_label_group(lg_id)) {
                    if (dominated_by_noop_in[l_id] == DOMINATES_IN_ALL) {
                        dominated_by_noop_in[l_id] = ts_id;
                    } else if (dominated_by_noop_in[l_id] != ts_id) {
                        dominated_by_noop_in[l_id] = DOMINATES_IN_NONE;
                    }
                    if (!dominates_in.empty()) {
                        for (LabelGroupID lg2_id : irrelevant_label_groups_ts[ts_id]) {
                            for (int l2_id : ts.get_label_group(lg2_id)) {
                                if (dominates_in[l2_id][l_id] == DOMINATES_IN_ALL) {
                                    dominates_in[l2_id][l_id] = ts_id;
                                } else if (dominates_in[l2_id][l_id] != ts_id) {
                                    dominates_in[l2_id][l_id] = DOMINATES_IN_NONE;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }


    inline T get_simulates_irrelevant(LabelGroupID lgroup, int ts_id) const {
        // TODO: Check that label group is not dead
        return simulates_irrelevant[ts_id][lgroup];
    }


    inline bool set_simulates_irrelevant(LabelGroupID lg_id, int ts_id, const TransitionSystem &ts, T value) {
        //std::cout << "simulates irrelevant: " << g_operators[l].get_name() << " in " << g_fact_names[ts_id][0] << ": " << value << std::endl;
        assert(value != std::numeric_limits<int>::lowest() + 1);
        assert(lg_id >= 0);

        //Returns if there were changes in dominates_noop_in
        assert(value <= simulates_irrelevant[ts_id][lg_id]);
        if (value < simulates_irrelevant[ts_id][lg_id]) {
            simulates_irrelevant[ts_id][lg_id] = value;

            if (value == std::numeric_limits<int>::lowest()) {
                for (int l : ts.get_label_group(lg_id)) {
                    if (dominates_noop_in[l] == DOMINATES_IN_ALL) {
                        dominates_noop_in[l] = ts_id;
                    } else if (dominates_noop_in[l] != ts_id) {
                        dominates_noop_in[l] = DOMINATES_IN_NONE;
                    }
                    if (!dominates_in.empty()) {
                        for (int lg2 : irrelevant_label_groups_ts[ts_id]) {
                            for (int l2 : ts.get_label_group(LabelGroupID(lg2))) {
                                if (dominates_in[l][l2] == DOMINATES_IN_ALL) {
                                    dominates_in[l][l2] = ts_id;
                                } else if (dominates_in[l][l2] != ts_id) {
                                    dominates_in[l][l2] = DOMINATES_IN_NONE;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    /* int mix_numbers(const std::vector<int> & values,  */
    /* 		    const std::vector<int> & values_irrelevant_labels,  */
    /* 		    int lts) const;  */

public:
    NumericLabelRelation(const Labels &labels, int num_labels_to_use_dominates_in);

    //Initializes label relation (only the first time, to reinitialize call reset instead)
    template<typename NDR>
    void init(const std::vector<TransitionSystem> &tss, const NDR &sim) {
        num_labels = labels.get_size();
        num_tss = int(tss.size());

        std::cout << "Init label dominance: " << num_labels << " labels " << tss.size() << " systems.\n";


        std::vector<T>().swap(cost_of_label);
        std::vector<std::vector<LabelGroupID> >().swap(ts_label_id_to_label_group_id);
        std::vector<std::vector<std::vector<T> > >().swap(lqrel);
        std::vector<std::vector<T> >().swap(simulates_irrelevant);
        std::vector<std::vector<T> >().swap(simulated_by_irrelevant);

        irrelevant_label_groups_ts.resize(tss.size());
        ts_label_id_to_label_group_id.resize(tss.size());
        simulates_irrelevant.resize(tss.size());
        simulated_by_irrelevant.resize(tss.size());
        lqrel.resize(tss.size());

        cost_of_label.resize(num_labels);
        for (LabelID l(0); l < num_labels; ++l) {
            cost_of_label[l] = labels.get_label_cost(l);
        }

        for (int i = 0; i < num_tss; ++i) {

            int num_label_groups = tss[i].num_label_groups();

            // Compute the map from label_id to label group id for this transition system
            ts_label_id_to_label_group_id[i].resize(labels.get_size());
            for (LabelID l_id(0); l_id < labels.get_size(); ++l_id) {
                ts_label_id_to_label_group_id[i][l_id] = tss[i].get_label_group_id_of_label(l_id);
            }

            // Compute irrelevant label groups for this ts
            for (LabelGroupID lg_id(0); lg_id < num_label_groups; ++lg_id) {
                if (!tss[i].is_relevant_label_group(lg_id)) {
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
        std::vector<std::vector<int> >().swap(dominates_in);
        std::vector<int>().swap(dominated_by_noop_in);
        std::vector<int>().swap(dominates_noop_in);
        dominated_by_noop_in.resize(num_labels, DOMINATES_IN_ALL);
        dominates_noop_in.resize(num_labels, DOMINATES_IN_ALL);

        if (num_labels <
            num_labels_to_use_dominates_in) { // If we have more than 5000 labels, there is not enough space.
            dominates_in.resize(num_labels);
            for (auto &l1 : dominates_in) {
                l1.resize(num_labels, DOMINATES_IN_ALL);
            }
        }
        std::cout << "Update label dominance: " << num_labels
                  << " labels " << tss.size() << " systems.\n";

        for (int i = 0; i < num_tss; ++i) {
            update(i, tss[i], sim[i]);
        }

    }

    template<typename NDR>
    bool update(const std::vector<TransitionSystem> &tss, const NDR &sim) {

        bool changes = false;

        for (int i = 0; i < int(tss.size()); ++i) {
            changes |= update(i, tss[i], sim[i]);
        }

        /* if(tau_labels) { */
        /*     for (int lts_id = 0; lts_id < lts.size(); ++lts_id) { */
        /* 	tau_labels->get_tau_labels().erase(lts_id, */
        /* 	 [&](int label) { */
        /* 	     return dominates_noop_in[label] != DOMINATES_IN_ALL && */
        /* 		 dominates_noop_in[label] != lts_id; */
        /* 	 }); */
        /*     } */
        /* }				  */

        return changes;
    }

    inline int get_num_labels() const {
        return num_labels;
    }

    inline bool may_dominated_by_noop(LabelID l, int ts_id) const {
        return dominated_by_noop_in[l] == DOMINATES_IN_ALL || dominated_by_noop_in[l] == ts_id;
    }

    inline bool may_dominate_noop_in(LabelID l, int ts_id) const {
        return dominates_noop_in[l] == DOMINATES_IN_ALL || dominates_noop_in[l] == ts_id;
    }

    inline bool dominates_noop_in_all(LabelID l) const {
        return dominates_noop_in[l] == DOMINATES_IN_ALL;
    }

    inline int get_dominates_noop_in(LabelID l) const {
        return dominates_noop_in[l];
    }

    inline int dominates_noop_in_some(LabelID l) const {
        return dominates_noop_in[l] >= 0;
    }


    //Returns true if l dominates l2 in ts_id (simulates l2 in all j \neq ts_id)
    inline bool may_dominate(LabelID l1, LabelID l2, int ts_id) const {
        if (dominates_in.empty()) {
            for (int ts2_id = 0; ts2_id < num_tss; ++ts2_id) {
                if (ts2_id != ts_id && get_lqrel(l1, l2, ts2_id) == std::numeric_limits<int>::lowest()) {
                    assert(num_tss > 1);
                    return false;
                }
            }
            return true;
        }

        assert(num_tss > 1 || dominates_in[l1][l2] == DOMINATES_IN_ALL || (dominates_in[l1][l2] == ts_id));

#ifndef NDEBUG
        if (dominates_in[l1][l2] == DOMINATES_IN_ALL || (dominates_in[l1][l2] == ts_id)) {
            for (int ts2_id = 0; ts2_id < num_tss; ++ts2_id) {
                if (!(ts_id == ts2_id || get_lqrel(l1, l2, ts2_id) != std::numeric_limits<int>::lowest())) {
                    std::cout << this << "l1: " << l1 << " l2: " << l2 << " ts2_id: " << ts2_id << " group1: "
                              << ts_label_id_to_label_group_id[ts2_id][l1] << " group2: "
                              << ts_label_id_to_label_group_id[ts2_id][l2]
                              << std::endl;
                }
                assert(ts2_id == ts_id || get_lqrel(l1, l2, ts2_id) != std::numeric_limits<int>::lowest());
            }
        }
#endif
        return dominates_in[l1][l2] == DOMINATES_IN_ALL || (dominates_in[l1][l2] == ts_id);
    }

    //Returns true if l1 simulates l2 in ts_id
    inline bool may_simulate(LabelGroupID lg1_id, LabelGroupID lg2_id, int ts_id) const {
        return get_lqrel(lg1_id, lg2_id, ts_id) != std::numeric_limits<int>::lowest();
    }

    /* //Returns true if l1 simulates l2 in lts */
    /* inline bool may_simulate (int l1, int l2, int lts) const{ */
    /* 	if(dominates_in.empty()) { */
    /* 	    return get_lqrel(l1, l2, lts) != std::numeric_limits<int>::lowest(); */
    /* 	} */
    /*     return dominates_in[l1][l2] !=  DOMINATES_IN_NONE && */
    /* 	    (dominates_in[l1][l2] == DOMINATES_IN_ALL || */
    /* 	     dominates_in[l1][l2] != lts); */
    /* } */


    //Returns true if l1 dominates l2 in ts_id
    T q_dominates(LabelID l1, LabelID l2, int ts_id) const {
        if (may_dominate(l1, l2, ts_id)) {
            T total_sum = 0;

            for (int lts_id = 0; lts_id < num_tss; ++lts_id) {
                if (lts_id != ts_id) {
                    assert (get_lqrel(l1, l2, lts_id) != std::numeric_limits<int>::lowest());
                    total_sum += get_lqrel(l1, l2, lts_id);
                }
            }

            assert(num_tss > 0 || total_sum == T(0));

            return total_sum;
        } else {
            assert(false);
            return std::numeric_limits<int>::lowest();
        }
    }

    T q_dominates_noop(LabelID l, int ts_id) const {
        if (may_dominate_noop_in(l, ts_id)) {
            T total_sum = 0;

            for (int ts2_id = 0; ts2_id < num_tss; ++ts2_id) {
                if (ts2_id != ts_id) {
                    assert (get_simulates_irrelevant(ts_label_id_to_label_group_id[ts2_id][l], ts2_id) !=
                            std::numeric_limits<int>::lowest());
                    total_sum += get_simulates_irrelevant(ts_label_id_to_label_group_id[ts2_id][l], ts2_id);
                }
            }
            return total_sum;
        } else {
            assert(false);
            return std::numeric_limits<int>::lowest();
        }
    }


    T q_dominated_by_noop(LabelID l, int ts) const {
        if (may_dominated_by_noop(l, ts)) {
            T total_sum = 0;

            for (int ts_id = 0; ts_id < num_tss; ++ts_id) {
                if (ts_id != ts) {
                    assert (get_simulated_by_irrelevant(ts_label_id_to_label_group_id[ts_id][l], ts_id) !=
                            std::numeric_limits<int>::lowest());
                    total_sum += get_simulated_by_irrelevant(ts_label_id_to_label_group_id[ts_id][l], ts_id);
                }
            }
            return total_sum;

        } else {
            assert(false);
            return std::numeric_limits<int>::lowest();
        }
    }

    /* const std::vector<int> & get_tau_labels_for_states(int lts) const { */
    /* 	return (tau_labels ? */
    /* 		tau_labels_noop_dominance[lts] : tau_labels_self_loops[lts]); */
    /* } */

    T get_label_cost(LabelID label) const {
        return cost_of_label[label];
    }

    void dump(const TransitionSystem &ts, int ts_id) const;

    /* std::shared_ptr<TauLabelManager<T>> get_tau_labels() const { */
    /* 	return tau_labels; */
    /* } */
};

#endif
