#ifndef FTS_REPRESENTATION_LABELS_H
#define FTS_REPRESENTATION_LABELS_H

#include <memory>
#include <vector>
#include "types.h"
#include "../task_transformation/label_map.h"
#include "../operator_cost.h"

namespace task_representation {
class SASTask;

class Label {
    /*
      This class implements labels as used by merge-and-shrink transition systems.
      Labels are opaque tokens that have an associated cost.
    */
    int cost;
public:
    explicit Label(int cost_)
        : cost(cost_) {
    }

    Label(const Label & other, OperatorCost cost_type);
    Label(const Label &other) = default;
    ~Label() {}
    int get_cost() const {
        return cost;
    }
};

/*
  This class serves both as a container class to handle the set of all labels
  and to perform label reduction on this set.
*/
class Labels {
    std::vector<std::unique_ptr<Label>> labels;
    int max_size; // the maximum number of labels that can be created
    int num_active_entries; // the number of valid indices in labels
//    std::vector<std::vector<int>> sas_op_indices_by_label;
public:
    Labels(
        std::vector<std::unique_ptr<Label>> &&labels,
        int max_size);

    Labels(const Labels & other, OperatorCost cost_type);

    task_transformation::LabelMapping  cleanup();
        
//        std::vector<std::vector<int>> &&sas_op_indices_by_label);
    Labels(const Labels &other);
    ~Labels();
    void reduce_labels(const std::vector<int> &old_label_nos);
    std::unique_ptr<Label> extract_label(int label_no);
    bool is_current_label(int label_no) const;
    int get_label_cost(int label_no) const;
    void dump_labels() const;

    int get_size() const {
        return labels.size();
    }

    int get_max_size() const {
        return max_size;
    }

    int get_num_active_entries() const {
        return num_active_entries;
    }

    void remove_labels(const std::vector<LabelID> & labels);

//    const std::vector<int> &get_sas_op_indices_for_label(int label) const {
//        return sas_op_indices_by_label[label];
//    }
};
}

#endif
