#ifndef NUMERIC_DOMINANCE_NUMERIC_SIMULATION_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_SIMULATION_RELATION_H

#include <vector>
#include <string>
#include <iostream>
#include "label_dominance_function.h"
#include "int_epsilon.h"
#include "tau_labels.h"
#include "../task_representation/fts_task.h"
#include "../task_representation/state.h"

namespace dominance {
    bool
    applyPostSrc(const TransitionSystem &ts, int src, std::function<bool(const Transition &t, LabelGroupID lg_id)> &&f);

    template<typename T>
    class LocalDominanceFunction {
        // This is a local dominance function computed for a transition system using the following parameters
        const TransitionSystem &ts;
        const int ts_id;
        const int truncate_value;

        std::shared_ptr<TauLabelInfo<T>> tau_labels;
        int tau_distances_id{};

        // Precomputed dominance function
        std::vector<std::vector<T> > relation;
        T max_relation_value;

        bool cancelled;

        T compare_noop(int tr_s_target, LabelID tr_s_label, int t,
                       T tau_distance,
                       const LabelDominanceFunction<T> &label_dominance) const;


        T compare_transitions(int tr_s_target, LabelID tr_s_label,
                              int tr_t_target, LabelID tr_t_label,
                              T tau_distance,
                              const LabelDominanceFunction<T> &label_dominance) const;

        int update_pair(const LabelDominanceFunction<T> &label_dominance,
                        const TauDistances<T> &tau_distances,
                        int s, int t);

        int update_pair_stable(const LabelDominanceFunction<T> &label_dominance,
                               const TauDistances<T> &tau_distances,
                               int s, int t);

    public:
        LocalDominanceFunction(const TransitionSystem &ts, int ts_id, int truncate_value,
                               std::shared_ptr<TauLabelInfo<T>> tau_labels_mgr);

        void init_goal_respecting();

        int update(const LabelDominanceFunction<T> &label_dominance, int max_time);


        void cancel_simulation_computation();

        inline bool simulates(int s, int t) const {
            return relation[s][t] >= 0;
        }

        inline bool may_simulate(int s, int t) const {
            assert(s < int(relation.size()));
            assert(t < int(relation[s].size()));
            return relation[s][t] > std::numeric_limits<int>::lowest();
        }

        inline T q_simulates(int s, int t) const {
            /* if(s >= relation.size()) { */
            /*     std::cout << s << std::endl; */
            /*     std::cout << relation.size() << std::endl; */
            /* } */

            assert(s < int(relation.size()));
            assert(t < int(relation[s].size()));
            assert(s != t || relation[s][t] == 0);
            return relation[s][t];
        }

        inline bool strictly_simulates(int s, int t) const {
            /* if(s >= relation.size()) { */
            /*     std::cout << s << std::endl; */
            /*     std::cout << relation.size() << std::endl; */
            /* } */

            return relation[s][t] >= 0 && relation[t][s] < 0;
        }

        inline bool positively_simulates(int s, int t) const {
            /* if(s >= relation.size()) { */
            /*     std::cout << s << std::endl; */
            /*     std::cout << relation.size() << std::endl; */
            /* } */

            assert(size_t(s) < relation.size());
            assert(size_t(t) < relation[s].size());
            assert(s != t || relation[s][t] == 0);
            return relation[s][t] >= 0;
        }

        inline bool similar(int s, int t) const {
            return simulates(s, t) && simulates(t, s);
        }

        inline void update_value(int s, int t, T value) {
            /* if(value < -truncate_value) { */
            /*     // std::cout << value << " rounded to -infty: " << truncate_value << std::endl; */
            /*     value = std::numeric_limits<int>::lowest(); */
            /* } */
            assert(value != std::numeric_limits<int>::lowest() + 1);
            relation[s][t] = value;
        }

        inline const std::vector<std::vector<T> > &get_relation() {
            return relation;
        }

        T compute_max_value() {
            max_relation_value = 0;
            for (const auto &row: relation) {
                for (T value: row) {
                    max_relation_value = std::max(max_relation_value, value);
                }
            }
            return max_relation_value;
        }

        T get_max_value() const {
            return max_relation_value;
        }

        std::vector<int> get_dangerous_labels() const;

        void dump(const std::vector<std::string> &names) const;

        void dump() const;

        void statistics() const;

        bool has_dominance() const;

        bool has_positive_dominance() const;

        int get_ts_id() const { return ts_id; }
    };

}

#endif
