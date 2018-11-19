#include "state_mapping.h"
#include "merge_and_shrink_representation.h"

using namespace std;
namespace task_transformation {
    
    StateMapping::StateMapping(std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&
                               merge_and_shrink_representations_) :
        merge_and_shrink_representations(std::move(merge_and_shrink_representations_)) {}


    std::vector<int> StateMapping::convert_state(const GlobalState & state) const {
        vector<int> values(merge_and_shrink_representations.size());
        for (size_t var = 0; var < merge_and_shrink_representations.size(); ++var) {
            values[var] =  merge_and_shrink_representations[var]->get_value(state);
            if (values[var] == -1) {
                values.clear(); //dead end states are represented by an empty vector
                return values;
            }
        }
        return values;
    }

    int StateMapping::get_value_abstract_variable(const std::vector<int> & state, int var) const {
        return merge_and_shrink_representations[var]->get_value(state);
    }
}
