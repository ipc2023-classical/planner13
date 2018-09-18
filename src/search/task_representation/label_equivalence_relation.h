#ifndef FTS_REPRESENTATION_LABEL_EQUIVALENCE_RELATION_H
#define FTS_REPRESENTATION_LABEL_EQUIVALENCE_RELATION_H

#include "types.h"
#include "labels.h"

#include <algorithm>
#include <unordered_set>
#include <vector>
#include <cassert>

namespace task_representation {
class LabelGroup {
    /*
      A label group contains a set of locally equivalent labels, possibly of
      different cost, and stores the minimum cost of all labels of the group.
    */
    std::vector<LabelID> labels;
    int cost;
public:
    LabelGroup() : cost(INF) {
    }

    void set_cost(int cost_) {
        cost = cost_;
    }

    void insert(LabelID label) {
        return labels.push_back(label);
    }

    void erase(LabelID label) {
	auto pos = std::find(labels.begin(), labels.end(), label);
	assert (pos != labels.end());
	labels.erase(pos);
    }

    void clear() {
        labels.clear();
    }

    std::vector<LabelID>::const_iterator begin() const {
        return labels.begin();
    }

    std::vector<LabelID>::const_iterator end() const {
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
      This class groups labels together and allows easy access to the group. It is used by
      the class TransitionSystem to group locally equivalent labels. Label groups have
      implicit IDs defined by their index in label_groups.
    */

    const Labels &labels;

    std::vector<LabelGroup> label_groups;
    // maps each label to its group's ID and its iterator within the group.
    std::vector<LabelGroupID> label_to_groups;

    void add_label_to_group(LabelGroupID group_id, LabelID label_no);
public:
    /*
      Constructs an empty label equivalence relation. It can be filled using
      the public add_label_group method below.
    */
    explicit LabelEquivalenceRelation(const Labels &labels);

    /*
      The given label mappings (from label reduction) contain the new label
      and the old label that were reduced to the new one.

      If affected_group_ids is not given, then all old labels must have been
      in the same group before, and the new labels are added to this group.
      Otherwise, all old labels are removed from their group(s) and the new
      label is added to a new group. Furthermore, the costs of the affected
      groups are recomputed.
    */
    void apply_label_mapping(const std::vector<std::pair<LabelID, std::vector<LabelID>>> &label_mapping,
			     const std::unordered_set<LabelGroupID> *affected_group_ids = nullptr);
    
    // Moves all labels from one goup into the other
    void move_group_into_group(LabelGroupID from_group_id, LabelGroupID to_group_id);
    int add_label_group(const std::vector<LabelID> &new_labels);

    bool is_empty_group(LabelGroupID group_id) const {
        return label_groups[group_id].empty();
    }

    LabelGroupID get_group_id(LabelID label_no) const {
        return label_to_groups[label_no];
    }

    int get_size() const {
        return label_groups.size();
    }

    const LabelGroup &get_group(LabelGroupID group_id) const {
        return label_groups.at(group_id);
    }
};
}

#endif
