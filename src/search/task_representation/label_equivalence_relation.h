#ifndef FTS_REPRESENTATION_LABEL_EQUIVALENCE_RELATION_H
#define FTS_REPRESENTATION_LABEL_EQUIVALENCE_RELATION_H


#include <list>
#include <unordered_set>
#include <vector>

#include "../algorithms/segmented_vector.h"
#include "types.h"

namespace task_transformation {
    class LabelMapping;
}

namespace task_representation {
class Labels;

class LabelGroup {
    /*
      A label group contains a set of locally equivalent labels, possibly of
      different cost, and stores the minimum cost of all labels of the group.
    */
    std::list<int> labels;
    int cost;
public:
    LabelGroup();

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
      This class groups labels together and allows easy acces to the group
      and position within a group for every label. It is used by the class
      TransitionSystem to group locally equivalent labels. Label groups
      have implicit IDs defined by their index in grouped_labels.
    */
    const Labels &labels;

    /*
      NOTE: it is somewhat dangerous to use lists inside vectors and storing
      iterators to these lists, because whenever the vector needs to be
      resized, these iterators may become invalid. In the constructor, we
      make sure to reserve enough memory so reallocation is never needed.
    */
    segmented_vector::SegmentedVector<LabelGroup> grouped_labels;
    // maps each label to its group's ID and its iterator within the group.
    std::vector<std::pair<LabelGroupID, LabelIter>> label_to_positions;

    void add_label_to_group(LabelGroupID group_id, int label_no);
public:
    /*
      Constructs an empty label equivalence relation. It can be filled using
      the public add_label_group method below.
    */
    explicit LabelEquivalenceRelation(const Labels &labels);
    LabelEquivalenceRelation(const Labels &labels, std::vector<std::vector<int>> & label_groups);
    LabelEquivalenceRelation(const LabelEquivalenceRelation &other);
    /*
      NOTE: we need a custom copy constructor here because we need to fill
      label_to_positions with correct LabelIter objects that point to the
      copied LabelGroup objects rather than to those of the given
      LabelEquivalenceRelation other.
      NOTE: we also need it to add the copy of the labels object.
    */
    LabelEquivalenceRelation(const LabelEquivalenceRelation &other, const Labels &labels);

    /*
      The given label mappings (from label reduction) contain the new label
      and the old label that were reduced to the new one.

      If affected_group_ids is not given, then all old labels must have been
      in the same group before, and the new labels are added to this group.
      Otherwise, all old labels are removed from their group(s) and the new
      label is added to a new group. Furthermore, the costs of the affected
      groups are recomputed.
    */
    void apply_label_mapping(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        const std::unordered_set<LabelGroupID> *affected_group_ids = nullptr);

    void apply_label_mapping(const task_transformation::LabelMapping &label_mapping);

    
    std::vector<LabelGroupID> remove_labels(const std::vector<LabelID> & labels);

    // Moves all labels from one goup into the other
    void move_group_into_group(LabelGroupID from_group_id, LabelGroupID to_group_id);
    int add_label_group(const std::vector<int> &new_labels);

    bool is_empty_group(LabelGroupID group_id) const {
        return grouped_labels[group_id].empty();
    }
    
    LabelGroupID get_group_id(int label_no) const {
        return label_to_positions[label_no].first;
    }

    int get_size() const {
        return grouped_labels.size();
    }

    const LabelGroup &get_group(LabelGroupID group_id) const {
        return grouped_labels[group_id];
    }
};
}

#endif
