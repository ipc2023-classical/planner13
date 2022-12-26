#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H

#include "../pruning_method.h"

#include "dominance_check.h"
#include "dominance_function.h"
#include "dominance_function_builder.h"

using namespace task_representation;

namespace options {
class OptionParser;

class Options;
}

namespace dominance {

template<typename T>
class NumericDominancePruning : public PruningMethod {
protected:

    bool initialized;
    std::shared_ptr<DominanceFunctionBuilder> dominance_function_builder;
    std::shared_ptr<DominanceFunction<T>> numeric_dominance_relation;
    DominanceCheck<T> dominance_check;

    const bool prune_dominated_by_parent;
    const bool prune_dominated_by_initial_state;
    const bool prune_successors;

    const bool dump;
    const bool exit_after_preprocessing;

    void dump_options() const;

    bool apply_pruning() const;

public:
    void initialize(const std::shared_ptr<task_representation::FTSTask> &task) override;

    //Methods for pruning explicit search
//    virtual void prune_applicable_operators(const State & state, int g, std::vector<const Operator *> & operators, SearchProgress & search_progress) override;
    //virtual bool prune_generation(const State &state, int g, const State &parent, int action_cost) override;
    //virtual bool prune_expansion (const State &state, int g) override;

    void prune_operators(const State &state, std::vector<OperatorID> &op_ids) override;

    //virtual bool is_dead_end(const State &state) override;

    //virtual int compute_heuristic(const State &state) override;

    explicit NumericDominancePruning(const options::Options &opts);

    ~NumericDominancePruning() override = default;

    //virtual void print_statistics() override;

    //virtual bool proves_task_unsolvable() const override {
    //    return true;
    //}
};
}

#endif
