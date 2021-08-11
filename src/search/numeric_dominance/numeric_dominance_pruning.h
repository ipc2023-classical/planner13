#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H

#include "../pruning_method.h"

#include "tau_labels.h"
#include "int_epsilon.h"
#include "numeric_dominance_relation.h"

using namespace task_representation;

namespace options {
class OptionParser;

class Options;
}

namespace numeric_dominance {

template<typename T>
class NumericDominancePruning : public PruningMethod {
protected:

    bool initialized;
    std::shared_ptr<TauLabelManager<T>> tau_labels;

    const bool prune_dominated_by_parent;
    const bool prune_dominated_by_initial_state;
    const bool prune_successors;

    const int truncate_value;
    const int max_simulation_time;
    const int min_simulation_time;
    const int max_total_time;

    const int max_lts_size_to_compute_simulation;
    const int num_labels_to_use_dominates_in;

    const bool dump;
    const bool exit_after_preprocessing;

    std::shared_ptr<    NumericDominanceRelation<T>> numeric_dominance_relation;

    bool all_desactivated;
    bool activation_checked;

    int states_inserted; //Count the number of states inserted
    int states_checked; //Count the number of states inserted
    int states_pruned; //Count the number of states pruned
    int deadends_pruned; //Count the number of dead ends detected

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
