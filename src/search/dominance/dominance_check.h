#ifndef FAST_DOWNWARD_DOMINANCE_CHECK_H
#define FAST_DOWNWARD_DOMINANCE_CHECK_H

#include <memory>
#include <vector>
#include <set>

#include "dominance_function.h"

namespace dominance {
    // DominanceCheck builds a wrapper around a Dominance Function, providing methods in order to quickly check whether
    // a state is dominated
    template <typename T>
    class DominanceCheck {
        std::shared_ptr<DominanceFunction<T>> qdf;

        //Auxiliar data structure to compare against initial state
        std::vector<int> initial_state;

        //Auxiliary data-structures to perform successor pruning
        mutable std::set<int> relevant_simulations;
        mutable std::vector<int> parent, succ;
        mutable std::vector<T> values_initial_state_against_parent;

    public:
        //Initialize must be called before calling any other method
        void initialize (std::shared_ptr<DominanceFunction<T>> qdf, const FTSTask & task);

        bool strictly_dominates_initial_state(const State &) const;

        bool action_selection_pruning(const FTSTask & task, const State &state, std::vector<OperatorID> &applicable_operators) const;

        void prune_dominated_by_parent_or_initial_state(const FTSTask & task, const State &op_id,
                                                        std::vector<OperatorID> &applicable_operators,
                                                        bool parent_ids_stored,
                                                        bool compare_against_parent,
                                                        bool compare_against_initial_state) const;

    };
}

#endif //FAST_DOWNWARD_DOMINANCE_CHECK_H
