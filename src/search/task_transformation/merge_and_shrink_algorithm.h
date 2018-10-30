#ifndef TASK_TRANSFORMATION_MERGE_AND_SHRINK_ALGORITHM_H
#define TASK_TRANSFORMATION_MERGE_AND_SHRINK_ALGORITHM_H

#include <memory>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class Timer;
}

namespace task_representation {
class SASTask;
class TransitionSystem;
}

namespace task_transformation {
class FactoredTransitionSystem;
class LabelReduction;
class MergeAndShrinkRepresentation;
class MergeStrategyFactory;
class ShrinkStrategy;
enum class Verbosity;

class MergeAndShrinkAlgorithm {
    // TODO: when the option parser supports it, the following should become
    // unique pointers.
    std::shared_ptr<MergeStrategyFactory> merge_strategy_factory;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<LabelReduction> label_reduction;

    // Options for shrinking
    // Hard limit: the maximum size of a transition system at any point.
    const int max_states;
    // Hard limit: the maximum size of a transition system before being merged.
    const int max_states_before_merge;
    /* A soft limit for triggering shrinking even if the hard limits
       max_states and max_states_before_merge are not violated. */
    const int shrink_threshold_before_merge;

    // Options for pruning
    const bool prune_unreachable_states;
    const bool prune_irrelevant_states;

    const Verbosity verbosity;
    long starting_peak_memory;

    // Return true iff fts has been detected to be unsolvable.
    bool prune_fts(FactoredTransitionSystem &fts, const utils::Timer &timer) const;
    void statistics(int maximum_intermediate_size) const;
    void main_loop(
        FactoredTransitionSystem &fts,
        const task_representation::SASTask &sas_task,
        const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
public:
    explicit MergeAndShrinkAlgorithm(const options::Options &opts);
    void dump_options() const;
    void warn_on_unusual_options() const;
    FactoredTransitionSystem build_factored_transition_system(const task_representation::SASTask &sas_task);
};

extern void add_merge_and_shrink_algorithm_options_to_parser(options::OptionParser &parser);
extern void add_transition_system_size_limit_options_to_parser(options::OptionParser &parser);
extern void handle_shrink_limit_options_defaults(options::Options &opts);
}

#endif
