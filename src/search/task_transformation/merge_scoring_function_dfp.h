#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_DFP_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_DFP_H

#include "merge_scoring_function.h"

namespace task_transformation {
class MergeScoringFunctionDFP : public MergeScoringFunction {
    std::vector<int> compute_label_ranks(
        const FactoredTransitionSystem &fts, int index) const;
protected:
    virtual std::string name() const override;
public:
    MergeScoringFunctionDFP() = default;
    virtual ~MergeScoringFunctionDFP() override = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return true;
    }
};
}

#endif
