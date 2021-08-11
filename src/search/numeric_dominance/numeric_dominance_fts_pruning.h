
#ifndef FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H
#define FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H

#include "numeric_dominance_relation.h"

namespace numeric_dominance {

class FTSTransitionPruning {
public:
    FTSTransitionPruning() = default;

    // Returns transition system in which pruning has been done
    virtual std::vector<int> prune_transitions(FactoredTransitionSystem &fts) const = 0;
};

template<typename T>
class NumericDominanceFTSPruning : public FTSTransitionPruning {
    const int truncate_value;
    const int max_simulation_time;
    const int min_simulation_time;
    const int max_total_time;

    const int max_lts_size_to_compute_simulation;
    const int num_labels_to_use_dominates_in;

    std::shared_ptr<TauLabelManager<T>> tau_labels;

    bool prune_transitions_before_main_loop;
    bool prune_transitions_after_main_loop;

    bool dump;

    std::shared_ptr<NumericDominanceRelation<T>> compute_dominance_relation(const FactoredTransitionSystem &fts) const;
public:

    virtual ~NumericDominanceFTSPruning() = default;

    explicit NumericDominanceFTSPruning(options::Options opts);

    std::vector<int> prune_dominated_transitions(FactoredTransitionSystem &fts, std::shared_ptr<NumericDominanceRelation<T>> ndrel) const;

    std::vector<int> prune_transitions(FactoredTransitionSystem &fts) const override;
};

}




#endif //FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H
