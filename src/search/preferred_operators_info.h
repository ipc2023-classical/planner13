#ifndef PREFERRED_OPERATORS_INFO_H
#define PREFERRED_OPERATORS_INFO_H

#include "operator_id.h"
#include "task_representation/fact.h"
#include "task_transformation/types.h"
#include "global_state.h" 
#include <map>
#include "algorithms/ordered_set.h"

namespace task_representation{
    class   SearchTask;
}

class PreferredOperatorsInfo {
    task_transformation::Mapping mapping;
    std::map<int, std::vector<task_representation::FactPair>> preferred_effects_by_label;
    
public:
    void set_mapping(const task_transformation::Mapping & mapping_) {
        mapping = mapping_;
    }
    
    void clear();
    
    void set_preferred(int label, const task_representation::FactPair & fact_pair);

    void get_preferred_operators(const GlobalState & state,
                                 const task_representation::SearchTask & search_task, 
                                 const std::vector<OperatorID> & applicable_operators,
                                 ordered_set::OrderedSet<OperatorID> & result_preferred_operators) const;
};

#endif
