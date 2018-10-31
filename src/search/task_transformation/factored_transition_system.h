#ifndef TASK_TRANSFORMATION_FACTORED_TRANSITION_SYSTEM_H
#define TASK_TRANSFORMATION_FACTORED_TRANSITION_SYSTEM_H

#include "types.h"

#include <memory>
#include <vector>

namespace task_representation {
class Labels;
class TransitionSystem;
}

using namespace task_representation;

namespace task_transformation {
class Distances;
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;

class FTSConstIterator {
    /*
      This class allows users to easily iterate over the active indices of
      a factored transition system.
    */
    const FactoredTransitionSystem &fts;
    // current_index is the actual iterator
    int current_index;

    void next_valid_index();
public:
    FTSConstIterator(const FactoredTransitionSystem &fts, bool end);
    void operator++();

    int operator*() const {
        return current_index;
    }

    bool operator==(const FTSConstIterator &rhs) const {
        return current_index == rhs.current_index;
    }

    bool operator!=(const FTSConstIterator &rhs) const {
        return current_index != rhs.current_index;
    }
};

class FactoredTransitionSystem {
    std::unique_ptr<Labels> labels;
    // Entries with nullptr have been merged.
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;
    std::vector<std::unique_ptr<Distances>> distances;
    const bool compute_init_distances;
    const bool compute_goal_distances;
    int num_active_entries;

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
    FactoredTransitionSystem(
        std::unique_ptr<Labels> labels,
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
        std::vector<std::unique_ptr<Distances>> &&distances,
        const bool compute_init_distances,
        const bool compute_goal_distances,
        Verbosity verbosity);
    FactoredTransitionSystem(FactoredTransitionSystem &&other);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem &operator=(
        const FactoredTransitionSystem &) = delete;

    // Merge-and-shrink transformations.
    /*
      Apply the given label mapping to the factored transition system by
      updating all transitions of all transition systems. Only for the factor
      at combinable_index, the local equivalence relation over labels must be
      recomputed; for all factors, all labels that are combined by the label
      mapping have been locally equivalent already before.
    */
    void apply_label_mapping(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        int combinable_index);

    /*
      Apply the given state equivalence relation to the transition system at
      index if it would reduce its size. If the transition system was shrunk,
      update the other components of the factor (distances, MSR) and return
      true, otherwise return false.

      Note that this method is also suitable to be used for a prune
      transformation. All states not mentioned in the state equivalence
      relation are pruned.
    */
    bool apply_abstraction(
        int index,
        const StateEquivalenceRelation &state_equivalence_relation,
        Verbosity verbosity);

    /*
      Merge the two factors at index1 and index2.
    */
    int merge(
        int index1,
        int index2,
        Verbosity verbosity);

    /*
      Various extraction methods, rendering the FTS invalid.
    */
    std::unique_ptr<TransitionSystem> extract_transition_system(int index);
    std::unique_ptr<Labels> extract_labels();

    void statistics(int index) const;
    void dump(int index) const;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    const Distances &get_distances(int index) const {
        return *distances[index];
    }

    /*
      A factor is solvabe iff the distance of the initial state to some goal
      state is not infinity. Technically, the distance is infinity either if
      the information of Distances is infinity or if the initial state is
      pruned.
    */
    bool is_factor_solvable(int index) const;

    int get_num_active_entries() const {
        return num_active_entries;
    }

    // Used by LabelReduction and MergeScoringFunctionDFP
    const Labels &get_labels() const {
        return *labels;
    }

    // The following methods are used for iterating over the FTS
    FTSConstIterator begin() const {
        return FTSConstIterator(*this, false);
    }

    FTSConstIterator end() const {
        return FTSConstIterator(*this, true);
    }

    int get_size() const {
        return transition_systems.size();
    }

    bool is_active(int index) const;
};
}

#endif