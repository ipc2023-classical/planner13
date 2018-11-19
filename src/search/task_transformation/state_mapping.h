#ifndef TASK_TRANSFORMATION_STATE_MAPPING_H
#define TASK_TRANSFORMATION_STATE_MAPPING_H

#include <vector>
#include <memory>
#include <iostream>

class GlobalState;

namespace task_transformation {
    
    class MergeAndShrinkRepresentation;

class StateMapping {
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> merge_and_shrink_representations;

public: 
    StateMapping(std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> && merge_and_shrink_representations_) ;

    std::vector<int> convert_state(const GlobalState & state) const;
    int get_value_abstract_variable(const std::vector<int> & state, int var) const;
    
};

}

#endif
