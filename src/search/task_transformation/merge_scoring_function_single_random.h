#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_SINGLE_RANDOM_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_SINGLE_RANDOM_H

#include "merge_scoring_function.h"

#include <memory>

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace task_transformation {
class TransitionSystem;
class MergeScoringFunctionSingleRandom : public MergeScoringFunction {
    int random_seed; // only for dump options
    std::shared_ptr<utils::RandomNumberGenerator> rng;
protected:
    virtual std::string name() const override;
    virtual void dump_function_specific_options() const override;
public:
    explicit MergeScoringFunctionSingleRandom(const options::Options &options);
    virtual ~MergeScoringFunctionSingleRandom() override = default;
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
