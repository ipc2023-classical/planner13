#ifndef FTS_REPRESENTATION_LABELS_H
#define FTS_REPRESENTATION_LABELS_H

#include <memory>
#include <vector>

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
    std::vector<std::vector<int>> sas_op_indices_by_label;
public:
    Labels(
        std::vector<std::unique_ptr<Label>> &&labels,
        int max_size,
        std::vector<std::vector<int>> &&sas_op_indices_by_label);
    void reduce_labels(const std::vector<int> &old_label_nos);
    bool is_current_label(int label_no) const;
    int get_label_cost(int label_no) const;

    int get_min_operator_cost() const {
        if(labels.empty()) {
            return 0;
        }
        int minimum_cost = labels[0]->get_cost();
        for (const auto & label : labels) {
            minimum_cost = std::min(minimum_cost, label->get_cost());
        }
        return minimum_cost;
    }

    void dump_labels() const;
    int get_size() const {
        return labels.size();
    }

    int get_max_size() const {
        return max_size;
    }

    const std::vector<int> &get_sas_op_indices_for_label(int label) const {
        return sas_op_indices_by_label[label];
    }
};
}

#endif
