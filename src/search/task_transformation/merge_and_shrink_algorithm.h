#ifndef TASK_TRANSFORMATION_MERGE_AND_SHRINK_ALGORITHM_H
#define TASK_TRANSFORMATION_MERGE_AND_SHRINK_ALGORITHM_H

#include "../operator_cost.h"
#include <memory>
#include <vector>
#include <set>

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
class LabelID;
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
    // TODO: when the option parser supports it, the following should become unique
    // pointers.
    std::shared_ptr<MergeStrategyFactory> merge_strategy_factory;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    const bool shrink_atomic_fts;
    const bool run_atomic_loop;

    const bool run_final_lr_shrink;
    const bool reduce_labels_final_fts;
    const bool shrink_final_fts;
    const bool run_final_loop;

    int num_states_to_trigger_shrinking;
    int max_states;
    std::shared_ptr<LabelReduction> label_reduction;

    // Options for pruning
    const bool prune_unreachable_states;
    const bool prune_irrelevant_states;
    const bool prune_transitions_from_goal;

    const Verbosity verbosity;

    const bool run_main_loop;
    // Options related to computing partial abstractions
    const double max_time;
    const int num_transitions_to_abort;
    const int num_transitions_to_exclude;

    const OperatorCost cost_type;

    //std::unique_ptr<task_transformation::LabelMap> label_map;
    long starting_peak_memory;

    bool ran_out_of_time(const utils::Timer &timer) const;
    bool too_many_transitions(const FactoredTransitionSystem &fts, int index) const;
    bool too_many_transitions(const FactoredTransitionSystem &fts) const;
    bool exclude_if_too_many_transitions() const;
    // Return true iff fts has been detected to be unsolvable.
    bool prune_fts(FactoredTransitionSystem &fts, const utils::Timer &timer) const;
    bool check_dead_labels(FactoredTransitionSystem &fts,
                           const std::vector<int> & ts_to_check_initially) const;

    bool remove_dead_labels(FactoredTransitionSystem &fts,
                            std::set<LabelID> & dead_labels) const;

    bool check_dead_labels(FactoredTransitionSystem &fts, int index) const;

    void statistics(int maximum_intermediate_size) const;
    void main_loop(
        FactoredTransitionSystem &fts,
        const FTSTask &fts_task,
        const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;

    void apply_full_label_reduction_and_shrinking(FactoredTransitionSystem &fts,
                                                  bool apply_label_reduction, bool apply_shrink,
                                                  bool run_loop, const utils::Timer &timer,
                                                  Verbosity verbosity);

public:
    explicit MergeAndShrinkAlgorithm(const options::Options &opts);
    void dump_options() const;
    void warn_on_unusual_options() const;
    FactoredTransitionSystem build_factored_transition_system(const std::shared_ptr<task_representation::FTSTask> &fts_task, bool lossy_mapping);
};

extern void add_merge_and_shrink_algorithm_options_to_parser(options::OptionParser &parser);
extern void add_transition_system_size_limit_options_to_parser(options::OptionParser &parser);
extern void handle_shrink_limit_options_defaults(options::Options &opts);
}

#endif
