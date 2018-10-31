#include "labels.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include "../task_representation/sas_task.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace task_representation {
Labels::Labels(
    vector<unique_ptr<Label>> &&labels,
    int max_size,
    vector<vector<int>> &&sas_op_indices_by_label)
    : labels(move(labels)),
      max_size(max_size),
      sas_op_indices_by_label(move(sas_op_indices_by_label)) {
}

void Labels::reduce_labels(const vector<int> &old_label_nos) {
    int new_label_cost = labels[old_label_nos[0]]->get_cost();
    sas_op_indices_by_label.push_back(vector<int>());
    for (size_t i = 0; i < old_label_nos.size(); ++i) {
        int old_label_no = old_label_nos[i];
        labels[old_label_no] = nullptr;
        const vector<int> &ops = sas_op_indices_by_label[old_label_no];
        sas_op_indices_by_label.back().insert(sas_op_indices_by_label.back().end(), ops.begin(), ops.end());
        sas_op_indices_by_label[old_label_no].clear();
    }
    labels.push_back(utils::make_unique_ptr<Label>(new_label_cost));
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
