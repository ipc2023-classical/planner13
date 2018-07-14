#ifndef FTS_REPRESENTATION_LABELS_H
#define FTS_REPRESENTATION_LABELS_H


#include "types.h"

#include <memory>
#include <vector>

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
public:
    explicit Labels(const SASTask & sas_task);
    void reduce_labels(const std::vector<LabelID> &old_label_nos);
    bool is_current_label(LabelID label_no) const;
    int get_label_cost(LabelID label_no) const;
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
};


#endif
