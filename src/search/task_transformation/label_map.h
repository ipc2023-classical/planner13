#ifndef TASK_TRANSFORMATION_LABEL_MAP_H
#define TASK_TRANSFORMATION_LABEL_MAP_H

#include <vector>

namespace task_transformation {
class LabelMap{
    std::vector<int> reduced_labels; // reduced label numbers indexed by original label number
//    std::vector<int> original_labels; // indexed by reduced label number
public:
    explicit LabelMap(int num_labels);
    void update(int new_label_no, const std::vector<int> &old_label_nos);

    int get_reduced_label(int old_label) const{
        return reduced_labels[old_label];
    }

//    int get_original_label(int reduced_label) const{
//        return original_labels[reduced_label];
//    }
};
}

#endif
