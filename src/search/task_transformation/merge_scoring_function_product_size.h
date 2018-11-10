#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_PRODUCT_SIZE_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_PRODUCT_SIZE_H

#include "merge_scoring_function.h"

namespace options {
class Options;
}

namespace task_transformation {
class MergeScoringFunctionProductSize : public MergeScoringFunction {
    int max_states;
protected:
    virtual std::string name() const override;
public:
    explicit MergeScoringFunctionProductSize(const options::Options &options);
    virtual ~MergeScoringFunctionProductSize() override = default;
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
