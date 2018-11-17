#ifndef TASK_TRANSFORMATION_SHRINK_COMPOSITE_H
#define TASK_TRANSFORMATION_SHRINK_COMPOSITE_H

#include "shrink_strategy.h"
#include <memory>

namespace options{
    class Options;
}

namespace task_transformation {
class ShrinkComposite : public ShrinkStrategy {
    const std::vector<std::shared_ptr<ShrinkStrategy> > strategies;

public:
    ShrinkComposite(const options::Options &opts);
    ShrinkComposite(const std::vector<std::shared_ptr<ShrinkStrategy> > & strategies);
    
    virtual ~ShrinkComposite() = default;


    
    virtual StateEquivalenceRelation compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index,
        int target_size) const override;


    virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity) const override;
    
    virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity, int & index) const override;

    virtual bool requires_init_distances() const override;
    virtual bool requires_goal_distances() const override;

    virtual void dump_strategy_specific_options() const override;

    virtual std::string name() const override;
};
}

#endif
