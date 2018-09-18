#include "transition_system.h"

#include "label_equivalence_relation.h"
#include "labels.h"

#include "../utils/collections.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace task_representation {
ostream &operator<<(ostream &os, const Transition &trans) {
    os << trans.src << "->" << trans.target;
    return os;
}

TSConstIterator::TSConstIterator(
    const LabelEquivalenceRelation &label_equivalence_relation,
    const vector<vector<Transition>> &transitions_by_group_id,
    bool end)
    : label_equivalence_relation(label_equivalence_relation),
      transitions_by_group_id(transitions_by_group_id),
      current_group_id((end ? label_equivalence_relation.get_size() : 0)) {
    next_valid_index();
}

void TSConstIterator::next_valid_index() {
    while (current_group_id < label_equivalence_relation.get_size()
           && label_equivalence_relation.is_empty_group(current_group_id)) {
        ++current_group_id;
    }
}

void TSConstIterator::operator++() {
    ++current_group_id;
    next_valid_index();
}

GroupAndTransitions TSConstIterator::operator*() const {
    return GroupAndTransitions(
        label_equivalence_relation.get_group(current_group_id),
        transitions_by_group_id[current_group_id]);
}


/*
  Implementation note: Transitions are grouped by their label groups,
  not by source state or any such thing. Such a grouping is beneficial
  for fast generation of products because we can iterate label group
  by label group, and it also allows applying transition system
  mappings very efficiently.

  We rarely need to be able to efficiently query the successors of a
  given state; actually, only the distance computation requires that,
  and it simply generates such a graph representation of the
  transitions itself. Various experiments have shown that maintaining
  a graph representation permanently for the benefit of distance
  computation is not worth the overhead.
*/

TransitionSystem::TransitionSystem(
    int num_variables,
    vector<int> &&incorporated_variables,
    unique_ptr<LabelEquivalenceRelation> &&label_equivalence_relation,
    vector<vector<Transition>> &&transitions_by_label,
    int num_states,
    vector<bool> &&goal_states,
    int init_state,
    bool compute_label_equivalence_relation)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      label_equivalence_relation(move(label_equivalence_relation)),
      transitions_by_group_id(move(transitions_by_label)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    if (compute_label_equivalence_relation) {
        compute_locally_equivalent_labels();
    }
    assert(are_transitions_sorted_unique());
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.
    */
    for (LabelGroupID group_id1 (0); group_id1 < label_equivalence_relation->get_size(); ++group_id1) {
        if (!label_equivalence_relation->is_empty_group(group_id1)) {
            const vector<Transition> &transitions1 = transitions_by_group_id[group_id1];
            for (LabelGroupID group_id2 (group_id1 + 1);
                 group_id2 < label_equivalence_relation->get_size(); ++group_id2) {
                if (!label_equivalence_relation->is_empty_group(group_id2)) {
                    vector<Transition> &transitions2 = transitions_by_group_id[group_id2];
                    if ((transitions1.empty() && transitions2.empty())
                        || transitions1 == transitions2) {
                        label_equivalence_relation->move_group_into_group(
                            group_id2, group_id1);
                        utils::release_vector_memory(transitions2);
                    }
                }
            }
        }
    }
}

string TransitionSystem::tag() const {
    string desc(get_description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (const GroupAndTransitions &gat : *this) {
        if (!utils::is_sorted_unique(gat.transitions))
            return false;
    }
    return true;
}

// bool TransitionSystem::is_solvable(const Distances &distances) const {
//     return init_state != PRUNED_STATE &&
//            distances.get_goal_distance(init_state) != INF;
// }

int TransitionSystem::compute_total_transitions() const {
    int total = 0;
    for (const GroupAndTransitions &gat : *this) {
        total += gat.transitions.size();
    }
    return total;
}

string TransitionSystem::get_description() const {
    ostringstream s;
    if (incorporated_variables.size() == 1) {
        s << "atomic transition system #" << *incorporated_variables.begin();
    } else {
        s << "composite transition system with "
          << incorporated_variables.size() << "/" << num_variables << " vars";
    }
    return s.str();
}

void TransitionSystem::dump_dot_graph() const {
    assert(are_transitions_sorted_unique());
    cout << "digraph transition_system";
    for (size_t i = 0; i < incorporated_variables.size(); ++i)
        cout << "_" << incorporated_variables[i];
    cout << " {" << endl;
    cout << "    node [shape = none] start;" << endl;
    for (int i = 0; i < num_states; ++i) {
        bool is_init = (i == init_state);
        bool is_goal = (goal_states[i] == true);
        cout << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
             << "] node" << i << ";" << endl;
        if (is_init)
            cout << "    start -> node" << i << ";" << endl;
    }
    for (const GroupAndTransitions &gat : *this) {
        const LabelGroup &label_group = gat.label_group;
        const vector<Transition> &transitions = gat.transitions;
        for (const Transition &transition : transitions) {
            int src = transition.src;
            int target = transition.target;
            cout << "    node" << src << " -> node" << target << " [label = ";
            for (auto label_it = label_group.begin();
                 label_it != label_group.end(); ++label_it) {
                if (label_it != label_group.begin())
                    cout << "_";
                cout << "x" << *label_it;
            }
            cout << "];" << endl;
        }
    }
    cout << "}" << endl;
}

void TransitionSystem::dump_labels_and_transitions() const {
    cout << tag() << "transitions" << endl;
    for (const GroupAndTransitions &gat : *this) {
        const LabelGroup &label_group = gat.label_group;
//        cout << "group ID: " << ts_it.get_id() << endl;
        cout << "labels: ";
        for (auto label_it = label_group.begin();
             label_it != label_group.end(); ++label_it) {
            if (label_it != label_group.begin())
                cout << ",";
            cout << *label_it;
        }
        cout << endl;
        cout << "transitions: ";
        const vector<Transition> &transitions = gat.transitions;
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            if (i != 0)
                cout << ",";
            cout << src << " -> " << target;
        }
        cout << endl;
        cout << "cost: " << label_group.get_cost() << endl;
    }
}

void TransitionSystem::statistics() const {
    cout << tag() << get_size() << " states, "
         << compute_total_transitions() << " arcs " << endl;
}


bool TransitionSystem::is_goal_relevant() const {
    for(bool is_goal : goal_states) {
        if (!is_goal) {
            return true;
        }
    }
    return false;
}



bool TransitionSystem::is_unit_cost() const {
    for (const GroupAndTransitions &gat : *this) {
        const LabelGroup &label_group = gat.label_group;
        if (label_group.get_cost() != 1)
            return false;
    }
    return true;
}
}
