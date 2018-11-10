#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_UTILS_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_UTILS_H

#include <memory>

namespace task_representation {
class TransitionSystem;
}

namespace task_transformation {
class FactoredTransitionSystem;
class ShrinkStrategy;

/*
  Copy the two transition systems at the given indices, possibly shrink them
  according to the same rules as merge-and-shrink does, and return their
  product.
*/
extern std::unique_ptr<task_representation::TransitionSystem> shrink_before_merge_externally(
    const FactoredTransitionSystem &fts,
    int index1,
    int index2,
    const ShrinkStrategy &shrink_strategy,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge);
}

#endif
