#include "label_equivalence_relation.h"

#include "labels.h"

#include <cassert>

using namespace std;

namespace task_representation {
LabelEquivalenceRelation::LabelEquivalenceRelation(const Labels &labels)
    : labels(labels) {
    label_to_groups.resize(labels.get_size());
}

void LabelEquivalenceRelation::add_label_to_group(LabelGroupID group_id, LabelID label_no) {
    label_groups[group_id].insert(label_no);
    label_to_groups[label_no] = group_id;

    int label_cost = labels.get_label_cost(label_no);
    if (label_cost < label_groups[group_id].get_cost()) {
        label_groups[group_id].set_cost(label_cost);
    }
}

void LabelEquivalenceRelation::apply_label_mapping(
    const vector<pair<LabelID, vector<LabelID>>> &label_mapping,
    const unordered_set<LabelGroupID> *affected_group_ids) {
    for (const auto &mapping : label_mapping) {
        LabelID new_label_no = mapping.first;
        const auto &old_label_nos = mapping.second;

        // Add new label to group
        LabelGroupID canonical_group_id = get_group_id(old_label_nos.front());
        if (!affected_group_ids) {
            add_label_to_group(canonical_group_id, new_label_no);
        } else {
            add_label_group({new_label_no});
        }

        // Remove old labels from group
        for (LabelID old_label_no : old_label_nos) {
            if (!affected_group_ids) {
                assert(canonical_group_id == get_group_id(old_label_no));
            }
            label_groups[get_group_id(old_label_no)].erase(old_label_no);
        }
    }

    if (affected_group_ids) {
        // Recompute the cost of all affected label groups.
        const auto &group_ids = *affected_group_ids;
        for (LabelGroupID group_id : group_ids) {
            LabelGroup &label_group = label_groups[group_id];
            // Setting cost to infinity for empty groups does not hurt.
            label_group.set_cost(INF);
            for (LabelID label_no : label_group) {
                int cost = labels.get_label_cost(label_no);
                if (cost < label_group.get_cost()) {
                    label_group.set_cost(cost);
                }
            }
        }
    }
}

void LabelEquivalenceRelation::move_group_into_group(LabelGroupID from_group_id, LabelGroupID to_group_id) {
    assert(!is_empty_group(from_group_id));
    assert(!is_empty_group(to_group_id));
    LabelGroup &from_group = label_groups[from_group_id];
    for (LabelID label_no : from_group) {
        add_label_to_group(to_group_id, label_no);
    }
    from_group.clear();
}

int LabelEquivalenceRelation::add_label_group(const vector<LabelID> &new_labels) {
    LabelGroupID new_group_id (label_groups.size());
    label_groups.push_back(LabelGroup());
    for (LabelID label_no : new_labels) {
        add_label_to_group(new_group_id, label_no);
    }
    return new_group_id;
}
}
