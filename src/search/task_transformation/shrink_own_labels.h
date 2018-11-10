#ifndef MERGE_AND_SHRINK_SHRINK_OWN_LABELS_H
#define MERGE_AND_SHRINK_SHRINK_OWN_LABELS_H

#include "shrink_strategy.h"

#include<memory>

namespace options{
    class Options;
}

namespace task_transformation {
    
    class ShrinkOwnLabels : public ShrinkStrategy {
        const bool perform_sg_shrinking;
        const bool preserve_optimality;

        bool is_own_label (const FactoredTransitionSystem & fts, int label, int ts) const;

    public:
        ShrinkOwnLabels(const options::Options &opts);

        virtual ~ShrinkOwnLabels() override;

        virtual StateEquivalenceRelation compute_equivalence_relation(
            const FactoredTransitionSystem &fts,
            int index,
            int target_size) const override;

        
        virtual StateEquivalenceRelation compute_equivalence_relation(
            const task_representation::TransitionSystem &ts,
            const Distances &distances,
            int target_size) const;

        
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
