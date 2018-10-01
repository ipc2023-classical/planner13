#ifndef FTS_REPRESENTATION_FTS_TASK_H
#define FTS_REPRESENTATION_FTS_TASK_H

#include "types.h"

#include "fts_operators.h"

#include <memory>
#include <vector>

class GlobalState;

namespace task_representation {
struct FactPair;
class Labels;
class SASTask;
class State;
class TransitionSystem;


class FTSTask {
    std::unique_ptr<Labels> labels;
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;

    /*
      Assert that the factor at the given index is in a consistent state, i.e.
      that there is a transition system, a distances object, and an MSR.
    */
    void assert_index_valid(int index) const;

    /*
      We maintain the invariant that for all factors, distances are always
      computed and all transitions are grouped according to locally equivalent
      labels.
    */
    bool is_component_valid(int index) const;

    void assert_all_components_valid() const;
public:
    FTSTask(const SASTask & sas_task); //Creates the fts task from the SAS+ representation
    FTSTask(FTSTask &&other) = default;
    ~FTSTask();
    FTSTask(const FTSTask &) = default;
    FTSTask &operator=(const FTSTask &) = default;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    /*
      A factor is solvable iff the distance of the initial state to some goal
      state is not infinity. Technically, the distance is infinity either if
      the information of Distances is infinity or if the initial state is
      pruned.
    */
    bool is_factor_solvable(int index) const;

    // Used by LabelReduction and MergeScoringFunctionDFP
    const Labels &get_labels() const {
        return *labels;
    }

    int get_size() const {
        return transition_systems.size();
    }

    int get_label_cost(LabelID label) const;

    int get_min_operator_cost() const;

    int get_num_labels() const;

    std::string get_fact_name(const FactPair & fact) const;

    bool are_facts_mutex(const FactPair & f1, const FactPair & f2) const;

    std::vector<int> get_initial_state_data() const;

    State get_initial_state() const;

    bool is_goal_state (const GlobalState & state) const;

    bool has_axioms() const {
        return false;
    }
};
}


#endif
