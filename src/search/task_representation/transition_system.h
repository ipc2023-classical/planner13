#ifndef FTS_REPRESENTATION_TRANSITION_SYSTEM_H
#define FTS_REPRESENTATION_TRANSITION_SYSTEM_H

#include "types.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace task_representation {
class Distances;
class LabelEquivalenceRelation;
class LabelGroup;
class Labels;

struct Transition {
    int src;
    int target;

    Transition(int src, int target)
        : src(src), target(target) {
    }

    bool operator==(const Transition &other) const {
        return src == other.src && target == other.target;
    }

    bool operator<(const Transition &other) const {
        return src < other.src || (src == other.src && target < other.target);
    }

    // Required for "is_sorted_unique" in utilities
    bool operator>=(const Transition &other) const {
        return !(*this < other);
    }
};

struct GroupAndTransitions {
    const LabelGroup &label_group;
    const std::vector<Transition> &transitions;
    GroupAndTransitions(const LabelGroup &label_group,
                        const std::vector<Transition> &transitions)
        : label_group(label_group),
          transitions(transitions) {
    }
};

class TSConstIterator {
    /*
      This class allows users to easily iterate over both label groups and
      their transitions of a TransitionSystem. Most importantly, it hides
      the data structure used by LabelEquivalenceRelation, which could be
      easily exchanged.
    */
    const LabelEquivalenceRelation &label_equivalence_relation;
    const std::vector<std::vector<Transition>> &transitions_by_group_id;
    // current_group_id is the actual iterator
    LabelGroupID current_group_id;

    void next_valid_index();
public:
    TSConstIterator(const LabelEquivalenceRelation &label_equivalence_relation,
                    const std::vector<std::vector<Transition>> &transitions_by_group_id,
                    bool end);
    void operator++();
    GroupAndTransitions operator*() const;

    bool operator==(const TSConstIterator &rhs) const {
        return current_group_id == rhs.current_group_id;
    }

    bool operator!=(const TSConstIterator &rhs) const {
        return current_group_id != rhs.current_group_id;
    }
};

class TransitionSystem {
private:
    /*
      The following two attributes are only used for output.

      - num_variables: total number of variables in the factored
        transition system

      - incorporated_variables: variables that contributed to this
        transition system
    */
    const int num_variables;
    std::vector<int> incorporated_variables;

    /*
      All locally equivalent labels are grouped together, and their
      transitions are only stored once for every such group, see below.

      LabelEquivalenceRelation stores the equivalence relation over all
      labels of the underlying factored transition system.
    */
    std::unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;

    /*
      The transitions of a label group are indexed via its ID. The ID of a
      group does not change, and hence its transitions are never moved.

      We tested different alternatives to store the transitions, but they all
      performed worse: storing a vector transitions in the label group increases
      memory usage and runtime; storing the transitions more compactly and
      incrementally increasing the size of transitions_of_groups whenever a
      new label group is added also increases runtime. See also issue492 and
      issue521.
    */
    std::vector<std::vector<Transition>> transitions_by_group_id;

    int num_states;
    std::vector<bool> goal_states;
    int init_state;

    std::shared_ptr<Distances> init_distances, goal_distances;
    /*
      Check if two or more labels are locally equivalent to each other, and
      if so, update the label equivalence relation.
    */
    void compute_locally_equivalent_labels();

    const std::vector<Transition> &get_transitions_for_group_id(int group_id) const {
        return transitions_by_group_id[group_id];
    }

    // Statistics and output
    int compute_total_transitions() const;
    std::string get_description() const;
public:
    TransitionSystem(
        int num_variables,
        std::vector<int> &&incorporated_variables,
        std::unique_ptr<LabelEquivalenceRelation> &&label_equivalence_relation,
        std::vector<std::vector<Transition>> &&transitions_by_label,
        int num_states,
        std::vector<bool> &&goal_states,
        int init_state,
        bool compute_label_equivalence_relation);
    ~TransitionSystem() = default;

    TSConstIterator begin() const {
        return TSConstIterator(*label_equivalence_relation,
                               transitions_by_group_id,
                               false);
    }

    TSConstIterator end() const {
        return TSConstIterator(*label_equivalence_relation,
                               transitions_by_group_id,
                               true);
    }

    /*
      Method to identify the transition system in output.
      Print "Atomic transition system #x: " for atomic transition systems,
      where x is the variable. For composite transition systems, print
      "Transition system (x/y): " for a transition system containing x
      out of y variables.
    */
    std::string tag() const;

    /*
      The transitions for every group of locally equivalent labels are
      sorted (by source, by target) and there are no duplicates.
    */
    bool are_transitions_sorted_unique() const;

    bool is_solvable() const;

    
    const Distances &get_init_distances() const {
        return *init_distances;
    }

    const Distances &get_goal_distances() const {
        return *goal_distances;
    }

    void dump_dot_graph() const;
    void dump_labels_and_transitions() const;
    void statistics() const;

    bool is_unit_cost() const;
    
    bool is_goal_relevant() const;

    int get_size() const {
        return num_states;
    }

    int get_init_state() const {
        return init_state;
    }

    bool is_goal_state(int state) const {
        return goal_states[state];
    }

    const std::vector<int> &get_incorporated_variables() const {
        return incorporated_variables;
    }
};
}

#endif
