#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H

#include <vector>
#include <memory>
#include "../task_representation/labels.h"
#include "numeric_simulation_relation.h"
#include "numeric_label_relation.h"
#include "../task_representation/fts_task.h"
#include "../task_representation/state.h"
#include "../utils/timer.h"

using namespace task_representation;

/* class LTSComplex; */
class Operator;

class SearchProgress;

template<typename T>
class NumericDominanceRelationBuilder {

    const std::vector<std::unique_ptr<TransitionSystem>> tss;

    //Auxiliary data-structures to perform successor pruning
    mutable std::set<int> relevant_simulations;
    mutable std::vector<int> parent, parent_ids, succ, succ_ids;
    mutable std::vector<T> values_initial_state_against_parent;

    //Auxiliar data structure to compare against initial state
    std::vector<int> initial_state, initial_state_ids;

    const int truncate_value;
    const int max_simulation_time;
    const int min_simulation_time, max_total_time;
    const int max_lts_size_to_compute_simulation;

public:
    NumericDominanceRelation<T> compute_ld_simulation(const std::vector<std::unique_ptr<TransitionSystem>> &tss_, const Labels &labels, bool dump) {

    }

};

/*
 * Class that represents the collection of simulation relations for a
 * factored LTS. Uses unique_ptr so that it owns the simulations and
 * it cannot be copied away.
 */
template<typename T>
class NumericDominanceRelation {

    NumericLabelRelation<T> label_dominance;
    std::shared_ptr<TauLabelManager<T>> tau_labels;

protected:
    std::vector<std::unique_ptr<NumericSimulationRelation<T>>> simulations;
    std::vector<int> simulation_of_variable;
    T total_max_value;

    std::unique_ptr<NumericSimulationRelation<T>> init_simulation(int ts_id);

public:
    NumericDominanceRelation(const std::shared_ptr<FTSTask>& task_,
                             int truncate_value_,
                             int max_simulation_time_,
                             int min_simulation_time_,
                             int max_total_time_,
                             int max_lts_size_to_compute_simulation_,
                             int num_labels_to_use_dominates_in,
                             std::shared_ptr<TauLabelManager<T>> tau_label_mgr) :
            task(task_),
            truncate_value(truncate_value_),
            max_simulation_time(max_simulation_time_),
            min_simulation_time(min_simulation_time_),
            max_total_time(max_total_time_),
            max_lts_size_to_compute_simulation(max_lts_size_to_compute_simulation_),
            label_dominance(task->get_labels(), num_labels_to_use_dominates_in), tau_labels(tau_label_mgr) {


    }

    void compute_ld_simulation(std::vector<TransitionSystem> &tss, const Labels &labels, bool dump);


    bool action_selection_pruning (const State & state, std::vector<OperatorID>& applicable_operators) const;

    void prune_dominated_by_parent_or_initial_state (const State & op_id,
				   std::vector<OperatorID> & applicable_operators, bool parent_ids_stored,
						     bool compare_against_parent, bool compare_against_initial_state) const;


    //Methods to use the dominance relation
    //int get_cost(const State &state) const;

    //bool parent_dominates_successor(const State & parent, const Operator *op) const;
    bool dominates(const State &t, const State &s, int g_diff) const;

    bool dominates_parent(const std::vector<int> &state, const std::vector<int> &parent_state, int action_cost) const;

    void init();



    //Methods to access the underlying simulation relations
    const std::vector<std::unique_ptr<NumericSimulationRelation<T>>> &get_simulations() const {
        return simulations;
    }

    int size() const {
        return simulations.size();
    }

    NumericSimulationRelation<T> &operator[](int index) {
        return *(simulations[index]);
    }

    const NumericSimulationRelation<T> &operator[](int index) const {
        return *(simulations[index]);
    }

    bool strictly_dominates(const State &dominating_state,
                            const State &dominated_state) const;

    bool strictly_dominates_initial_state(const State &) const;

    void set_initial_state (const std::vector<int>& state);
};



#endif
