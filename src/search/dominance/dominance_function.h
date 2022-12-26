#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H

#include <utility>
#include <vector>
#include <memory>

#include "../task_representation/labels.h"
#include "../task_representation/fts_task.h"
#include "../task_representation/state.h"

#include "local_dominance_function.h"
#include "label_dominance_function.h"

#include "../utils/timer.h"

namespace dominance {

/*
 * Class that represents the collection of local dominance functions for a factored LTS.
 * Uses unique_ptr so that it owns the local functions and it cannot be copied away.
 */
    template<typename TCost>
    class DominanceFunction {
        std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> local_functions;
        LabelDominanceFunction<TCost> label_relation;

    public:
        DominanceFunction(std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> _local_functions,
                          LabelDominanceFunction<TCost> _label_relation) :
                local_functions (std::move(_local_functions)), label_relation (_label_relation) {
        }

        bool dominates(const State &t, const State &s, int g_diff) const;
        bool dominates_parent(const std::vector<int> &state, const std::vector<int> &parent_state, int action_cost) const;
        bool strictly_dominates(const State &dominating_state, const State &dominated_state) const;
        bool propagate_transition_pruning(int ts_id, const TransitionSystem &ts, int src, LabelID l_id, int target) const;

     //TODO: Add evaluation returning TCost?

        const LabelDominanceFunction<TCost> &get_label_relation() {
            return label_relation;
        }

        size_t size() const {
            return local_functions.size();
        }

        LocalDominanceFunction<TCost> &operator[](int index) {
            return *(local_functions[index]);
        }

        const LocalDominanceFunction<TCost> &operator[](int index) const {
            return *(local_functions[index]);
        }
    };

}

#endif
