#ifndef TASK_TRANSFORMATION_FTS_FACTORY_H
#define TASK_TRANSFORMATION_FTS_FACTORY_H

/*
  Factory for factored transition systems.

  Takes a planning task and produces a factored transition system that
  represents the planning task. This provides the main bridge from
  planning tasks to the concepts on which merge-and-shrink abstractions
  are based (transition systems, labels, etc.). The "internal" classes of
  merge-and-shrink should not need to know about planning task concepts.
*/

#include <memory>
#include <vector>

namespace task_representation {
class Labels;
class SASTask;
class TransitionSystem;
}

namespace task_transformation {
class Distances;
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;
enum class Verbosity;

extern std::pair<std::unique_ptr<task_representation::Labels>,
                 std::vector<std::unique_ptr<task_representation::TransitionSystem>>>
    create_labels_and_transition_systems(
        const task_representation::SASTask &sas_task);

extern std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> create_mas_representations(
    const std::vector<std::unique_ptr<task_representation::TransitionSystem>>
        &transition_systems);

extern std::vector<std::unique_ptr<Distances>> create_distances(
    const std::vector<std::unique_ptr<task_representation::TransitionSystem>>
        &transition_systems);
}

#endif
