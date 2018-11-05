#include "label_map.h"

#include <numeric>
#include <iostream>
#include <set>

using namespace std;

namespace task_transformation {
LabelMap::LabelMap(int num_labels) {
    reduced_labels.resize(num_labels);
    iota(reduced_labels.begin(), reduced_labels.end(), 0);
}

void LabelMap::update(const vector<int> &old_to_new_labels) {
    for (int &entry : reduced_labels) {
        if (old_to_new_labels[entry] != -1) {
            entry = old_to_new_labels[entry];
        }
    }
}

void LabelMap::dump() const {
    for (int entry : reduced_labels) {
        cout << entry << " ";
    }
    cout << endl;
}
}
