#ifndef MERGE_AND_SHRINK_LABEL_EQUIVALENCE_RELATION_H
#define MERGE_AND_SHRINK_LABEL_EQUIVALENCE_RELATION_H

#include "types.h"

#include <list>
#include <unordered_set>
#include <vector>

namespace merge_and_shrink {
class Labels;

using LabelIter = std::list<int>::iterator;
using LabelConstIter = std::list<int>::const_iterator;

class LabelGroup {
    /*
      A label group contains a set of locally equivalent labels, possibly of
      different cost, and stores the minimum cost of all labels of the group.
    */
    std::list<int> labels;
    int cost;
public:
    LabelGroup() : cost(INF) {
    }

    void set_cost(int cost_) {
        cost = cost_;
    }

    LabelIter insert(int label) {
        return labels.insert(labels.end(), label);
    }

    void erase(LabelIter pos) {
        labels.erase(pos);
    }

    void clear() {
        labels.clear();
    }

    LabelConstIter begin() const {
        return labels.begin();
    }

    LabelConstIter end() const {
        return labels.end();
    }

    bool empty() const {
        return labels.empty();
    }

    int get_cost() const {
        return cost;
    }
};

class LabelEquivalenceRelation {
    /*
      This class groups labels together and allows easy access to the group
      and position within a group for every label. It is used by the class
      TransitionSystem to group locally equivalent labels. Label groups
      have implicit IDs defined by their index in grouped_labels.
    */

    const Labels &labels;

    /*
      NOTE: it is somewhat dangerous to use lists inside vectors and storing
      iterators to these lists, because whenever the vector needs to be
      resized, iterators to these lists may become invalid. In the constructor,
      we thus make sure to reserve enough memory so reallocation is never needed.
    */
    std::vector<LabelGroup> grouped_labels;
    // maps each label to its group's ID and its iterator within the group.
    std::vector<std::pair<int, LabelIter>> label_to_positions;

    void add_label_to_group(int group_id, int label_no);
public:
    /*
      Constructs an empty label equivalence relation. It can be filled using
      the public add_label_group method below.
    */
    explicit LabelEquivalenceRelation(const Labels &labels);
    /*
      NOTE: we need a custom copy constructor here because we need to fill
      label_to_positions with correct LabelIter objects that point to the
      copied LabelGroup objects rather than to those of the given
      LabelEquivalenceRelation other.
    */
    LabelEquivalenceRelation(const LabelEquivalenceRelation &other);

    /*
      The given label mappings (from label reduction) contain the new label
      and the old labels that were reduced to the new one.

      If affected_group_ids is not given, then all old labels must have been
      in the same group before, and the new labels are added to this group.
      Otherwise, all old labels are removed from their group(s) and the new
      label is added to a new group. Furthermore, the costs of the affected
      groups are recomputed.
    */
    void apply_label_mapping(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        const std::unordered_set<int> *affected_group_ids = nullptr);
    // Moves all labels from one group into the other.
    void move_group_into_group(int from_group_id, int to_group_id);
    int add_label_group(const std::vector<int> &new_labels);
    int add_label_group(std::list<int>::const_iterator start_it, std::list<int>::const_iterator end_it);

    bool is_empty_group(int group_id) const {
        return grouped_labels[group_id].empty();
    }

    int get_group_id(int label_no) const {
        return label_to_positions[label_no].first;
    }

    int get_size() const {
        return grouped_labels.size();
    }

    const LabelGroup &get_group(int group_id) const {
        return grouped_labels.at(group_id);
    }
};
}

#endif
