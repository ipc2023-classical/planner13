#ifndef TASK_TRANSFORMATION_UTILS_H
#define TASK_TRANSFORMATION_UTILS_H

#include "types.h"

#include <vector>

namespace task_representation {
class TransitionSystem;
}

using namespace task_representation;

namespace task_transformation {
class FactoredTransitionSystem;
class ShrinkStrategy;

/*
  Compute target sizes for shrinking two transition systems with sizes size1
  and size2 before they are merged. Use the following rules:
  1) Right before merging, the transition systems may have at most
     max_states_before_merge states.
  2) Right after merging, the product may have most max_states_after_merge
     states.
  3) Transition systems are shrunk as little as necessary to satisfy the above
     constraints. (If possible, neither is shrunk at all.)
  There is often a Pareto frontier of solutions following these rules. In this
  case, balanced solutions (where the target sizes are close to each other)
  are preferred over less balanced ones.
*/
extern std::pair<int, int> compute_shrink_sizes(
    int size1,
    int size2,
    int max_states_before_merge,
    int max_states_after_merge);

/*
  This function first determines if the current size of the factor is larger
  than the number of states to trigger shrinking. If so, it will "shrink" the
  factor, imposing its current size as the allowed maxmimum size in order to
  not force and information-lossy shrinking.
*/
extern bool shrink_factor(
    FactoredTransitionSystem &fts,
    int index,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosit,
    int num_states_to_trigger_shrinking);

/*
  Prune unreachable and/or irrelevant states of the factor at index. This
  requires that init and/or goal distances have been computed accordingly.
  Return true iff any states have been pruned.

  TODO: maybe this functionality belongs to a new class PruneStrategy.
*/
//extern bool prune_step(
//    FactoredTransitionSystem &fts,
//    int index,
//    bool prune_unreachable_states,
//    bool prune_irrelevant_states,
//    Verbosity verbosity);

/*
  Compute the abstraction mapping based on the given state equivalence
  relation.
*/
extern std::vector<int> compute_abstraction_mapping(
    int num_states,
    const StateEquivalenceRelation &equivalence_relation);

extern bool is_goal_relevant(const TransitionSystem &ts);
}

#endif
