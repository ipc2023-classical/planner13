#ifndef FAST_DOWNWARD_DOMINANCE_FUNCTION_BUILDER_H
#define FAST_DOWNWARD_DOMINANCE_FUNCTION_BUILDER_H

#include <utility>
#include <vector>
#include <memory>
#include "../task_representation/labels.h"
#include "local_dominance_function.h"
#include "label_dominance_function.h"
#include "../task_representation/fts_task.h"
#include "../task_representation/state.h"
#include "../utils/timer.h"

#include "../option_parser.h"
#include "../plugin.h"

namespace task_transformation {
    class FactoredTransitionSystem;
}

namespace dominance {

    class TauLabelManager;

    // DominanceFunctionBuilder is the class that configures how to obtain a dominance function from a composite task.
    class DominanceFunctionBuilder {
        const int truncate_value;
        const int max_simulation_time;
        const int min_simulation_time;
        const int max_total_time;
        const int max_lts_size_to_compute_simulation;
        const int num_labels_to_use_dominates_in;
        const bool dump;

        std::shared_ptr<TauLabelManager> tau_label_manager;

    public:
        explicit DominanceFunctionBuilder(const options::Options &opts);

        //TODO: Check if only reachability is actually used. Otherwise, consider removing it.
        template<typename TCostType>
        std::shared_ptr<DominanceFunction<TCostType>> compute_dominance_function(const task_representation::FTSTask & fts_task,
                                                                                 bool only_reachability = false) const;

        template<typename TCostType>
        std::shared_ptr<DominanceFunction<TCostType>> compute_dominance_function(const FactoredTransitionSystem & fts_task,
                                                                                 bool only_reachability = false) const;

        template<typename TCostType>
        std::shared_ptr<DominanceFunction<TCostType>>
        compute_dominance_function(const std::vector<std::unique_ptr<task_representation::TransitionSystem>> & tss,
                                   const task_representation::Labels & labels, bool only_reachability ) const;

        void dump_options() const;

    };

}

#endif //FAST_DOWNWARD_DOMINANCE_FUNCTION_BUILDER_H
