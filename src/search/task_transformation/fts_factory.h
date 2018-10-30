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

namespace task_representation {
class SASTask;
}

namespace task_transformation {
class FactoredTransitionSystem;
enum class Verbosity;

extern FactoredTransitionSystem create_factored_transition_system(
    const task_representation::SASTask &sas_task, Verbosity verbosity);
}

#endif
