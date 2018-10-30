#ifndef TASK_TRANSFORMATION_MERGE_STRATEGY_FACTORY_H
#define TASK_TRANSFORMATION_MERGE_STRATEGY_FACTORY_H

#include <memory>
#include <string>

namespace task_representation {
class SASTask;
}

using namespace task_representation;

namespace task_transformation {
class FactoredTransitionSystem;
class MergeStrategy;

class MergeStrategyFactory {
protected:
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategyFactory() = default;
    virtual ~MergeStrategyFactory() = default;
    void dump_options() const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const SASTask &sas_task,
        const FactoredTransitionSystem &fts) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};
}

#endif
