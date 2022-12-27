#ifndef NUMERIC_DOMINANCE_NUMERIC_TAU_LABELS_H
#define NUMERIC_DOMINANCE_NUMERIC_TAU_LABELS_H

#include <vector>
#include <memory>

#include <cassert>
#include <set>
#include <limits>
#include "../utils/id.h"
#include "../task_representation/types.h"


namespace options {
class OptionParser;

class Options;
}

namespace task_representation { class FTSTask; class TransitionSystem; class Labels;}

namespace dominance {
    template <typename TCost> class LabelDominanceFunction;

    template<typename TCost>
    class TauDistances;


    template<typename TCost>
    class TauLabels {
        int num_tau_labels_for_some, num_tau_labels_for_all;
        std::vector<TCost> original_cost;

        //The cost of applying a tau label may be higher than the cost of the label since it
        //may require executing other tau label moves in different transition systems.
        std::vector<int> label_relevant_for;
        std::vector<std::vector<TCost> > tau_label_cost;

        std::vector<std::vector<int>> tau_labels;

    public:
        explicit TauLabels(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss, const task_representation::Labels &labels);

        static bool label_may_be_tau_in_other_ts(const task_representation::TransitionSystem &ts, task_representation::LabelID l_id);

        std::set<int>
        add_recursive_tau_labels(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                                 const std::vector<std::unique_ptr<TauDistances<TCost>>> &tau_distances);

        bool empty() const {
            return num_tau_labels_for_some == 0;
        }

        std::set<int> add_noop_dominance_tau_labels(const LabelDominanceFunction<TCost> &label_dominance);

        int size(int lts) const {
            return tau_labels[lts].size();
        }

        const std::vector<int> &get_tau_labels(int lts) const {
            return tau_labels[lts];
        }


        TCost get_cost(int lts, int label) const {
            if (tau_label_cost[lts].empty()) {
                return original_cost[label];
            }
            return tau_label_cost[lts][label] + original_cost[label];
        }

        void set_tau_cost(int lts, int label, TCost tau_cost) {
            if (tau_label_cost[lts].empty()) {
                tau_label_cost[lts].resize(original_cost.size(), 0);
            }
            assert (tau_cost >= 0);
            tau_label_cost[lts][label] = tau_cost;
        }

        template<typename F>
        void erase(int lts_id, F lambda) {
            erase_if(tau_labels[lts_id], lambda);
        }
    };

    template<typename TCost>
    class TauDistances {
        ID<TauDistances> id;
        int num_tau_labels;
        std::vector<std::vector<TCost> > distances_with_tau;
        std::vector<TCost> goal_distances_with_tau;
        std::vector<std::vector<int> > reachable_with_tau;
        //std::vector<int> max_cost_reach_from_with_tau, max_cost_reach_with_tau;
        TCost cost_fully_invertible;
        // List of states for which distances_with_tau is not infinity
    public:
        TauDistances() :
                id(0), num_tau_labels(0), cost_fully_invertible(std::numeric_limits<int>::max()) {
        }

        bool precompute(const TauLabels<TCost> &tau_labels, const task_representation::TransitionSystem &ts, int ts_id, bool only_reachability);

        bool empty() const {
            return num_tau_labels == 0;
        }

        auto get_id() const {
            return id;
        }

        inline TCost minus_shortest_path(int from, int to) const {
            assert(from < int(distances_with_tau.size()));
            assert(to < int(distances_with_tau[from].size()));
            assert((from != to && distances_with_tau[from][to] > 0) ||
                   (from == to && distances_with_tau[from][to] == 0));
            return distances_with_tau[from][to] == std::numeric_limits<int>::max()
                   ? std::numeric_limits<int>::lowest()
                   : -distances_with_tau[from][to];
        }

        inline TCost shortest_path(int from, int to) const {
            assert(from < distances_with_tau.size());
            assert(to < distances_with_tau[from].size());
            assert((from != to && distances_with_tau[from][to] > 0) ||
                   (from == to && distances_with_tau[from][to] == 0));

            return distances_with_tau[from][to];
        }

        TCost get_goal_distance(int s) const {
            return goal_distances_with_tau[s];
        }

        const std::vector<int> &states_reachable_from(int s) const {
            return reachable_with_tau[s];
        }

        bool is_fully_invertible() const {
            return cost_fully_invertible < std::numeric_limits<int>::max();
        }

        TCost get_cost_fully_invertible() const;
    };


    // Contains the information of TauLabels for a given FTS: which labels are tau in each TS and what are the TauDistances.
    // Warning: This is actually used during the computation of dominance, so the tau labels/distances may change dynamically,
    template <typename TCost>
    class TauLabelInfo {
        TauLabels<TCost> tau_labels;
        std::vector<std::unique_ptr<TauDistances<TCost>>> tau_distances;
        bool are_distances_required;

        public:
        TauLabelInfo (TauLabels<TCost> tau_labels, const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss, bool are_distances_required);

        bool add_recursive_tau_labels(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss);

        bool add_noop_dominance_tau_labels(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                                           const LabelDominanceFunction<TCost> &label_dominance);

        const TauDistances<TCost> &get_tau_distances(int lts_id) const {
            assert(tau_distances[lts_id]);
            return *(tau_distances[lts_id]);
        }

        const TauLabels<TCost> &get_tau_labels() const {
            return tau_labels;
        }
    };

    class TauLabelManager {
        const bool self_loops;
        const bool recursive;
        const bool noop_dominance;
    public:
        explicit TauLabelManager(const options::Options &opts);

        template <typename TCost>
        std::shared_ptr<TauLabelInfo<TCost>> compute_tau_labels (const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                                                const task_representation::Labels &labels, bool only_reachability) const;

        template <typename TCost>
        bool recompute_tau_labels(TauLabelInfo<TCost> & tau_labels, const std::vector<std::unique_ptr<task_representation::TransitionSystem>> &tss,
                                  const LabelDominanceFunction<TCost> &label_dominance) const;

        void print_config() const;

        static void add_options_to_parser(options::OptionParser &parser);

    };
}

#endif
