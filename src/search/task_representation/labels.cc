#include "labels.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include "../task_representation/sas_task.h"
#include "../task_transformation/label_map.h"

#include <cassert>
#include <iostream>

using namespace std;
using namespace task_transformation;
namespace task_representation {

    Label::Label(const Label & other, OperatorCost cost_type) :
        cost (get_adjusted_action_cost(other.cost, cost_type)) {
    }

    Labels::Labels(const Labels & other, OperatorCost cost_type) :
        max_size(other.max_size), num_active_entries(other.num_active_entries) {
        for (auto & l  : other.labels) {
            labels.push_back(utils::make_unique_ptr<Label>(*l, cost_type));
        }
    }
Labels::Labels(
    vector<unique_ptr<Label>> &&labels,
    int max_size)
//    vector<vector<int>> &&sas_op_indices_by_label)
    : labels(move(labels)),
      max_size(max_size),
      num_active_entries(this->labels.size()) {
//      sas_op_indices_by_label(move(sas_op_indices_by_label)) {
}

    LabelMapping  Labels::cleanup() {
        vector<int> old_to_new_labels (get_size(), -1);
        int new_label_no = 0;
        for (int label_no = 0; label_no < get_size(); ++label_no) {
            if (is_current_label(label_no)) {
                if (label_no != new_label_no) {
                    labels[new_label_no] = extract_label(label_no);
                }
                old_to_new_labels[label_no] = new_label_no++;
            }
        }
        labels.resize(new_label_no);

        
        return LabelMapping(old_to_new_labels, new_label_no);        
    }

Labels::Labels(const Labels &other)
    : max_size(other.max_size), num_active_entries(other.num_active_entries) {
    labels.reserve(other.labels.size());
    for (const unique_ptr<Label> &label : other.labels) {
        if (label) {
            labels.push_back(utils::make_unique_ptr<Label>(label->get_cost()));
        } else {
            labels.push_back(nullptr);
        }
    }
}

// Due to the LabelMap pointer
Labels::~Labels() {
}

void Labels::reduce_labels(const vector<int> &old_label_nos) {
    int new_label_cost = labels[old_label_nos[0]]->get_cost();
//    sas_op_indices_by_label.push_back(vector<int>());
    for (size_t i = 0; i < old_label_nos.size(); ++i) {
        int old_label_no = old_label_nos[i];
        labels[old_label_no] = nullptr;
//        const vector<int> &ops = sas_op_indices_by_label[old_label_no];
//        sas_op_indices_by_label.back().insert(sas_op_indices_by_label.back().end(), ops.begin(), ops.end());
//        sas_op_indices_by_label[old_label_no].clear();
    }
    labels.push_back(utils::make_unique_ptr<Label>(new_label_cost));
    num_active_entries -= (old_label_nos.size() - 1);
}

void Labels::remove_labels(const vector<LabelID> & removed_labels) {
    for (int label : removed_labels) {
        labels[label] = nullptr;
    }
    num_active_entries -= (removed_labels.size());
}

unique_ptr<Label> Labels::extract_label(int label_no) {
    assert(is_current_label(label_no));
    return move(labels[label_no]);
}

bool Labels::is_current_label(int label_no) const {
    assert(utils::in_bounds(label_no, labels));
    return labels[label_no] != nullptr;
}

int Labels::get_label_cost(int label_no) const {
    assert(labels[label_no]);
    return labels[label_no]->get_cost();
}

void Labels::dump_labels() const {
    cout << "active labels:" << endl;
    for (size_t label_no = 0; label_no < labels.size(); ++label_no) {
        if (labels[label_no]) {
            cout << "label " << label_no
                 << ", cost " << labels[label_no]->get_cost()
                 << endl;
        }
    }
}
}
