#ifndef MERGE_AND_SHRINK_SHRINK_WEAK_BISIMULATION_H
#define MERGE_AND_SHRINK_SHRINK_WEAK_BISIMULATION_H

#include "shrink_strategy.h"

#include<memory>
#include<vector>

namespace options{
    class Options;
}

namespace task_representation {
    class TransitionSystem;
}

namespace task_transformation {
    struct Signature;
    class ShrinkWeakBisimulation : public ShrinkStrategy {
        const bool preserve_optimality;
        const bool ignore_irrelevant_tau_groups;
        mutable bool apply_haslum_rule; // TODO: make const immutable once supported

        int initialize_groups(
            const std::vector<int> & goal_distances,
            std::vector<int> &state_to_group) const;

    void compute_signatures(
        const task_representation::TransitionSystem &ts,
        const std::vector<int> & mapping_to_scc,
        const std::vector<int> & goal_distances,
        const std::vector<bool> &tau_label_group,
        const std::vector<bool> & outside_relevant_group,
        std::vector<Signature> &signatures,
        const std::vector<int> &state_to_group,
        const std::vector<std::vector<int>> &can_reach_via_tau_path) const;

    public:
        ShrinkWeakBisimulation(const options::Options &opts);

        virtual ~ShrinkWeakBisimulation() override = default;

        virtual StateEquivalenceRelation compute_equivalence_relation(
            const FactoredTransitionSystem &fts,
            int index,
            int /*target*/) const override;

        virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts, Verbosity verbosity) const override;

        virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts, Verbosity verbosity, int & index) const override;

        virtual bool requires_init_distances() const {
            return false;
        }
        virtual bool requires_goal_distances() const {
            return false;
        }

        virtual void dump_strategy_specific_options() const override;

        virtual std::string name() const override;

        static std::shared_ptr<ShrinkStrategy> create_default();
    };
}
#endif
