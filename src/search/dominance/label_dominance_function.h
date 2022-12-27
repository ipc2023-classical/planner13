#ifndef NUMERIC_DOMINANCE_NUMERIC_LABEL_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_LABEL_RELATION_H

#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"

#include <iostream>
#include <vector>
#include <limits>
#include <cassert>

namespace task_representation {
    class TransitionSystem;
}

namespace dominance {

    const int DOMINATES_IN_ALL = -2;
    const int DOMINATES_IN_NONE = -1;

    template<typename TCost>
    class DominanceFunction;

    template<typename TCost>
    class LocalDominanceFunction;

/*
 * Label relation represents the preorder relations on labels that
 * occur in a set of LTS
 */
    template<typename TCost>
    class LabelDominanceFunction {
        const task_representation::Labels &labels;
        int num_labels;
        int num_tss;

        // Summary matrix for each l1, l2 indicating whether l1 dominates
        // l2 in all (-2), in none (-1) or only in i (i)
        std::vector<std::vector<int> > _may_dominate_in;
        std::vector<int> _may_dominates_noop_in, _may_dominated_by_noop_in;


        std::vector<TCost> cost_of_label;

        // Maps ts id and label id to the label group id
        std::vector<std::vector<task_representation::LabelGroupID> > ts_label_id_to_label_group_id;
        std::vector<std::vector<task_representation::LabelGroupID>> irrelevant_label_groups_ts;
        std::vector<std::vector<std::vector<TCost> > > lqrel;
        std::vector<std::vector<TCost> > simulated_by_irrelevant;
        std::vector<std::vector<TCost> > simulates_irrelevant;

        bool update(int ts_id, const task_representation::TransitionSystem &ts, const LocalDominanceFunction<TCost> &sim);

        inline TCost get_lqrel(task_representation::LabelGroupID lg1_id, task_representation::LabelGroupID lg2_id, int lts) const {
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

        inline TCost get_lqrel(task_representation::LabelID l1_id, task_representation::LabelID l2_id, int ts) const {
            return get_lqrel(ts_label_id_to_label_group_id[ts][l1_id], ts_label_id_to_label_group_id[ts][l2_id], ts);
        }

        inline bool
        set_lqrel(task_representation::LabelGroupID lg1_id, task_representation::LabelGroupID lg2_id, int ts_id, const task_representation::TransitionSystem &ts, TCost value) {
            assert(value != std::numeric_limits<int>::lowest() + 1);
            /* assert(_may_dominate_in.empty() ||
               _may_dominate_in[l1][l2] == DOMINATES_IN_ALL || _may_dominate_in[l1][l2] != ts_id); */
            /* int pos1 = position_of_label[ts_id][l1]; */
            /* int pos2 = position_of_label[ts_id][l2]; */

            assert(lg1_id >= 0 && lg2_id >= 0);
            assert(ts_id >= 0 && ts_id < int(lqrel.size()));
            assert(lg1_id < int(lqrel[ts_id].size()));
            assert(lg2_id < int(lqrel[ts_id][lg1_id].size()));

            assert(value <= lqrel[ts_id][lg1_id][lg2_id]);
            if (value < lqrel[ts_id][lg1_id][lg2_id]) {
                lqrel[ts_id][lg1_id][lg2_id] = value;

                if (value == std::numeric_limits<int>::lowest() && !_may_dominate_in.empty()) {
                    for (int l1: ts.get_label_group(lg1_id)) {
                        for (int l2: ts.get_label_group(lg2_id)) {
                            if (_may_dominate_in[l1][l2] == DOMINATES_IN_ALL) {
                                _may_dominate_in[l1][l2] = ts_id;
                            } else if (_may_dominate_in[l1][l2] != ts_id) {
                                _may_dominate_in[l1][l2] = DOMINATES_IN_NONE;
                            }
                        }
                    }
                }
                return true;
            }
            return false;
        }

        inline TCost get_simulated_by_irrelevant(task_representation::LabelID l, int ts) const {
            // TODO: Check that label group is not dead
            return simulated_by_irrelevant[ts][ts_label_id_to_label_group_id[ts][l]];
        }

        inline TCost get_simulated_by_irrelevant(task_representation::LabelGroupID lgroup, int lts) const {
            // TODO: Check that label group is not dead
            return simulated_by_irrelevant[lts][lgroup];
        }

        inline bool set_simulated_by_irrelevant(task_representation::LabelGroupID lg_id, int ts_id, const task_representation::TransitionSystem &ts, TCost value) {
            //Returns if there were changes in _may_dominated_by_noop_in
            //int pos = position_of_label[ts_id][l];
            assert(lg_id >= 0);

            assert(value <= simulated_by_irrelevant[ts_id][lg_id]);
            assert(value != std::numeric_limits<int>::lowest() + 1);
            if (value < simulated_by_irrelevant[ts_id][lg_id]) {
                simulated_by_irrelevant[ts_id][lg_id] = value;

                if (value == std::numeric_limits<int>::lowest()) {

                    for (int l_id: ts.get_label_group(lg_id)) {
                        if (_may_dominated_by_noop_in[l_id] == DOMINATES_IN_ALL) {
                            _may_dominated_by_noop_in[l_id] = ts_id;
                        } else if (_may_dominated_by_noop_in[l_id] != ts_id) {
                            _may_dominated_by_noop_in[l_id] = DOMINATES_IN_NONE;
                        }
                        if (!_may_dominate_in.empty()) {
                            for (task_representation::LabelGroupID lg2_id: irrelevant_label_groups_ts[ts_id]) {
                                for (int l2_id: ts.get_label_group(lg2_id)) {
                                    if (_may_dominate_in[l2_id][l_id] == DOMINATES_IN_ALL) {
                                        _may_dominate_in[l2_id][l_id] = ts_id;
                                    } else if (_may_dominate_in[l2_id][l_id] != ts_id) {
                                        _may_dominate_in[l2_id][l_id] = DOMINATES_IN_NONE;
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

        inline bool set_simulates_irrelevant(task_representation::LabelGroupID lg_id, int ts_id, const task_representation::TransitionSystem &ts, TCost value) {
            //std::cout << "simulates irrelevant: " << g_operators[l].get_name() << " in " << g_fact_names[ts_id][0] << ": " << value << std::endl;
            assert(value != std::numeric_limits<int>::lowest() + 1);
            assert(lg_id >= 0);

            //Returns if there were changes in _may_dominates_noop_in
            assert(value <= simulates_irrelevant[ts_id][lg_id]);
            if (value < simulates_irrelevant[ts_id][lg_id]) {
                simulates_irrelevant[ts_id][lg_id] = value;

                if (value == std::numeric_limits<int>::lowest()) {
                    for (int l: ts.get_label_group(lg_id)) {
                        if (_may_dominates_noop_in[l] == DOMINATES_IN_ALL) {
                            _may_dominates_noop_in[l] = ts_id;
                        } else if (_may_dominates_noop_in[l] != ts_id) {
                            _may_dominates_noop_in[l] = DOMINATES_IN_NONE;
                        }
                        if (!_may_dominate_in.empty()) {
                            for (int lg2: irrelevant_label_groups_ts[ts_id]) {
                                for (int l2: ts.get_label_group(task_representation::LabelGroupID(lg2))) {
                                    if (_may_dominate_in[l][l2] == DOMINATES_IN_ALL) {
                                        _may_dominate_in[l][l2] = ts_id;
                                    } else if (_may_dominate_in[l][l2] != ts_id) {
                                        _may_dominate_in[l][l2] = DOMINATES_IN_NONE;
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

    public:
        explicit LabelDominanceFunction(const task_representation::Labels &labels);

        //Initializes label relation (only the first time, to reinitialize call reset instead
        // If use_may_dominate is true, it will use the summary tables. This speeds up a little bit dominance checks but
        // requires memory quadratic in the number of labels so it is not recommended if there are more than 1000 labels.
        void init(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                  const std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> & local_dominance_functions, bool use_may_dominate);

        bool update(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                    const std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> & local_dominance_functions) {
            bool changes = false;

            for (int i = 0; i < int(tss.size()); ++i) {
                changes |= update(i, *(tss[i]), *(local_dominance_functions[i]));
            }

            /* if(tau_labels) { */
            /*     for (int lts_id = 0; lts_id < lts.size(); ++lts_id) { */
            /* 	tau_labels->get_tau_labels().erase(lts_id, */
            /* 	 [&](int label) { */
            /* 	     return _may_dominates_noop_in[label] != DOMINATES_IN_ALL && */
            /* 		 _may_dominates_noop_in[label] != lts_id; */
            /* 	 }); */
            /*     } */
            /* }				  */

            return changes;
        }

        inline int get_num_labels() const {
            return num_labels;
        }

        inline bool may_dominated_by_noop(task_representation::LabelID l, int ts_id) const {
            return _may_dominated_by_noop_in[l] == DOMINATES_IN_ALL || _may_dominated_by_noop_in[l] == ts_id;
        }

        inline bool may_dominate_noop_in(task_representation::LabelID l, int ts_id) const {
            return _may_dominates_noop_in[l] == DOMINATES_IN_ALL || _may_dominates_noop_in[l] == ts_id;
        }

        inline bool dominates_noop_in_all(task_representation::LabelID l) const {
            return _may_dominates_noop_in[l] == DOMINATES_IN_ALL;
        }

        inline int get_may_dominates_noop_in(task_representation::LabelID l) const {
            return _may_dominates_noop_in[l];
        }

        inline int get_may_dominated_by_noop_in(task_representation::LabelID l) const {
            return _may_dominated_by_noop_in[l];
        }

        inline int dominates_noop_in_some(task_representation::LabelID l) const {
            return _may_dominates_noop_in[l] >= 0;
        }


        //Returns true if l dominates l2 in ts_id (simulates l2 in all j \neq ts_id)
        inline bool may_dominate(task_representation::LabelID l1, task_representation::LabelID l2, int ts_id) const {
            if (_may_dominate_in.empty()) {
                for (int ts2_id = 0; ts2_id < num_tss; ++ts2_id) {
                    if (ts2_id != ts_id && get_lqrel(l1, l2, ts2_id) == std::numeric_limits<int>::lowest()) {
                        assert(num_tss > 1);
                        return false;
                    }
                }
                return true;
            }

            assert(num_tss > 1 || _may_dominate_in[l1][l2] == DOMINATES_IN_ALL || (_may_dominate_in[l1][l2] == ts_id));

#ifndef NDEBUG
            if (_may_dominate_in[l1][l2] == DOMINATES_IN_ALL || (_may_dominate_in[l1][l2] == ts_id)) {
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
            return _may_dominate_in[l1][l2] == DOMINATES_IN_ALL || (_may_dominate_in[l1][l2] == ts_id);
        }

        //Returns true if l1 simulates l2 in ts_id
        inline bool may_simulate(task_representation::LabelGroupID lg1_id, task_representation::LabelGroupID lg2_id, int ts_id) const {
            return get_lqrel(lg1_id, lg2_id, ts_id) != std::numeric_limits<int>::lowest();
        }

        /* //Returns true if l1 simulates l2 in lts */
        /* inline bool may_simulate (int l1, int l2, int lts) const{ */
        /* 	if(_may_dominate_in.empty()) { */
        /* 	    return get_lqrel(l1, l2, lts) != std::numeric_limits<int>::lowest(); */
        /* 	} */
        /*     return _may_dominate_in[l1][l2] !=  DOMINATES_IN_NONE && */
        /* 	    (_may_dominate_in[l1][l2] == DOMINATES_IN_ALL || */
        /* 	     _may_dominate_in[l1][l2] != lts); */
        /* } */


        //Returns true if l1 dominates l2 in ts_id
        TCost q_dominates(task_representation::LabelID l1, task_representation::LabelID l2, int ts_id) const {
            if (may_dominate(l1, l2, ts_id)) {
                TCost total_sum = 0;

                for (int ts_id2 = 0; ts_id2 < num_tss; ++ts_id2) {
                    if (ts_id2 != ts_id) {
                        assert (get_lqrel(l1, l2, ts_id2) != std::numeric_limits<int>::lowest());
                        total_sum += get_lqrel(l1, l2, ts_id2);
                    }
                }

                assert(num_tss > 0 || total_sum == TCost(0));

                return total_sum;
            } else {
                //assert(false);
                return std::numeric_limits<int>::lowest();
            }
        }

        TCost q_dominates_noop(task_representation::LabelID l, int exclude_ts_id = -2) const {
            if (may_dominate_noop_in(l, exclude_ts_id)) {
                TCost total_sum = 0;

                for (int ts2_id = 0; ts2_id < num_tss; ++ts2_id) {
                    if (ts2_id != exclude_ts_id) {
                        assert (get_simulates_irrelevant(ts_label_id_to_label_group_id[ts2_id][l], ts2_id) !=
                                std::numeric_limits<int>::lowest());
                        total_sum += get_simulates_irrelevant(ts_label_id_to_label_group_id[ts2_id][l], ts2_id);
                    }
                }
                return total_sum;
            } else {
                //assert(false);
                return std::numeric_limits<int>::lowest();
            }
        }

        // -2 indicates none are excluded and it must therefore may_dominate in all
        TCost q_dominated_by_noop(task_representation::LabelID l, int exclude_ts = -2) const {
            if (may_dominated_by_noop(l, exclude_ts)) {
                TCost total_sum = 0;

                for (int ts_id = 0; ts_id < num_tss; ++ts_id) {
                    if (ts_id != exclude_ts) {
                        assert (get_simulated_by_irrelevant(ts_label_id_to_label_group_id[ts_id][l], ts_id) !=
                                std::numeric_limits<int>::lowest());
                        total_sum += get_simulated_by_irrelevant(ts_label_id_to_label_group_id[ts_id][l], ts_id);
                    }
                }
                return total_sum;

            } else {
                //assert(false);
                return std::numeric_limits<int>::lowest();
            }
        }

        inline TCost get_simulates_irrelevant(task_representation::LabelGroupID lgroup, int ts_id) const {
            // TODO: Check that label group is not dead
            return simulates_irrelevant[ts_id][lgroup];
        }

        inline TCost get_label_simulates_irrelevant(task_representation::LabelID l_id, int ts_id) const {
            return simulates_irrelevant[ts_id][ts_label_id_to_label_group_id[ts_id][l_id]];
        }

        TCost get_label_cost(task_representation::LabelID label) const {
            return cost_of_label[label];
        }

        void dump(const task_representation::TransitionSystem &ts, int ts_id) const;

    };
}
#endif
