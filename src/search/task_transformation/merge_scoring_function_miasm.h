#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_MIASM_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_MIASM_H

#include "merge_scoring_function.h"

#include <memory>

namespace options {
class Options;
}

namespace task_transformation {
class ShrinkStrategy;
class MergeScoringFunctionMIASM : public MergeScoringFunction {
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    const int max_states;
    const int max_states_before_merge;
    const int shrink_threshold_before_merge;
protected:
    virtual std::string name() const override;
public:
    explicit MergeScoringFunctionMIASM(const options::Options &options);
    virtual ~MergeScoringFunctionMIASM() override = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;

    virtual bool requires_init_distances() const override {
        return true;
    }

    virtual bool requires_goal_distances() const override {
        return true;
    }
};
}

#endif
