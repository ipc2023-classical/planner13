#ifndef TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_H
#define TASK_TRANSFORMATION_MERGE_SCORING_FUNCTION_H

#include <string>
#include <vector>

namespace task_representation {
class SASTask;
}

using namespace task_representation;

namespace task_transformation {
class FactoredTransitionSystem;
class MergeScoringFunction {
protected:
    bool initialized;
    virtual std::string name() const = 0;
    virtual void dump_function_specific_options() const {}
public:
    MergeScoringFunction();
    virtual ~MergeScoringFunction() = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;

    // Overriding methods must set initialized to true.
    virtual void initialize(const SASTask &) {
        initialized = true;
    }

    void dump_options() const;
};
}

#endif
