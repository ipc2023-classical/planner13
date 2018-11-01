#include "label_map.h"

#include <numeric>
#include <set>

using namespace std;

namespace task_transformation {
LabelMap::LabelMap(int num_labels) {
    reduced_labels.resize(num_labels);
    iota(reduced_labels.begin(), reduced_labels.end(), 0);
//    original_labels.resize(num_labels);
//    iota(original_labels.begin(), original_labels.end(), 0);
}

void LabelMap::update(int new_label_no, const vector<int> &old_label_nos) {
    set<int> old_labels(old_label_nos.begin(), old_label_nos.end());
    for (size_t old_label_no = 0; old_label_no < reduced_labels.size(); ++old_label_no) {
        if (old_labels.count(reduced_labels[old_label_no])) {
            reduced_labels[old_label_no] = new_label_no;
        }
    }
}
}
