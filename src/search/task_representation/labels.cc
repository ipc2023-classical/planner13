#include "labels.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include "../task_representation/sas_task.h"

#include <cassert>
#include <iostream>

using namespace std;


Labels::Labels(const SASTask & sas_task) {
    for (int index = 0; index < sas_task.get_num_operators(); ++index) {
        labels.push_back(utils::make_unique_ptr<Label>(sas_task.get_operator_cost(index, false)));
    }
}

void Labels::reduce_labels(const vector<LabelID> &old_label_nos) {
    int new_label_cost = labels[old_label_nos[0]]->get_cost();
    for (size_t i = 0; i < old_label_nos.size(); ++i) {
        int old_label_no = old_label_nos[i];
        labels[old_label_no] = nullptr;
    }
    labels.push_back(utils::make_unique_ptr<Label>(new_label_cost));
}

bool Labels::is_current_label(LabelID label_no) const {
    assert(utils::in_bounds(label_no, labels));
    return labels[label_no] != nullptr;
}

int Labels::get_label_cost(LabelID label_no) const {
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

