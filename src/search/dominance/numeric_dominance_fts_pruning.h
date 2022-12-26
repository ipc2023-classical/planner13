
#ifndef FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H
#define FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H

#include "dominance_function.h"

namespace dominance {

    class DominanceFunctionBuilder;

class FTSTransitionPruning {
    public:
    FTSTransitionPruning() = default;

    // Returns transition system in which pruning has been done
    virtual std::vector<int> prune_transitions(FactoredTransitionSystem &fts) const = 0;
};

template<typename T>
class NumericDominanceFTSPruning : public FTSTransitionPruning {
    std::shared_ptr<DominanceFunctionBuilder> dominance_function_builder;

    bool prune_transitions_before_main_loop;
    bool prune_transitions_after_main_loop;

    bool dump;

    std::vector<int> prune_dominated_transitions(FactoredTransitionSystem &fts, std::shared_ptr<DominanceFunction<T>> ndrel) const;

public:
    explicit NumericDominanceFTSPruning(const options::Options & opts);
    virtual ~NumericDominanceFTSPruning() = default;

    std::vector<int> prune_transitions(FactoredTransitionSystem &fts) const override;
};

}




#endif //FAST_DOWNWARD_NUMERIC_DOMINANCE_FTS_PRUNING_H
