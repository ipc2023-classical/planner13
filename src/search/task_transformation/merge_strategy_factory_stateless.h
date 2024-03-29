#ifndef TASK_TRANSFORMATION_MERGE_STRATEGY_FACTORY_STATELESS_H
#define TASK_TRANSFORMATION_MERGE_STRATEGY_FACTORY_STATELESS_H

#include "merge_strategy_factory.h"

namespace options {
class Options;
}

namespace task_transformation {
class MergeSelector;
class MergeStrategyFactoryStateless : public MergeStrategyFactory {
    std::shared_ptr<MergeSelector> merge_selector;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeStrategyFactoryStateless(options::Options &options);
    virtual ~MergeStrategyFactoryStateless() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const FTSTask &fts_taskk,
        const FactoredTransitionSystem &fts) override;
    virtual bool requires_init_distances() const override;
    virtual bool requires_goal_distances() const override;
};
}

#endif
