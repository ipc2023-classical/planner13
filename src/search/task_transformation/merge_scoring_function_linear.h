#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_LINEAR_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_LINEAR_H

#include "merge_scoring_function.h"

namespace task_transformation {
class MergeScoringFunctionLinear : public MergeScoringFunction {
public:
    virtual std::string name() const override;
public:
    MergeScoringFunctionLinear() = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return false;
    }
};
}

#endif
