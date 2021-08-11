#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_RELATION_H

#include <utility>
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

// This class is for building the numeric dominance relation
template<typename T>
class NumericDominanceRelationBuilder {

    const std::vector<TransitionSystem>& tss;
    const Labels& labels;

    std::shared_ptr<NumericDominanceRelation<T>> ndr;

    const int truncate_value;
    const int max_simulation_time;
    const int min_simulation_time, max_total_time;
    const int max_lts_size_to_compute_simulation;

public:

    NumericDominanceRelationBuilder<T>(const std::vector<TransitionSystem> &_tss, const Labels& _labels,
                                       int truncate_value_,
                                       int max_simulation_time_,
                                       int min_simulation_time_,
                                       int max_total_time_,
                                       int max_lts_size_to_compute_simulation_,
                                       int num_labels_to_use_dominates_in,
                                       std::shared_ptr<TauLabelManager<T>> tau_label_mgr) :
        tss(_tss), labels(_labels),
        ndr(std::make_shared<NumericDominanceRelation<T>>(tss.size(), labels, num_labels_to_use_dominates_in, tau_label_mgr)),
        truncate_value(truncate_value_),
        max_simulation_time(max_simulation_time_),
        min_simulation_time(min_simulation_time_),
        max_total_time(max_total_time_),
        max_lts_size_to_compute_simulation(max_lts_size_to_compute_simulation_) {

    }

    std::shared_ptr<NumericDominanceRelation<T>> compute_ld_simulation(bool dump);

    std::unique_ptr<NumericSimulationRelation<T>> init_simulation(int ts_id);

    void init();

    void set_initial_state();
};

/*
 * Class that represents the collection of simulation relations for a
 * factored LTS. Uses unique_ptr so that it owns the simulations and
 * it cannot be copied away.
 */
template<typename T>
class NumericDominanceRelation {
    friend NumericDominanceRelationBuilder<T>;

    NumericLabelRelation<T> label_relation;
    std::shared_ptr<TauLabelManager<T>> tau_labels;

    //Auxiliar data structure to compare against initial state
    std::vector<int> initial_state;

    //Auxiliary data-structures to perform successor pruning
    mutable std::set<int> relevant_simulations;
    mutable std::vector<int> parent, succ;
    mutable std::vector<T> values_initial_state_against_parent;

    std::vector<std::unique_ptr<NumericSimulationRelation<T>>> simulations;
    std::vector<int> simulation_of_variable;
    T total_max_value;

    int num_transition_systems;

public:
    NumericDominanceRelation(int num_transition_systems, const Labels& labels, int num_labels_to_use_dominates_in, std::shared_ptr<TauLabelManager<T>> tau_label_mgr) :
            label_relation(labels, num_labels_to_use_dominates_in), tau_labels(tau_label_mgr),
            num_transition_systems(num_transition_systems) {

        parent.resize(num_transition_systems);
        succ.resize(num_transition_systems);
        values_initial_state_against_parent.resize(num_transition_systems);

    }

    bool action_selection_pruning (const std::shared_ptr<FTSTask> fts_task, const State & state,
                                   std::vector<OperatorID>& applicable_operators) const;

    void prune_dominated_by_parent_or_initial_state (const std::shared_ptr<FTSTask> fts_task, const State & op_id,
				   std::vector<OperatorID> & applicable_operators, bool parent_ids_stored,
						     bool compare_against_parent, bool compare_against_initial_state) const;


    //Methods to use the dominance relation
    //int get_cost(const State &state) const;

    //bool parent_dominates_successor(const State & parent, const Operator *op) const;
    bool dominates(const State &t, const State &s, int g_diff) const;

    bool dominates_parent(const std::vector<int> &state, const std::vector<int> &parent_state, int action_cost) const;

    bool propagate_transition_pruning(int ts_id, const TransitionSystem &ts, int src, LabelID l_id, int target) const;

    int size() const {
        return num_transition_systems;
    }

    NumericLabelRelation<T> &get_label_relation() {
        return label_relation;
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
};



#endif
