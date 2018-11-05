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
class FTSTask;
class TransitionSystem;
}

using namespace task_representation;

namespace task_transformation {
class FactoredTransitionSystem;
class LabelMap;
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
    bool shrink_atomic_fts;
    int num_states_to_trigger_shrinking;
    int num_states_to_terminate_main_loop;
    std::shared_ptr<LabelReduction> label_reduction;

    // Options for pruning
//    const bool prune_unreachable_states;
//    const bool prune_irrelevant_states;

    const Verbosity verbosity;
    long starting_peak_memory;

    // Return true iff fts has been detected to be unsolvable.
//    bool prune_fts(FactoredTransitionSystem &fts, const utils::Timer &timer) const;
    void statistics(int maximum_intermediate_size) const;
    void main_loop(
        FactoredTransitionSystem &fts,
        const FTSTask &fts_task,
        const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
public:
    explicit MergeAndShrinkAlgorithm(const options::Options &opts);
    void dump_options() const;
    void warn_on_unusual_options() const;
    FactoredTransitionSystem build_factored_transition_system(const FTSTask &fts_task);
    std::unique_ptr<LabelMap> extract_label_map();
};

extern void add_merge_and_shrink_algorithm_options_to_parser(options::OptionParser &parser);
}

#endif
